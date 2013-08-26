/*
 * biji-local-provider.c
 * Copyright (C) Pierre-Yves LUYTEN 2013 <py@luyten.fr>
 * 
 * bijiben is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * bijiben is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * as of today, local provider really go through every single
 * file at startup : no cache.
 * We could as well play with metadata mtime
 * and read file only when really needed
 */


#include "biji-local-note.h"
#include "biji-local-provider.h"



/*
 * Items are both notes and collections
 *
 */

struct BijiLocalProviderPrivate_
{
  BijiProviderInfo    info;

  GFile               *location;
  GHashTable          *items;
  GCancellable        *load_cancellable;
};


G_DEFINE_TYPE (BijiLocalProvider, biji_local_provider, BIJI_TYPE_PROVIDER)


/* Properties */
enum {
  PROP_0,
  PROP_LOCATION,
  BIJI_LOCAL_PROP
};



static GParamSpec *properties[BIJI_LOCAL_PROP] = { NULL, };


#define ATTRIBUTES_FOR_NOTEBOOK "standard::content-type,standard::name"


static void
load_location_error (GFile *location,
                     GError *error)
{
  gchar *path;
  

  path = g_file_get_path (location);
  g_printerr ("Unable to load location %s: %s", path, error->message);
  g_free (path);
  g_error_free (error);
}


static void
release_enum_cb (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_file_enumerator_close_finish (G_FILE_ENUMERATOR (source),
                                  res,
                                  NULL);
  g_object_unref (source);
}


static void
create_collection_if_needed (gpointer key,
                             gpointer value,
                             gpointer user_data)
{
  BijiLocalProvider *self;
  BijiInfoSet *set;
  BijiCollection *collection;
  BijiNoteBook *book;


  self = user_data;
  set = value;
  collection = g_hash_table_lookup (self->priv->items, key);
  book = biji_provider_get_book (BIJI_PROVIDER (self));

  if (!collection)
  {
    collection = biji_collection_new (G_OBJECT (book), key, set->title, set->mtime);

    g_hash_table_insert (self->priv->items,
                         g_strdup (key),
                         collection);
  }

  /* InfoSet are freed per g_hash_table_destroy thanks to below caller */
}


static void
local_provider_finish (GHashTable *collections,
                       gpointer user_data)
{
  BijiLocalProvider *self;
  GList *list;


  self = user_data;
  g_hash_table_foreach (collections, create_collection_if_needed, user_data);
  g_hash_table_destroy (collections);


  /* Now simply provide data to controller */
  list = g_hash_table_get_values (self->priv->items);
  BIJI_PROVIDER_GET_CLASS (self)->notify_loaded (BIJI_PROVIDER (self), list);
  g_list_free (list);
}


static void
enumerate_next_files_ready_cb (GObject *source,
                               GAsyncResult *res,
                               gpointer user_data)
{
  GFileEnumerator *enumerator;
  BijiLocalProvider *self;
  GList *files, *l;
  GError *error;
  gchar *base_path;


  enumerator = G_FILE_ENUMERATOR (source);
  self = user_data;
  error = NULL;
  files = g_file_enumerator_next_files_finish (enumerator, res, &error);
  g_file_enumerator_close_async (enumerator, G_PRIORITY_DEFAULT, NULL,
                                 release_enum_cb, NULL);

  if (error != NULL)
  {
    load_location_error (g_file_enumerator_get_container (enumerator), error);
    return;
  }


  base_path = g_file_get_path (self->priv->location);

  for (l = files; l != NULL; l = l->next)
    {
      GFileInfo *file;
      const gchar *name;
      BijiNoteObj *note;
      BijiInfoSet info;

      file = l->data;
      name = g_file_info_get_name (file);

      if (!g_str_has_suffix (name, ".note"))
        continue;

      info.url = g_build_filename (base_path, name, NULL);
      info.title = "";
      info.content = "";
      info.mtime = 0;


      note = biji_local_note_new_from_info (BIJI_PROVIDER (self),
                                            biji_provider_get_book (BIJI_PROVIDER (self)),
                                            &info);
      biji_lazy_deserialize (note);


      g_hash_table_replace (self->priv->items,
                            info.url, note);

    }

  g_free (base_path);
  g_list_free_full (files, g_object_unref);

  /* Now we have all notes,
   * load the collections and we're good to notify loading done */
  biji_get_all_collections_async (biji_provider_get_book (BIJI_PROVIDER (self)),
                                  local_provider_finish,
                                  self);
}

