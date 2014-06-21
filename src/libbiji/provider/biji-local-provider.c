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


#include <uuid/uuid.h>

#include "biji-local-note.h"
#include "biji-local-provider.h"



/*
 * Items are both notes and notebooks
 *
 */

struct BijiLocalProviderPrivate_
{
  BijiProviderInfo    info;

  GFile               *location;
  GFile               *trash_file;
  GHashTable          *items;
  GHashTable          *archives;
  GCancellable        *load_cancellable;

  BijiProviderHelper  *living_helper;
  BijiProviderHelper  *archives_helper;
};


G_DEFINE_TYPE (BijiLocalProvider, biji_local_provider, BIJI_TYPE_PROVIDER)


/* Properties */
enum {
  PROP_0,
  PROP_LOCATION,
  BIJI_LOCAL_PROP
};



static GParamSpec *properties[BIJI_LOCAL_PROP] = { NULL, };


#define ATTRIBUTES_FOR_LOCATION "standard::content-type,standard::name"


static void local_prov_load_archives (BijiProvider *prov);

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
create_notebook_if_needed (gpointer key,
                           gpointer value,
                           gpointer user_data)
{
  BijiLocalProvider *self;
  BijiProviderHelper *helper;
  BijiInfoSet *set;
  BijiNotebook *notebook;
  BijiManager *manager;

  helper = user_data;

  /* hmm. rather not load notebooks for archives
   * this is the work for the restore task
   * to handle notebooks. */
  if (helper->group == BIJI_ARCHIVED_ITEMS)
    return;

  set = value;
  self = BIJI_LOCAL_PROVIDER (helper->provider);
  notebook = g_hash_table_lookup (self->priv->items, key);
  manager = biji_provider_get_manager (BIJI_PROVIDER (self));

  if (!notebook)
  {
    notebook = biji_notebook_new (G_OBJECT (manager), key, set->title, set->mtime);

    g_hash_table_insert (self->priv->items,
                         g_strdup (key),
                         notebook);
  }

  /* InfoSet are freed per g_hash_table_destroy thanks to below caller */
}


static void
local_provider_finish (GHashTable *notebooks,
                       gpointer user_data)
{
  BijiLocalProvider *self;
  BijiProviderHelper *helper;
  GList *list;

  helper = user_data;
  self = BIJI_LOCAL_PROVIDER (helper->provider);

  if (helper->group == BIJI_LIVING_ITEMS)
    g_hash_table_foreach (notebooks, create_notebook_if_needed, user_data);

  g_hash_table_destroy (notebooks);

  /* Now simply provide data to controller */
  if (helper->group == BIJI_LIVING_ITEMS)
    list = g_hash_table_get_values (self->priv->items);
  else
    list = g_hash_table_get_values (self->priv->archives);

  BIJI_PROVIDER_GET_CLASS (self)->notify_loaded (BIJI_PROVIDER (self), list, helper->group);
  g_list_free (list);

  /* Now if we just loaded items, load the trash */
  if (helper->group == BIJI_LIVING_ITEMS)
    local_prov_load_archives (BIJI_PROVIDER (self));
}


