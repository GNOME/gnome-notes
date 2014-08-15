/*
 * biji-import-provider.c
 * 
 * Copyright 2013 Pierre-Yves Luyten <py@luyten.fr>
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
 * TODO : re-implement notebooks
 * TODO : re-implement old imports preserving
 */

#include "biji-import-provider.h"
#include "../deserializer/biji-tomboy-reader.h"


/* Properties */
enum {
  PROP_0,
  PROP_URI,
  PROP_TARGET,
  IMPORT_PROV_PROP
};

static GParamSpec *properties[IMPORT_PROV_PROP] = { NULL, };




struct BijiImportProviderPrivate_
{
  BijiProviderInfo info;
  gchar            *uri;
  gchar            *target; // the provider to import to
  GHashTable       *items; // same as manager, notes key=path, coll key = name.
};





static void biji_import_provider_finalize (GObject *object);

G_DEFINE_TYPE (BijiImportProvider, biji_import_provider, BIJI_TYPE_PROVIDER)


#define ATTRIBUTES_FOR_NOTEBOOK "standard::content-type,standard::name"





static BijiNoteObj *
instanciate_note (BijiImportProvider *self, GFileInfo *info, GFile *container)
{
  BijiNoteObj *retval = NULL;
  const gchar *name;
  gchar *path;
  GdkRGBA *color;
  BijiManager *manager;


  retval = NULL;
  manager = biji_provider_get_manager (BIJI_PROVIDER (self));

  /* First make sure it's a note */
  name = g_file_info_get_name (info);
  if (!g_str_has_suffix (name, ".note"))
    return NULL;

  path = g_build_filename (g_file_get_path (container), name, NULL);


  /* Deserialize it */

  GError *error = NULL;
  BijiInfoSet *set;
  gchar *html = NULL;
  GList *notebooks = NULL;

  biji_tomboy_reader_read   (path,
                             &error,
                             &set,
                             &html,
                             &notebooks);

  if (error != NULL)
  {
    g_warning ("Could not import %s - %s", path, error->message);
    g_free (path);
    g_error_free (error);
    return NULL;
  }


  g_debug ("note %s:%s \n%s\n%s", path, set->title, set->content, html);



  /* Create the note w/ default color */
  color = g_new0 (GdkRGBA, 1);
  biji_manager_get_default_color (manager, color);
  retval = biji_manager_note_new_full (manager,
                                         self->priv->target,
                                         g_strdup (g_file_info_get_name (info)),
                                         set,
                                         html,
                                         color);

  BIJI_NOTE_OBJ_GET_CLASS (retval)->save_note (retval);

  return retval;
}



static void
release_enum_cb (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_file_enumerator_close_finish (G_FILE_ENUMERATOR (source), res, NULL);
  g_object_unref (source);
}



/* Some notes might have been added previously */
static void
go_through_notes_cb (GObject *object, GAsyncResult *res, gpointer data)
{
  GFileEnumerator *enumerator;
  GList *l, *notes_info;
  GFile *container;
  BijiImportProvider *self;


  enumerator = G_FILE_ENUMERATOR (object);
  self = data;


  container = g_file_enumerator_get_container (enumerator);
  notes_info = g_file_enumerator_next_files_finish (enumerator, res, NULL);
  g_file_enumerator_close_async (enumerator, G_PRIORITY_DEFAULT, NULL,
                                 release_enum_cb, self);



  /* Get the GList of notes and load them */
  for ( l=notes_info; l !=NULL; l = l->next)
  {
    GFileInfo *info;
    BijiNoteObj *iter;

    info = G_FILE_INFO (l->data);
    iter = instanciate_note (
                  self,
                  info,
                  container);

    if (iter != NULL)
    {
      g_hash_table_insert (self->priv->items,
                           (gpointer) biji_item_get_uuid (BIJI_ITEM (iter)),
                           (gpointer) iter);
    }
  }

  g_list_free_full (notes_info, g_object_unref);
  BIJI_PROVIDER_GET_CLASS (self)->notify_loaded (BIJI_PROVIDER (self),
                                                 g_hash_table_get_values (self->priv->items),
                                                 BIJI_LIVING_ITEMS);




  /* Goodbye */
  g_object_unref (self);
}




