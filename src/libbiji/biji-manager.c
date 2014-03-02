/* bjb-note-manager.c
 * Copyright (C) Pierre-Yves LUYTEN 2012 <py@luyten.fr>
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

#include <gtk/gtk.h>
#include <uuid/uuid.h>

#include "libbiji.h"
#include "biji-local-note.h" // FIXME !!!! biji_provider_note_new ()
#include "biji-notebook.h"
#include "biji-error.h"

#include "provider/biji-import-provider.h"
#include "provider/biji-local-provider.h"
#include "provider/biji-own-cloud-provider.h"


struct _BijiManagerPrivate
{
  /* Notes & Collections.
   * Keep a direct pointer to local provider for convenience. */

  GHashTable *items;
  GHashTable *archives;
  GHashTable *providers;
  BijiProvider *local_provider;

  /* Signals */
  gulong note_renamed ;

  GFile *location;
  GError *error;
  TrackerSparqlConnection *connection;


#ifdef BUILD_ZEITGEIST
  ZeitgeistLog *log;
#endif /* BUILD_ZEITGEIST */

  GdkRGBA color;
};


/* Properties */
enum {
  PROP_0,
  PROP_LOCATION,
  PROP_COLOR,
  PROP_ERROR,
  BIJI_MANAGER_PROPERTIES
};


/* Signals */
enum {
  BOOK_AMENDED,
  BIJI_MANAGER_SIGNALS
};

static guint biji_manager_signals[BIJI_MANAGER_SIGNALS] = { 0 };
static GParamSpec *properties[BIJI_MANAGER_PROPERTIES] = { NULL, };



#define BIJI_MANAGER_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BIJI_TYPE_MANAGER, BijiManagerPrivate))

G_DEFINE_TYPE (BijiManager, biji_manager, G_TYPE_OBJECT);






static void
biji_manager_init (BijiManager *self)
{
  BijiManagerPrivate *priv;

  priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_MANAGER,
                                                   BijiManagerPrivate);

  /* Item path is key for table */
  priv->items = g_hash_table_new_full (g_str_hash,
                                       g_str_equal,
                                       NULL,
                                       NULL);

  priv->archives = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          NULL,
                                          g_object_unref);
  /*
   * Providers are the different notes storage
   * the hash table use an id
   * 
   * - local files stored notes = "local"
   * - own cloud notes = account_get_id
   */

  priv->providers = g_hash_table_new (g_str_hash, g_str_equal);
}


#ifdef BUILD_ZEITGEIST
ZeitgeistLog *
biji_manager_get_zg_log (BijiManager *manager)
{
  return manager->priv->log;
}
#endif /* BUILD_ZEITGEIST */


TrackerSparqlConnection *
biji_manager_get_tracker_connection (BijiManager *manager)
{
  return manager->priv->connection;
}



GList *
biji_manager_get_providers         (BijiManager *manager)
{
  GList *providers, *l, *retval;

  retval = NULL;
  providers = g_hash_table_get_values (manager->priv->providers);

  for (l = providers; l != NULL; l = l->next)
  {
    retval = g_list_prepend (
               retval, (gpointer) biji_provider_get_info (BIJI_PROVIDER (l->data)));
  }

  g_list_free (providers);
  return retval;
}


static void
biji_manager_finalize (GObject *object)
{
  BijiManager *manager = BIJI_MANAGER (object) ;


  g_clear_object (&manager->priv->location);
  g_hash_table_destroy (manager->priv->items);
  g_hash_table_destroy (manager->priv->archives);


  G_OBJECT_CLASS (biji_manager_parent_class)->finalize (object);
}




