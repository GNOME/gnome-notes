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


#include "libbiji.h"
#include "biji-notebook.h"
#include "biji-error.h"

#include "provider/biji-import-provider.h"
#include "provider/biji-local-provider.h"
#include "provider/biji-memo-provider.h"
#include "provider/biji-own-cloud-provider.h"


struct _BijiManagerPrivate
{
  /* Notes & Collections.
   * Keep a direct pointer to local provider for convenience
   *
   * TODO: would be nice to have GHashTable onto providers
   * rather than one big central db here
   */

  GHashTable *items;
  GHashTable *archives;
  GHashTable *providers;
  BijiProvider *local_provider;

  /* Signals */
  gulong note_renamed ;

  GFile *location;
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
  BIJI_MANAGER_PROPERTIES
};


/* Signals */
enum {
  BOOK_AMENDED,
  BIJI_MANAGER_SIGNALS
};

static guint biji_manager_signals[BIJI_MANAGER_SIGNALS] = { 0 };
static GParamSpec *properties[BIJI_MANAGER_PROPERTIES] = { NULL, };
static void biji_manager_initable_iface_init (GInitableIface *iface);
static void biji_manager_async_initable_iface_init (GAsyncInitableIface *iface);

#define BIJI_MANAGER_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BIJI_TYPE_MANAGER, BijiManagerPrivate))

G_DEFINE_TYPE_WITH_CODE (BijiManager, biji_manager, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (G_TYPE_INITABLE, biji_manager_initable_iface_init)
                         G_IMPLEMENT_INTERFACE (G_TYPE_ASYNC_INITABLE, biji_manager_async_initable_iface_init))

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
  g_print ("sending the mass change\n");
}

static void
on_provider_abort_cb (BijiProvider *provider,
                      BijiManager  *self)
{
  const BijiProviderInfo *info;

  info = biji_provider_get_info (provider);
  g_hash_table_remove (self->priv->providers, (gpointer) info->unique_id);

  g_object_unref (G_OBJECT (provider));
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
  /* we can safely cast get_id from const to gpointer
   * since there is no key free func */

  const BijiProviderInfo *info;

  info = biji_provider_get_info (provider);
  g_hash_table_insert (self->priv->providers, (gpointer) info->unique_id, provider);

  g_signal_connect (provider, "loaded",
                    G_CALLBACK (on_provider_loaded_cb), self);
  g_signal_connect (provider, "abort",
                    G_CALLBACK (on_provider_abort_cb), self);
}

static void
load_goa_client (BijiManager *self,
                 GoaClient *client)
{
  GList *accounts, *l;
  GoaObject *object;
  GoaAccount *account;
  const gchar *type;
  BijiProvider *provider;

  accounts = goa_client_get_accounts (client);

  for (l = accounts; l != NULL; l = l->next)
  {
    object = GOA_OBJECT (l->data);
    account =  goa_object_peek_account (object);

    if (GOA_IS_ACCOUNT (account))
    {
      type = goa_account_get_provider_type (account);

      /* We do not need to store any object here.
       * account_get_id can be used to talk with libbji */
      if (g_strcmp0 (type, "owncloud") == 0)
      {
        g_message ("Loading account %s", goa_account_get_id (account));
        provider = biji_own_cloud_provider_new (self, object);
        _add_provider (self, provider);
      }
    }
  }

  g_list_free_full (accounts, g_object_unref);
}

static void
load_eds_registry (BijiManager *self,
                   ESourceRegistry *registry)
{
  GList *list, *l;
  BijiProvider *provider;

  list = e_source_registry_list_sources (registry, E_SOURCE_EXTENSION_MEMO_LIST);
  for (l = list; l != NULL; l = l->next)
  {
    provider = biji_memo_provider_new (self, l->data);
    _add_provider (self, provider);
  }

  g_list_free_full (list, g_object_unref);
}

static gboolean
biji_manager_initable_init (GInitable *initable,
                            GCancellable *cancellable,
                            GError **error)
{
  BijiManager *self = BIJI_MANAGER (initable);
  BijiManagerPrivate *priv = self->priv;
  GError *local_error = NULL;
  GoaClient *client;
  ESourceRegistry *registry;

  /* If tracker fails for some reason,
   * do not attempt anything */
  priv->connection = tracker_sparql_connection_get (NULL, &local_error);

  if (local_error)
  {
    g_warning ("Unable to connect to Tracker: %s", local_error->message);
    g_propagate_error (error, local_error);
    return FALSE;
  }

  client = goa_client_new_sync (NULL, &local_error);
  if (local_error)
  {
    g_warning ("Unable to connect to GOA: %s", local_error->message);
    g_propagate_error (error, local_error);
    return FALSE;
  }

  registry = e_source_registry_new_sync (NULL, &local_error);
  if (local_error)
  {
    g_object_unref (client);
    g_warning ("Unable to connect to EDS: %s", local_error->message);
    g_propagate_error (error, local_error);
    return FALSE;
  }

  priv->local_provider = biji_local_provider_new (self, self->priv->location);
  _add_provider (self, priv->local_provider);

  load_goa_client (self, client);
  load_eds_registry (self, registry);

  g_object_unref (client);
  g_object_unref (registry);

  return TRUE;
}

static void
biji_manager_initable_iface_init (GInitableIface *iface)
{
  iface->init = biji_manager_initable_init;
}