static void
list_notes_to_copy (GObject *src_obj, GAsyncResult *res, gpointer data)
{
  GFileEnumerator *enumerator;
  GError *error;
  BijiImportProvider *self;

  self = data;
  error = NULL;
  enumerator = g_file_enumerate_children_finish (G_FILE (src_obj), res, &error);

  if (error)
  {
    g_warning ("Enumerator failed : %s", error->message);
    g_error_free (error);
    g_file_enumerator_close_async (enumerator, G_PRIORITY_DEFAULT, NULL, release_enum_cb, self);
  }

  else
  {
    g_file_enumerator_next_files_async (enumerator, G_MAXINT, G_PRIORITY_DEFAULT, NULL,
                                        go_through_notes_cb, self);
  }
}



static void
biji_import_provider_constructed (GObject *object)
{
  BijiImportProvider *self;
  BijiImportProviderPrivate *priv;
  GFile *to_import;


  self = BIJI_IMPORT_PROVIDER (object);
  priv = self->priv;


  priv->info.unique_id = NULL;
  priv->info.datasource = NULL;
  priv->info.name = "import-provider";
  priv->info.icon =  NULL;
  priv->info.domain = NULL;
  priv->info.user = NULL;

  
  to_import = g_file_new_for_uri (self->priv->uri);

  g_file_enumerate_children_async (to_import, ATTRIBUTES_FOR_NOTEBOOK,
                                   G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
                                   NULL, list_notes_to_copy, self);

  g_object_unref (to_import);
}


const BijiProviderInfo *
biji_import_provider_get_info (BijiProvider *self)
{
  return & (BIJI_IMPORT_PROVIDER (self)->priv->info);
}


static void
biji_import_provider_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  BijiImportProvider *self = BIJI_IMPORT_PROVIDER (object);


  switch (property_id)
    {
    case PROP_URI:
      self->priv->uri = g_value_dup_string (value);
      break;
    case PROP_TARGET:
      self->priv->target = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
biji_import_provider_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BijiImportProvider *self = BIJI_IMPORT_PROVIDER (object);

  switch (property_id)
    {
    case PROP_URI:
      g_value_set_string (value, self->priv->uri);
      break;
    case PROP_TARGET:
      g_value_set_string (value, self->priv->target);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
biji_import_provider_class_init (BijiImportProviderClass *klass)
{
  GObjectClass *g_object_class;
  BijiProviderClass *provider_class;

  g_object_class = G_OBJECT_CLASS (klass);
  provider_class = BIJI_PROVIDER_CLASS (klass);

  g_object_class->set_property = biji_import_provider_set_property;
  g_object_class->get_property = biji_import_provider_get_property;
  g_object_class->finalize = biji_import_provider_finalize;
  g_object_class->constructed = biji_import_provider_constructed;

  provider_class->get_info = biji_import_provider_get_info;

  properties[PROP_URI] =
    g_param_spec_string ("uri",
                         "The import path",
                         "The folder to be imported",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);


  properties[PROP_TARGET] =
    g_param_spec_string ("target",
                         "The target storage",
                         "Target provider to import to",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (g_object_class, IMPORT_PROV_PROP, properties);


  g_type_class_add_private ((gpointer)klass, sizeof (BijiImportProviderPrivate));
}


static void
biji_import_provider_finalize (GObject *object)
{
  G_OBJECT_CLASS (biji_import_provider_parent_class)->finalize (object);
}


static void
biji_import_provider_init (BijiImportProvider *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_IMPORT_PROVIDER, BijiImportProviderPrivate);
  self->priv->items = g_hash_table_new (g_str_hash, g_str_equal);
}


BijiProvider *
biji_import_provider_new (BijiManager *b, gchar *target_provider, gchar *uri)
{
  return g_object_new (BIJI_TYPE_IMPORT_PROVIDER,
                       "manager", b,
                       "target", target_provider,
                       "uri", uri,
                       NULL);
}