static void
biji_manager_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BijiManager *self = BIJI_MANAGER (object);
  GdkRGBA *color;

  switch (property_id)
    {
    case PROP_LOCATION:
      self->priv->location = g_value_dup_object (value);
      break;

    case PROP_ERROR:
      self->priv->error = g_value_get_pointer (value);
      break;

    case PROP_COLOR:
      color = g_value_get_pointer (value);
      self->priv->color.red = color->red;
      self->priv->color.blue = color->blue;
      self->priv->color.green = color->green;
      self->priv->color.alpha = color->alpha;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_manager_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BijiManager *self = BIJI_MANAGER (object);

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

static gboolean
title_is_unique (BijiManager *manager, gchar *title)
{
  gboolean is_unique = TRUE;
  BijiItem *iter;
  GList *items, *l;

  items = g_hash_table_get_values (manager->priv->items);

  for ( l=items ; l != NULL ; l = l->next)
  {
    if (BIJI_IS_ITEM (l->data) == FALSE)
      break;

    iter = BIJI_ITEM (l->data);

    if (g_strcmp0 (biji_item_get_title (iter), title) == 0)
    {
     is_unique = FALSE;
     break;
    }
  }

  g_list_free (items);
  return is_unique;
}

/* If title not unique, add sufffix "n", starting with 2, until ok */
gchar *
biji_manager_get_unique_title (BijiManager *manager, const gchar *title)
{
  if (!manager)
    return g_strdup (title);

  gchar *new_title;

  if (!title)
    title = "";

  new_title = g_strdup (title);
  gint suffix = 2;

  while (!title_is_unique (manager, new_title))
  {
    g_free (new_title);
    new_title = g_strdup_printf("%s (%i)", title, suffix);
    suffix++;
  }

  return new_title;
}


void
biji_manager_notify_changed (BijiManager            *manager,
                             BijiItemsGroup          group,
                             BijiManagerChangeFlag   flag,
                             BijiItem               *item)
{
  g_debug ("manager: notify changed, %i", flag);
  g_signal_emit (manager,
                 biji_manager_signals[BOOK_AMENDED],
                 0,
                 group,
                 flag,
                 item);
}


static void
on_item_deleted_cb (BijiItem *item, BijiManager *manager)
{
  biji_manager_notify_changed (manager,
                               BIJI_ARCHIVED_ITEMS,
                               BIJI_MANAGER_ITEM_DELETED,
                               item);
}


static void
on_item_restored_cb (BijiItem *item, BijiManager *manager)
{
}


void
manager_on_note_changed_cb (BijiNoteObj *note, BijiManager *manager)
{
  biji_manager_notify_changed (manager,
                               BIJI_LIVING_ITEMS,
                               BIJI_MANAGER_NOTE_AMENDED,
                               BIJI_ITEM (note));
}

static void
manager_on_item_icon_changed_cb (BijiNoteObj *note, BijiManager *manager)
{
  biji_manager_notify_changed (manager,
                               BIJI_LIVING_ITEMS,
                               BIJI_MANAGER_ITEM_ICON_CHANGED,
                               BIJI_ITEM (note));
}


gboolean
biji_manager_add_item (BijiManager *manager,
                       BijiItem *item,
                       BijiItemsGroup group,
                       gboolean notify)
{
  const gchar *uid;
  gboolean retval;

  g_return_val_if_fail (BIJI_IS_MANAGER (manager), FALSE);
  g_return_val_if_fail (BIJI_IS_ITEM (item), FALSE);

  retval = TRUE;
  uid = biji_item_get_uuid (item);

  /* Check if item is not already there */
  if (uid != NULL)
  {
    if (group == BIJI_LIVING_ITEMS &&
        g_hash_table_lookup (manager->priv->items, uid))
      retval = FALSE;

    if (group == BIJI_ARCHIVED_ITEMS &&
        g_hash_table_lookup (manager->priv->archives, uid))
      retval = FALSE;
  }


  /* Fail because UUID is null */
  else
    retval = FALSE;

  if (retval == TRUE)
  {
    /* Add the item*/
    if (group == BIJI_LIVING_ITEMS)
      g_hash_table_insert (manager->priv->items,
                           (gpointer) biji_item_get_uuid (item), item);
    else if (group == BIJI_ARCHIVED_ITEMS)
      g_hash_table_insert (manager->priv->archives,
                           (gpointer) biji_item_get_uuid (item), item);

    /* Connect */
    g_signal_connect (item, "deleted",
                      G_CALLBACK (on_item_deleted_cb), manager);
    g_signal_connect (item, "restored",
                      G_CALLBACK (on_item_restored_cb), manager);

    if (BIJI_IS_NOTE_OBJ (item))
    {
      g_signal_connect (item, "changed", G_CALLBACK (manager_on_note_changed_cb), manager);
      g_signal_connect (item, "renamed", G_CALLBACK (manager_on_note_changed_cb), manager);
      g_signal_connect (item, "color-changed", G_CALLBACK (manager_on_item_icon_changed_cb), manager);
    }

    else if (BIJI_IS_NOTEBOOK (item))
    {
      g_signal_connect (item , "icon-changed", G_CALLBACK (manager_on_item_icon_changed_cb), manager);
    }
  }

  if (retval && notify)
    biji_manager_notify_changed (manager, group, BIJI_MANAGER_ITEM_ADDED, item);

  return retval;
}




static void
on_provider_loaded_cb (BijiProvider *provider,
                       GList *items,
                       BijiItemsGroup  group,
                       BijiManager *manager)
{
  GList *l;


  switch (group)
  {
    case BIJI_LIVING_ITEMS:
      for (l=items; l!=NULL; l=l->next)
      {
        if (BIJI_IS_ITEM (l->data))
          biji_manager_add_item (manager, l->data, BIJI_LIVING_ITEMS, FALSE);
      }
      break;

    case BIJI_ARCHIVED_ITEMS:
      for (l=items; l!= NULL; l=l->next)
      {
        if (BIJI_IS_ITEM (l->data))
          biji_manager_add_item (manager, l->data, BIJI_ARCHIVED_ITEMS, FALSE);
      }
      break;

   default:
     break;
  }


  /* More cautious to ask to fully rebuild the model
   * because this might be the first provider.
   * See #708458
   * There are more performant fixes but not worth it */
  biji_manager_notify_changed (manager, group, BIJI_MANAGER_MASS_CHANGE, NULL);
}


/* 
 * It should be the right place
 * to stock somehow providers list
 * in order to handle properly manager__note_new ()
 * 
 */
static void
_add_provider (BijiManager *self,
               BijiProvider *provider)
{
  g_return_if_fail (BIJI_IS_PROVIDER (provider));


  /* we can safely cast get_id from const to gpointer
   * since there is no key free func */

  const BijiProviderInfo *info;

  info = biji_provider_get_info (provider);
  g_hash_table_insert (self->priv->providers, (gpointer) info->unique_id, provider);
  g_signal_connect (provider, "loaded", 
                    G_CALLBACK (on_provider_loaded_cb), self);
}


void
biji_manager_add_goa_object (BijiManager *self,
                               GoaObject *object)
{
  BijiProvider *provider;
  GoaAccount *account;
  const gchar *type;

  provider = NULL;
  account =  goa_object_get_account (object);

  if (GOA_IS_ACCOUNT (account))
  {
    type = goa_account_get_provider_type (account);

    if (g_strcmp0 (type, "owncloud") ==0)
      provider = biji_own_cloud_provider_new (self, object);
  }

  _add_provider (self, provider);
}


static void
biji_manager_constructed (GObject *object)
{
  BijiManager *self;
  BijiManagerPrivate *priv;
  gchar *filename;
  GFile *cache;
  GError *error;


  G_OBJECT_CLASS (biji_manager_parent_class)->constructed (object);
  self = BIJI_MANAGER (object);
  priv = self->priv;
  error = NULL;

  /* If tracker fails for some reason,
   * do not attempt anything */
  priv->connection = tracker_sparql_connection_get (NULL, &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    priv->error = g_error_new (BIJI_ERROR, BIJI_ERROR_TRACKER, "Tracker is not available");
    return;
  }


#ifdef BUILD_ZEITGEIST
  priv->log = biji_zeitgeist_init ();
#endif /* BUILD_ZEITGEIST */

  /* Ensure cache directory for icons */
  filename = g_build_filename (g_get_user_cache_dir (),
                               g_get_application_name (),
                               NULL);
  cache = g_file_new_for_path (filename);
  g_free (filename);
  g_file_make_directory (cache, NULL, NULL);
  g_object_unref (cache);

  priv->local_provider = biji_local_provider_new (self, self->priv->location);
  _add_provider (self, priv->local_provider);
}


static void
biji_manager_class_init (BijiManagerClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = biji_manager_finalize;
  object_class->constructed = biji_manager_constructed;
  object_class->set_property = biji_manager_set_property;
  object_class->get_property = biji_manager_get_property;

  biji_manager_signals[BOOK_AMENDED] =
    g_signal_new ("changed", G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,                         /* offset & accumulator */
                  _biji_marshal_VOID__ENUM_ENUM_POINTER,
                  G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_INT, G_TYPE_POINTER);

  properties[PROP_LOCATION] =
    g_param_spec_object ("location",
                         "The manager location",
                         "The location where the notes are loaded and saved",
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);


  properties[PROP_COLOR] =
    g_param_spec_pointer ("color",
                         "Default color",
                         "Note manager default color for notes",
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);


  properties[PROP_ERROR] =
    g_param_spec_pointer ("error",
                          "Unrecoverable error",
                          "Note manager unrecoverable error",
                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, BIJI_MANAGER_PROPERTIES, properties);
  g_type_class_add_private (klass, sizeof (BijiManagerPrivate));
}


void
biji_manager_get_default_color (BijiManager *manager, GdkRGBA *color)
{
  g_return_if_fail (BIJI_IS_MANAGER (manager));

  color->red = manager->priv->color.red;
  color->blue = manager->priv->color.blue;
  color->green = manager->priv->color.green;
  color->alpha = manager->priv->color.alpha;
}


gboolean
biji_manager_remove_item (BijiManager *manager, BijiItem *item)
{
  g_return_val_if_fail (BIJI_IS_MANAGER (manager), FALSE);
  g_return_val_if_fail (BIJI_IS_ITEM      (item), FALSE);

  BijiItem *to_delete = NULL;
  const gchar *path;
  gboolean retval = FALSE;

  path = biji_item_get_uuid (item);
  to_delete = g_hash_table_lookup (manager->priv->items, path);

  if (to_delete)
  {
    /* Signal before doing anything here. So the note is still
     * fully available for signal receiver. */
    biji_manager_notify_changed (manager, BIJI_LIVING_ITEMS, BIJI_MANAGER_ITEM_TRASHED, to_delete);
    biji_item_trash (item);
    g_hash_table_insert (manager->priv->archives,
                         (gpointer) biji_item_get_uuid (item), item);
    g_hash_table_remove (manager->priv->items, path);

    retval = TRUE;
  }

  return retval;
}



GList *
biji_manager_get_items             (BijiManager         *manager,
                                    BijiItemsGroup       group)
{
  GList *list;


  switch (group)
  {
    case BIJI_LIVING_ITEMS:
      list = g_hash_table_get_values (manager->priv->items);
      break;

    case BIJI_ARCHIVED_ITEMS:
      list = g_hash_table_get_values (manager->priv->archives);
      break;

    default:
      list = NULL;
      g_warning ("invalid BijiItemsGroup:%i", group);
  }

  return list;
}


void
biji_manager_load_archives          (BijiManager        *manager)
{
  GList *l, *ll;

  l = g_hash_table_get_values (manager->priv->providers);
  for (ll=l; ll!= NULL; ll=ll->next)
    biji_provider_load_archives (BIJI_PROVIDER (ll->data));
  g_list_free (l);
}


static void
_delete_item (gpointer key,
              gpointer value,
              gpointer user_data)
{
  BijiItem *i;

  i = BIJI_ITEM (value);
  biji_item_delete (i);
}


void
biji_manager_empty_bin              (BijiManager        *manager)
{
  g_hash_table_foreach (manager->priv->archives, _delete_item, NULL);
}


BijiItem *
biji_manager_get_item_at_path (BijiManager *manager, const gchar *path)
{
  BijiItem *retval;

  retval = g_hash_table_lookup (manager->priv->items, (gconstpointer) path);

  if (retval == NULL)
    retval = g_hash_table_lookup (manager->priv->archives, (gconstpointer) path);

  return retval;
}


BijiManager *
biji_manager_new (GFile *location, GdkRGBA *color, GError **error)
{
  BijiManager *retval;

  retval = g_object_new (BIJI_TYPE_MANAGER,
                           "location", location,
                           "color", color,
                           "error", *error,
                           NULL);

  *error = retval->priv->error;
  return retval;
}


BijiNoteObj *
biji_note_get_new_from_file (BijiManager *manager, const gchar* path)
{
  BijiInfoSet  set;
  BijiNoteObj *ret;

  set.url = (gchar*) path;
  set.mtime = 0;
  set.title = NULL;
  set.content = NULL;

  ret = biji_local_note_new_from_info (manager->priv->local_provider, manager, &set);
  biji_lazy_deserialize (ret);

  return ret ;
}

gchar *
biji_manager_get_uuid (void)
{
  uuid_t unique;
  char out[40];

  uuid_generate (unique);
  uuid_unparse_lower (unique, out);
  return g_strdup_printf ("%s.note", out);
}

/* Common UUID skeleton for new notes. */
static BijiNoteObj *
get_note_skeleton (BijiManager *manager)
{
  BijiNoteObj *ret = NULL;
  gchar * folder, *name, *path;
  BijiInfoSet set;

  set.title = NULL;
  set.content = NULL;
  set.mtime = 0;
  folder = g_file_get_path (manager->priv->location);

  while (!ret)
  {
    name = biji_manager_get_uuid ();
    path = g_build_filename (folder, name, NULL);
    g_free (name);
    set.url = path;

    if (!g_hash_table_lookup (manager->priv->items, path))
      ret = biji_local_note_new_from_info (manager->priv->local_provider, manager, &set);

    g_free (path);
  }

  biji_note_obj_set_all_dates_now (ret);
  return ret;
}


/* 
 * TODO : move this to local provider.
 */
static BijiNoteObj *
biji_manager_local_note_new           (BijiManager *manager, gchar *str)
{
  BijiNoteObj *ret = get_note_skeleton (manager);

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
  }

  biji_note_obj_save_note (ret);
  biji_manager_add_item (manager, BIJI_ITEM (ret), BIJI_LIVING_ITEMS, TRUE);

  return ret;
}