static void
enumerate_next_files_ready_cb (GObject *source,
                               GAsyncResult *res,
                               gpointer user_data)
{
  GFileEnumerator *enumerator;
  BijiLocalProvider *self;
  BijiProviderHelper *helper;
  GList *files, *l;
  GError *error;
  gchar *base_path;


  enumerator = G_FILE_ENUMERATOR (source);
  helper = user_data;
  self = BIJI_LOCAL_PROVIDER (helper->provider);
  error = NULL;
  files = g_file_enumerator_next_files_finish (enumerator, res, &error);
  g_file_enumerator_close_async (enumerator, G_PRIORITY_DEFAULT, NULL,
                                 release_enum_cb, NULL);

  if (error != NULL)
  {
    load_location_error (g_file_enumerator_get_container (enumerator), error);
    return;
  }


  if (helper->group == BIJI_LIVING_ITEMS)
    base_path = g_file_get_path (self->priv->location);
  else
    base_path = g_file_get_path (self->priv->trash_file);

  for (l = files; l != NULL; l = l->next)
    {
      GFileInfo *file;
      const gchar *name;
      BijiNoteObj *note;
      BijiInfoSet info;
      GHashTable *target;

      file = l->data;
      name = g_file_info_get_name (file);

      if (!g_str_has_suffix (name, ".note"))
        continue;

      info.url = g_build_filename (base_path, name, NULL);
      info.title = "";
      info.content = "";
      info.mtime = 0;


      note = biji_local_note_new_from_info (BIJI_PROVIDER (self),
                                            biji_provider_get_manager (BIJI_PROVIDER (self)),
                                            &info);
      biji_lazy_deserialize (note);

      if (helper->group == BIJI_LIVING_ITEMS)
        target = self->priv->items;
      else
        target = self->priv->archives;

      g_hash_table_replace (target, info.url, note);

    }

  g_free (base_path);
  g_list_free_full (files, g_object_unref);

  /* Now we have all notes,
   * load the notebooks and we're good to notify loading done */
  biji_get_all_notebooks_async (biji_provider_get_manager (BIJI_PROVIDER (self)),
                                local_provider_finish,
                                helper);
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
  BijiProviderHelper *helper;


  location = G_FILE (source);
  helper = user_data;
  self = BIJI_LOCAL_PROVIDER (helper->provider);
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
                                      helper);
}





static void
load_from_location (BijiProviderHelper *helper)
{
  GFile *file;
  GCancellable *cancel;
  BijiLocalProvider *self;

  self = BIJI_LOCAL_PROVIDER (helper->provider);

  if (helper->group == BIJI_LIVING_ITEMS)
  {
    file = self->priv->location;
    cancel = self->priv->load_cancellable;
  }

  else /* BIJI_ARCHIVED_ITEMS */
  {
    file = self->priv->trash_file;
    cancel = NULL;
  }


  g_file_enumerate_children_async (file,
                                   ATTRIBUTES_FOR_LOCATION, 0,
                                   G_PRIORITY_DEFAULT,
                                   cancel,
                                   enumerate_children_ready_cb,
                                   helper);
}


static void
biji_local_provider_constructed (GObject *object)
{
  BijiLocalProvider *self;

  g_return_if_fail (BIJI_IS_LOCAL_PROVIDER (object));
  self = BIJI_LOCAL_PROVIDER (object);


  self->priv->trash_file = g_file_get_child (self->priv->location, ".Trash");
  load_from_location (self->priv->living_helper);
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
  g_object_unref (self->priv->location);
  g_object_unref (self->priv->trash_file);

  biji_provider_helper_free (self->priv->living_helper);
  biji_provider_helper_free (self->priv->archives_helper);

  G_OBJECT_CLASS (biji_local_provider_parent_class)->finalize (object);
}


static void
biji_local_provider_init (BijiLocalProvider *self)
{
  BijiLocalProviderPrivate *priv;

  priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_LOCAL_PROVIDER, BijiLocalProviderPrivate);
  priv->load_cancellable = g_cancellable_new ();
  priv->items = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);
  priv->archives = g_hash_table_new_full (g_str_hash, g_str_equal, NULL, NULL);

  /* Info */
  priv->info.unique_id = "local";
  priv->info.datasource = g_strdup_printf ("local:%s",
                                           priv->info.unique_id);
  priv->info.name = _("Local storage");
  priv->info.icon =
    gtk_image_new_from_icon_name ("user-home", GTK_ICON_SIZE_INVALID);
  gtk_image_set_pixel_size (GTK_IMAGE (priv->info.icon), 48);
  g_object_ref (priv->info.icon);

  /* Helpers */
  priv->living_helper = biji_provider_helper_new (BIJI_PROVIDER (self),
                                                  BIJI_LIVING_ITEMS);
  priv->archives_helper = biji_provider_helper_new (BIJI_PROVIDER (self),
                                                   BIJI_ARCHIVED_ITEMS);
}