static void
enumerate_children_ready_cb (GObject *source,
                             GAsyncResult *res,
                             gpointer user_data)
{
  GFile *location;
  GFileEnumerator *enumerator;
  GError *error;
  BijiLocalProvider *self;


  location = G_FILE (source);
  self = user_data;
  error = NULL;
  enumerator = g_file_enumerate_children_finish (location,
                                                 res, &error);

  if (error != NULL)
  {
    load_location_error (location, error);
    return;
  }


  g_file_enumerator_next_files_async (enumerator, G_MAXINT,
                                      G_PRIORITY_DEFAULT,
                                      self->priv->load_cancellable,
                                      enumerate_next_files_ready_cb,
                                      self);
}





static void
load_from_location (BijiLocalProvider *self)
{
  g_file_enumerate_children_async (self->priv->location,
                                   ATTRIBUTES_FOR_NOTEBOOK, 0,
                                   G_PRIORITY_DEFAULT,
                                   self->priv->load_cancellable,
                                   enumerate_children_ready_cb,
                                   self);
}


static void
biji_local_provider_constructed (GObject *object)
{
  BijiLocalProvider *self;

  g_return_if_fail (BIJI_IS_LOCAL_PROVIDER (object));
  self = BIJI_LOCAL_PROVIDER (object);

  load_from_location (self);
}


static void
biji_local_provider_finalize (GObject *object)
{
  BijiLocalProvider *self;

  g_return_if_fail (BIJI_IS_LOCAL_PROVIDER (object));

  self = BIJI_LOCAL_PROVIDER (object);

  if (self->priv->load_cancellable)
    g_cancellable_cancel (self->priv->load_cancellable);

  g_object_unref (self->priv->load_cancellable);
  g_object_unref (self->priv->info.icon);

  G_OBJECT_CLASS (biji_local_provider_parent_class)->finalize (object);
}


static void
biji_local_provider_init (BijiLocalProvider *self)
{
  BijiLocalProviderPrivate *priv;

  priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_LOCAL_PROVIDER, BijiLocalProviderPrivate);
  priv->load_cancellable = g_cancellable_new ();
  priv->items = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);

  /* Info */
  priv->info.unique_id = "local";
  priv->info.datasource = g_strdup_printf ("local:%s",
                                           priv->info.unique_id);
  priv->info.name = _("Local storage");
  priv->info.icon =
    gtk_image_new_from_icon_name ("user-home", GTK_ICON_SIZE_INVALID);
  gtk_image_set_pixel_size (GTK_IMAGE (priv->info.icon), 48);
  g_object_ref (priv->info.icon);

}


static void
biji_local_provider_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BijiLocalProvider *self = BIJI_LOCAL_PROVIDER (object);


  switch (property_id)
    {
    case PROP_LOCATION:
      self->priv->location = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_local_provider_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  BijiLocalProvider *self = BIJI_LOCAL_PROVIDER (object);

  switch (property_id)
    {
    case PROP_LOCATION:
      g_value_set_object (value, self->priv->location);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


const BijiProviderInfo *
local_provider_get_info (BijiProvider *provider)
{
  BijiLocalProvider *self;

  self = BIJI_LOCAL_PROVIDER (provider);
  return &(self->priv->info);
}


static void
biji_local_provider_class_init (BijiLocalProviderClass *klass)
{
  GObjectClass *g_object_class;
  BijiProviderClass *provider_class;

  g_object_class = G_OBJECT_CLASS (klass);
  provider_class = BIJI_PROVIDER_CLASS (klass);

  g_object_class->constructed = biji_local_provider_constructed;
  g_object_class->finalize = biji_local_provider_finalize;
  g_object_class->get_property = biji_local_provider_get_property;
  g_object_class->set_property = biji_local_provider_set_property;

  provider_class->get_info = local_provider_get_info;

  properties[PROP_LOCATION] =
    g_param_spec_object ("location",
                         "The book location",
                         "The location where the notes are loaded and saved",
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (g_object_class, BIJI_LOCAL_PROP, properties);

  g_type_class_add_private ((gpointer)klass, sizeof (BijiLocalProviderPrivate));
}


BijiProvider *
biji_local_provider_new (BijiNoteBook *book,
                         GFile *location)
{
  return g_object_new (BIJI_TYPE_LOCAL_PROVIDER,
                       "book", book,
                       "location", location,
                       NULL);
}