/* Create the importer == switch depending on the uri.
 * That's all, the importer is responsible
 * for emiting the signal transfering the notes
 * And no need to _add_provider, it's a tmp provider. */
void
biji_manager_import_uri (BijiManager *manager,
                           gchar *target_provider_id,
                           gchar *uri)
{
  BijiProvider *ret;

  ret = biji_import_provider_new (manager, target_provider_id, uri);
  g_signal_connect (ret, "loaded", 
                    G_CALLBACK (on_provider_loaded_cb), manager);

}

/* 
 * Use "local" for a local note new
 * Use goa_account_get_id for goa
 */
BijiNoteObj *
biji_manager_note_new            (BijiManager *manager,
                                    gchar        *str,
                                    gchar        *provider_id)
{
  BijiProvider *provider;
  BijiNoteObj *retval;

  // If we move local_note_new to create_note for local provider
  // we won't need this stupid switch.

  if (provider_id == NULL ||
      g_strcmp0 (provider_id, "local") == 0)
    return biji_manager_local_note_new (manager, str);


  provider = g_hash_table_lookup (manager->priv->providers,
                                  provider_id);


  retval = BIJI_PROVIDER_GET_CLASS (provider)->create_new_note (provider, str);
  // do not save. up to the provider implementation to save it or not
  // at creation.
  biji_manager_add_item (manager, BIJI_ITEM (retval), BIJI_LIVING_ITEMS, TRUE);

  return retval;
}




BijiNoteObj *
biji_manager_note_new_full (BijiManager *manager,
                              gchar        *provider_id,
                              gchar        *suggested_path,
                              BijiInfoSet  *info,
                              gchar        *html,
                              GdkRGBA      *color)
{
  BijiProvider *provider;
  BijiNoteObj *retval;

  provider = g_hash_table_lookup (manager->priv->providers,
                                  provider_id);

  retval = BIJI_PROVIDER_GET_CLASS (provider)->create_note_full (provider,
                                                                 suggested_path,
                                                                 info,
                                                                 html,
                                                                 color);

  return retval;
}