/* UUID skeleton for new notes */
static gchar *
_get_uuid (void)
{
  uuid_t unique;
  char out[40];

  uuid_generate (unique);
  uuid_unparse_lower (unique, out);
  return g_strdup_printf ("%s.note", out);
}



static BijiNoteObj *
_get_note_skeleton (BijiLocalProvider *self)
{
  BijiNoteObj *ret = NULL;
  BijiManager *manager;
  gchar * folder, *name, *path;
  BijiInfoSet set;

  manager = biji_provider_get_manager (BIJI_PROVIDER (self));
  set.title = NULL;
  set.content = NULL;
  set.mtime = 0;
  folder = g_file_get_path (self->priv->location);

  while (!ret)
  {
    name = _get_uuid ();
    path = g_build_filename (folder, name, NULL);
    g_free (name);
    set.url = path;

    if (!biji_manager_get_item_at_path (manager, path))
      ret = biji_local_note_new_from_info (BIJI_PROVIDER (self), manager, &set);

    g_free (path);
  }

  biji_note_obj_set_all_dates_now (ret);
  return ret;
}



static BijiNoteObj *
local_prov_create_new_note (BijiProvider *self,
                            gchar        *str)
{
  BijiNoteObj *ret = _get_note_skeleton (BIJI_LOCAL_PROVIDER (self));
  BijiManager *manager;

  manager = biji_provider_get_manager (self);

  if (str)
  {
    gchar *unique, *html;

    unique = biji_manager_get_unique_title (manager, str);
    html = html_from_plain_text (str);

    biji_note_obj_set_title (ret, unique);
    biji_note_obj_set_raw_text (ret, str);
    biji_note_obj_set_html (ret, html);

    g_free (unique);
    g_free (html);

    /* Only save note if content */
    biji_note_obj_save_note (ret);
  }

  return ret;
}

static BijiNoteObj *
local_prov_create_note_full (BijiProvider *provider,
                             gchar        *suggested_path,
                             BijiInfoSet  *info,
                             gchar        *html,
                             GdkRGBA      *color)
{
  BijiLocalProvider *self;
  BijiNoteObj *retval;
  gchar *folder;

  g_return_val_if_fail (BIJI_IS_LOCAL_PROVIDER (provider), NULL);

  self = BIJI_LOCAL_PROVIDER (provider);
  retval = NULL;

  /* PATH */
  folder = g_file_get_path (self->priv->location);
  info->url = g_build_filename (folder, suggested_path, NULL);
  g_free (folder);

  /* RAW NOTE */
  retval = biji_local_note_new_from_info (provider,
                                          biji_provider_get_manager (provider),
                                          info);

  /* EXTRAS */
  biji_note_obj_set_html (retval, html);
  biji_note_obj_set_rgba (retval, color);


  return retval;
}


static void
local_prov_load_archives (BijiProvider *prov)
{
  BijiLocalProvider *self;

  g_return_if_fail (BIJI_IS_LOCAL_PROVIDER (prov));

  self = BIJI_LOCAL_PROVIDER (prov);
  load_from_location (self->priv->archives_helper);
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
  provider_class->create_new_note = local_prov_create_new_note;
  provider_class->create_note_full = local_prov_create_note_full;
  provider_class->load_archives = local_prov_load_archives;

  properties[PROP_LOCATION] =
    g_param_spec_object ("location",
                         "The manager location",
                         "The location where the notes are loaded and saved",
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (g_object_class, BIJI_LOCAL_PROP, properties);

  g_type_class_add_private ((gpointer)klass, sizeof (BijiLocalProviderPrivate));
}


BijiProvider *
biji_local_provider_new (BijiManager *manager,
                         GFile *location)
{
  return g_object_new (BIJI_TYPE_LOCAL_PROVIDER,
                       "manager", manager,
                       "location", location,
                       NULL);
}