static void
biji_manager_async_initable_iface_init (GAsyncInitableIface *iface)
{
  /* Use default */
}

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
                                          NULL);
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
on_item_deleted_cb (BijiItem *item, BijiManager *self)
{
  BijiItem        *to_delete;
  const gchar     *path;
  GHashTable      *store;
  BijiItemsGroup   group;


  to_delete = NULL;
  path = biji_item_get_uuid (item);
  store = NULL;

  if ((to_delete = g_hash_table_lookup (self->priv->archives, path)))
  {
    store = self->priv->archives;
    group = BIJI_ARCHIVED_ITEMS;
  }
  else if ((to_delete = g_hash_table_lookup (self->priv->items, path)))
  {
    store = self->priv->items;
    group = BIJI_LIVING_ITEMS;
  }

  if (store == NULL)
    return;


  g_hash_table_remove (store, path);
  biji_manager_notify_changed (self, group,
                               BIJI_MANAGER_ITEM_DELETED, item);
}


/* Signal if item is known */
static void
on_item_trashed_cb (BijiItem *item, BijiManager *self)
{
  const gchar *path;

  path = biji_item_get_uuid (item);
  item = g_hash_table_lookup (self->priv->items, path);

  if (item == NULL)
    return;

  biji_manager_notify_changed (self, BIJI_LIVING_ITEMS, BIJI_MANAGER_ITEM_TRASHED, item);
  g_hash_table_insert (self->priv->archives,
                       (gpointer) biji_item_get_uuid (item), item);
  g_hash_table_remove (self->priv->items, path);
}


/*
 * old uuid : we need this because local provider uses
 * file name as uuid. Now this proves this is not right.
 *
 * save : in order to restore the note inside tracker
 *
 *
 * notify... BIJI_ARCHIVED_ITEM
 * well, works currently : we assume Archives change.
 * but we might double-ping as well
 * or improve the whole logic
 */
static void
on_item_restored_cb (BijiItem *item, gchar *old_uuid, BijiManager *manager)
{
  if (BIJI_IS_NOTE_OBJ (item))
    biji_note_obj_save_note (BIJI_NOTE_OBJ (item));

  g_hash_table_insert (manager->priv->items,
                       (gpointer) biji_item_get_uuid (item), item);
  g_hash_table_remove (manager->priv->archives, old_uuid);

  biji_manager_notify_changed (manager,
                               BIJI_ARCHIVED_ITEMS,
                               BIJI_MANAGER_ITEM_DELETED,
                               item);
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
    g_signal_connect (item, "trashed",
                      G_CALLBACK (on_item_trashed_cb), manager);
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
biji_manager_constructed (GObject *object)
{
  gchar *filename;
  GFile *cache;

  G_OBJECT_CLASS (biji_manager_parent_class)->constructed (object);

#ifdef BUILD_ZEITGEIST
  BIJI_MANAGER (object)->priv->log = biji_zeitgeist_init ();
#endif /* BUILD_ZEITGEIST */

  /* Ensure cache directory for icons */
  filename = g_build_filename (g_get_user_cache_dir (),
                               g_get_application_name (),
                               NULL);
  cache = g_file_new_for_path (filename);
  g_free (filename);
  g_file_make_directory (cache, NULL, NULL);
  g_object_unref (cache);
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


static void
_delete_item (gpointer data,
              gpointer user_data)
{
  biji_item_delete (BIJI_ITEM (data));
}


/* Do not g_list_free_full here.
 * We only unref items where deletion works */
void
biji_manager_empty_bin              (BijiManager        *self)
{
  GList *items;

  items = g_hash_table_get_values (self->priv->archives);
  g_list_foreach (items, _delete_item, NULL);
  g_list_free (items);
}


BijiItem *
biji_manager_get_item_at_path (BijiManager *manager, const gchar *path)
{
  BijiItem *retval;
  g_return_val_if_fail (BIJI_IS_MANAGER(manager), NULL);

  if (path == NULL)
    return NULL;

  retval = g_hash_table_lookup (manager->priv->items, (gconstpointer) path);

  if (retval == NULL)
    retval = g_hash_table_lookup (manager->priv->archives, (gconstpointer) path);

  return retval;
}


BijiManager *
biji_manager_new (GFile *location, GdkRGBA *color, GError **error)
{
  BijiManager *retval;

  retval = g_initable_new (BIJI_TYPE_MANAGER, NULL, error,
                           "location", location,
                           "color", color,
                           NULL);

  return retval;
}

void
biji_manager_new_async (GFile *location, GdkRGBA *color,
                        GAsyncReadyCallback callback, gpointer user_data)
{
  g_async_initable_new_async (BIJI_TYPE_MANAGER, G_PRIORITY_DEFAULT,
                              NULL,
                              callback, user_data,
                              "location", location,
                              "color", color,
                              NULL);
}

BijiManager *
biji_manager_new_finish (GAsyncResult *res,
                         GError **error)
{
  GObject *source_obj;
  GObject *object;

  source_obj = g_async_result_get_source_object (res);
  object = g_async_initable_new_finish (G_ASYNC_INITABLE (source_obj), res, error);
  g_object_unref (source_obj);

  return object != NULL ? BIJI_MANAGER (object) : NULL;
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
biji_manager_note_new            (BijiManager  *self,
                                  gchar        *str,
                                  gchar        *provider_id)
{
  BijiProvider *provider = NULL;
  BijiNoteObj *retval;


  if (provider_id != NULL)
    provider = g_hash_table_lookup (self->priv->providers,
                                    provider_id);

  if (provider == NULL)
    provider = self->priv->local_provider;

  retval = BIJI_PROVIDER_GET_CLASS (provider)->create_new_note (provider, str);

  if (retval)
    biji_manager_add_item (self, BIJI_ITEM (retval), BIJI_LIVING_ITEMS, TRUE);

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
