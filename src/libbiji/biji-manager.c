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

#define G_LOG_DOMAIN "bjb-manager"

#include <gtk/gtk.h>

#include "config.h"
#include "libbiji.h"
#include "biji-notebook.h"

#include "provider/biji-import-provider.h"
#include "provider/biji-local-provider.h"
#include "provider/biji-memo-provider.h"
#include "provider/biji-nextcloud-provider.h"
#include "provider/biji-provider.h"


struct _BijiManager
{
  GObject parent_instance;

  /* Notes & Collections.
   * Keep a direct pointer to local provider for convenience
   *
   * TODO: would be nice to have GHashTable onto providers
   * rather than one big central db here
   */

  GListStore *notebooks;
  GHashTable *notes;
  GHashTable *archives;
  GListStore *providers;
  BijiProvider *local_provider;

  /* The active provider */
  BijiProvider *provider;

  /* Signals */
  gulong note_renamed ;

  GFile *location;
  BijiTracker *tracker;

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

G_DEFINE_TYPE (BijiManager, biji_manager, G_TYPE_OBJECT)

static void
on_provider_loaded_cb (BijiProvider   *provider,
                       GList          *items,
                       BijiItemsGroup  group,
                       BijiManager    *manager)
{
  GList *l;

  switch (group)
  {
    case BIJI_LIVING_ITEMS:
      for (l=items; l!=NULL; l=l->next)
      {
        if (BIJI_IS_NOTEBOOK (l->data) || BIJI_IS_NOTE_OBJ (l->data))
          biji_manager_add_item (manager, l->data, BIJI_LIVING_ITEMS, FALSE);
      }
      break;

    case BIJI_ARCHIVED_ITEMS:
      for (l=items; l!= NULL; l=l->next)
      {
        if (BIJI_IS_NOTEBOOK (l->data) || BIJI_IS_NOTE_OBJ (l->data))
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

static BijiProvider *
find_provider_with_id (GListModel *providers,
                       const char *provider_id)
{
  guint n_items;

  if (!providers || !provider_id)
    return NULL;

  n_items = g_list_model_get_n_items (providers);

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(BijiProvider) provider = NULL;
      const BijiProviderInfo *info;

      provider = g_list_model_get_item (providers, i);
      info = biji_provider_get_info (provider);

      if (g_strcmp0 (info->unique_id, provider_id) == 0)
        return provider;
    }

  return NULL;
}

/*
 * It should be the right place
 * to stock somehow providers list
 * in order to handle properly manager__note_new ()
 *
 */
static void
_add_provider (BijiManager  *self,
               BijiProvider *provider)
{
  g_list_store_append (self->providers, provider);

  g_signal_connect (provider, "loaded",
                    G_CALLBACK (on_provider_loaded_cb), self);
  g_signal_connect (provider, "abort",
                    G_CALLBACK (g_object_unref), self);
}

static void
load_local_provider (BijiManager *self)
{
  self->local_provider = biji_local_provider_new (self, self->location);
  _add_provider (self, self->local_provider);
}

static gboolean
load_goa_provider (BijiManager *self,
                   GError     **error)
{
  g_autoptr(GoaClient) client;
  GList *accounts, *l;
  GoaObject *object;
  GoaAccount *account;
  const gchar *type;
  BijiProvider *provider;
  g_autoptr(GError) local_error = NULL;

  client = goa_client_new_sync (NULL, &local_error);
  if (local_error)
  {
    g_warning ("Unable to connect to GOA: %s", local_error->message);
    g_propagate_error (error, local_error);
    return FALSE;
  }

  accounts = goa_client_get_accounts (client);

  for (l = accounts; l != NULL; l = l->next)
  {
    object = GOA_OBJECT (l->data);
    account = goa_object_peek_account (object);

    if (GOA_IS_ACCOUNT (account) &&
        !goa_account_get_documents_disabled (account))
    {
      type = goa_account_get_provider_type (account);

      /* We do not need to store any object here.
       * account_get_id can be used to talk with libbji */
      if (g_strcmp0 (type, "owncloud") == 0)
      {
        g_message ("Loading account %s", goa_account_get_id (account));
        provider = biji_nextcloud_provider_new (self, object);
        _add_provider (self, provider);
      }
    }
  }

  g_list_free_full (accounts, g_object_unref);

  return TRUE;
}

static gboolean
load_eds_provider (BijiManager *self,
                   GError     **error)
{
  g_autoptr(ESourceRegistry) registry;
  GList *list, *l;
  BijiProvider *provider;
  g_autoptr(GError) local_error = NULL;

  registry = e_source_registry_new_sync (NULL, &local_error);
  if (local_error)
  {
    g_warning ("Unable to connect to EDS: %s", local_error->message);
    g_propagate_error (error, local_error);
    return FALSE;
  }

  list = e_source_registry_list_sources (registry, E_SOURCE_EXTENSION_MEMO_LIST);
  for (l = list; l != NULL; l = l->next)
  {
    provider = biji_memo_provider_new (self, l->data);
    _add_provider (self, provider);
  }

  g_list_free_full (list, g_object_unref);

  return TRUE;
}

static void
biji_manager_init (BijiManager *self)
{
  self->notebooks = g_list_store_new (BIJI_TYPE_NOTEBOOK);
  self->providers = g_list_store_new (BIJI_TYPE_PROVIDER);

  /* Item path is key for table */
  self->notes = g_hash_table_new_full (g_str_hash,
                                       g_str_equal,
                                       NULL,
                                       NULL);

  self->archives = g_hash_table_new_full (g_str_hash,
                                          g_str_equal,
                                          NULL,
                                          NULL);
  self->tracker = biji_tracker_new (self);
}

TrackerSparqlConnection *
biji_manager_get_tracker_connection (BijiManager *self)
{
  return biji_tracker_get_connection (self->tracker);
}

gpointer
biji_manager_get_tracker (BijiManager *self)
{
  g_return_val_if_fail (BIJI_IS_MANAGER (self), NULL);

  return self->tracker;
}

void
biji_manager_set_provider (BijiManager *self,
                           const gchar *provider_id)
{
  g_hash_table_remove_all (self->notes);
  g_hash_table_remove_all (self->archives);

  self->provider = find_provider_with_id (G_LIST_MODEL (self->providers), provider_id);
  if (self->provider)
    biji_provider_load_items (self->provider);
  else
    g_warning ("Could not find provider with ID: %s", provider_id);
}

GListModel *
biji_manager_get_providers (BijiManager *self)
{
  g_return_val_if_fail (BIJI_IS_MANAGER (self), NULL);

  return G_LIST_MODEL (self->providers);
}

static void
biji_manager_finalize (GObject *object)
{
  BijiManager *self = BIJI_MANAGER (object);

  g_clear_object (&self->notebooks);
  g_clear_object (&self->location);
  g_clear_object (&self->tracker);
  g_hash_table_destroy (self->notes);
  g_hash_table_destroy (self->archives);
  g_clear_object (&self->providers);

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
      self->location = g_value_dup_object (value);
      break;

    case PROP_COLOR:
      color = g_value_get_pointer (value);
      self->color.red = color->red;
      self->color.blue = color->blue;
      self->color.green = color->green;
      self->color.alpha = color->alpha;
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
      g_value_set_object (value, self->location);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
title_is_unique (BijiManager *self, gchar *title)
{
  gboolean is_unique = TRUE;
  guint i, n_notebooks;
  GList *items, *l;

  items = g_hash_table_get_values (self->notes);

  for ( l=items ; l != NULL ; l = l->next)
  {
    const char *item_title = NULL;

    if (BIJI_IS_NOTE_OBJ (l->data))
      item_title = biji_note_obj_get_title (l->data);

    if (g_strcmp0 (item_title, title) == 0)
    {
     is_unique = FALSE;
     break;
    }
  }

  n_notebooks = g_list_model_get_n_items (G_LIST_MODEL (self->notebooks));
  for (i = 0; i < n_notebooks; i++)
    {
      g_autoptr(BijiNotebook) item = NULL;

      item = g_list_model_get_item (G_LIST_MODEL (self->notebooks), i);
      if (g_strcmp0 (biji_notebook_get_title (item), title) == 0)
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
  gchar *new_title;
  gint suffix = 2;

  if (!manager)
    return g_strdup (title);

  if (!title)
    title = "";

  new_title = g_strdup (title);

  while (!title_is_unique (manager, new_title))
  {
    g_free (new_title);
    new_title = g_strdup_printf("%s (%i)", title, suffix);
    suffix++;
  }

  return new_title;
}


void
biji_manager_notify_changed (BijiManager           *manager,
                             BijiItemsGroup         group,
                             BijiManagerChangeFlag  flag,
                             gpointer               item)
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
on_item_deleted_cb (BijiNoteObj *item,
                    BijiManager *self)
{
  BijiNoteObj     *to_delete;
  const gchar     *path;
  GHashTable      *store;
  BijiItemsGroup   group;


  to_delete = NULL;
  path = biji_note_obj_get_uuid (item);
  store = NULL;

  if ((to_delete = g_hash_table_lookup (self->archives, path)))
  {
    store = self->archives;
    group = BIJI_ARCHIVED_ITEMS;
  }
  else if ((to_delete = g_hash_table_lookup (self->notes, path)))
  {
    store = self->notes;
    group = BIJI_LIVING_ITEMS;
  }

  if (store == NULL)
    return;


  g_hash_table_remove (store, path);
  biji_manager_notify_changed (self, group, BIJI_MANAGER_ITEM_DELETED, item);
}


/* Signal if item is known */
static void
on_item_trashed_cb (BijiNoteObj *item,
                    BijiManager *self)
{
  const gchar *path;

  path = biji_note_obj_get_uuid (item);
  item = g_hash_table_lookup (self->notes, path);

  if (item == NULL)
    return;

  biji_manager_notify_changed (self, BIJI_LIVING_ITEMS, BIJI_MANAGER_ITEM_TRASHED, item);
  g_hash_table_insert (self->archives, (gpointer) path, item);
  g_hash_table_remove (self->notes, path);
}


static void
manager_on_note_changed_cb (BijiNoteObj *note, BijiManager *manager)
{
  biji_manager_notify_changed (manager,
                               BIJI_LIVING_ITEMS,
                               BIJI_MANAGER_NOTE_AMENDED,
                               note);
}

static void
manager_on_item_icon_changed_cb (BijiNoteObj *note, BijiManager *manager)
{
  biji_manager_notify_changed (manager,
                               BIJI_LIVING_ITEMS,
                               BIJI_MANAGER_ITEM_ICON_CHANGED,
                               note);
}

static int
compare_notebook (gconstpointer a,
                  gconstpointer b,
                  gpointer      user_data)
{
  g_autofree char *up_a = NULL;
  g_autofree char *up_b = NULL;
  BijiNotebook *item_a = (BijiNotebook *) a;
  BijiNotebook *item_b = (BijiNotebook *) b;

  up_a = g_utf8_casefold (biji_notebook_get_title (item_a), -1);
  up_b = g_utf8_casefold (biji_notebook_get_title (item_b), -1);

  return g_strcmp0 (up_a, up_b);
}

gboolean
biji_manager_add_item (BijiManager    *manager,
                       gpointer        item,
                       BijiItemsGroup  group,
                       gboolean        notify)
{
  const gchar *uid;
  gboolean retval;

  g_return_val_if_fail (BIJI_IS_MANAGER (manager), FALSE);
  g_return_val_if_fail (BIJI_IS_NOTEBOOK (item) || BIJI_IS_NOTE_OBJ (item), FALSE);

  retval = TRUE;
  if (BIJI_IS_NOTEBOOK (item))
    uid = biji_notebook_get_uuid (BIJI_NOTEBOOK (item));
  else
    uid = biji_note_obj_get_uuid (BIJI_NOTE_OBJ (item));

  /* Check if item is not already there */
  if (uid != NULL)
  {
    if (group == BIJI_LIVING_ITEMS &&
        (g_hash_table_lookup (manager->notes, uid) ||
         biji_manager_find_notebook (manager, uid)))
      retval = FALSE;

    if (group == BIJI_ARCHIVED_ITEMS &&
        g_hash_table_lookup (manager->archives, uid))
      retval = FALSE;
  }


  /* Fail because UUID is null */
  else
    retval = FALSE;

  if (retval == TRUE)
  {
    /* Add the item*/
    if (group == BIJI_LIVING_ITEMS)
      {
        if (BIJI_IS_NOTE_OBJ (item))
          g_hash_table_insert (manager->notes, (gpointer) uid, item);
        else if (BIJI_IS_NOTEBOOK (item))
          if (uid && !biji_manager_find_notebook (manager, uid))
            g_list_store_insert_sorted (manager->notebooks, item,
                                        compare_notebook, NULL);
      }
    else if (group == BIJI_ARCHIVED_ITEMS)
      g_hash_table_insert (manager->archives, (gpointer) uid, item);

    /* Connect */
    if (BIJI_IS_NOTE_OBJ (item))
    {
      g_signal_connect (item, "deleted", G_CALLBACK (on_item_deleted_cb), manager);
      g_signal_connect (item, "trashed", G_CALLBACK (on_item_trashed_cb), manager);
      g_signal_connect (item, "changed", G_CALLBACK (manager_on_note_changed_cb), manager);
      g_signal_connect (item, "renamed", G_CALLBACK (manager_on_note_changed_cb), manager);
      g_signal_connect (item, "color-changed", G_CALLBACK (manager_on_item_icon_changed_cb), manager);
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
}


void
biji_manager_get_default_color (BijiManager *self, GdkRGBA *color)
{
  g_return_if_fail (BIJI_IS_MANAGER (self));

  color->red = self->color.red;
  color->blue = self->color.blue;
  color->green = self->color.green;
  color->alpha = self->color.alpha;
}



GList *
biji_manager_get_notes (BijiManager    *self,
                        BijiItemsGroup  group)
{
  GList *list;


  switch (group)
  {
    case BIJI_LIVING_ITEMS:
      list = g_hash_table_get_values (self->notes);
      break;

    case BIJI_ARCHIVED_ITEMS:
      list = g_hash_table_get_values (self->archives);
      break;

    default:
      list = NULL;
      g_warning ("invalid BijiItemsGroup:%i", group);
  }

  return list;
}

GListModel *
biji_manager_get_notebooks (BijiManager *self)
{
  g_return_val_if_fail (BIJI_IS_MANAGER (self), NULL);

  return G_LIST_MODEL (self->notebooks);
}

BijiNotebook *
biji_manager_find_notebook (BijiManager *self,
                            const char  *uuid)
{
  GListModel *notebooks;
  guint n_items;

  g_return_val_if_fail (BIJI_IS_MANAGER (self), NULL);
  g_return_val_if_fail (uuid && *uuid, NULL);

  notebooks = biji_manager_get_notebooks (self);
  n_items = g_list_model_get_n_items (notebooks);

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(BijiNotebook) notebook = NULL;
      const char *item_uuid;

      notebook = g_list_model_get_item (notebooks, i);

      item_uuid = biji_notebook_get_uuid (notebook);

      if (g_strcmp0 (uuid, item_uuid) == 0)
        return notebook;
    }

  return NULL;
}

gpointer
biji_manager_get_item_at_path (BijiManager *self, const gchar *path)
{
  gpointer retval;

  g_return_val_if_fail (BIJI_IS_MANAGER (self), NULL);

  if (path == NULL)
    return NULL;

  retval = g_hash_table_lookup (self->notes, (gconstpointer) path);

  if (retval == NULL)
    retval = g_hash_table_lookup (self->archives, (gconstpointer) path);

  return retval;
}

BijiManager *
biji_manager_new (GFile   *location,
                  GdkRGBA *color)
{
  return g_object_new (BIJI_TYPE_MANAGER,
                       "location", location,
                       "color", color,
                       NULL);
}

static void
thread_run_cb (GObject      *object,
               GAsyncResult *result,
               gpointer      user_data)
{
  BijiManager *self = user_data;
  g_autoptr(GTask) task = NULL;
  GError *error = NULL;

  g_assert (BIJI_IS_MANAGER (self));
  g_assert (G_IS_TASK (result));

  task = g_task_get_task_data (G_TASK (result));
  g_object_ref (task);

  g_task_propagate_boolean (G_TASK (result), &error);

  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  g_task_return_boolean (task, TRUE);
}

static void
load_providers (GTask        *task,
                gpointer      source_object,
                gpointer      task_data,
                GCancellable *cancellable)
{
  BijiManager *self = source_object;
  GError *error = NULL;

  if (!biji_tracker_is_available (self->tracker))
    {
      g_task_return_new_error (task, G_IO_ERROR, G_IO_ERROR_FAILED,
                               "Connecting to tracker failed");
      return;
    }

  load_local_provider(self);

  load_goa_provider (self, &error);
  if (error)
    {
      g_task_return_error (task, error);
      return;
    }
  
  load_eds_provider (self, &error);
  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  g_task_return_boolean (task, TRUE);

  return;
}

static void
on_tracker_get_notebooks_cb (GObject      *object,
                             GAsyncResult *result,
                             gpointer      user_data)
{
  g_autoptr(BijiManager) self = user_data;
  g_autoptr(GHashTable) notebooks = NULL;
  g_autoptr(GList) values = NULL;

  g_assert (BIJI_IS_MANAGER (self));

  notebooks = biji_tracker_get_notebooks_finish (BIJI_TRACKER (object), result, NULL);

  if (!notebooks)
    return;

  values = g_hash_table_get_values (notebooks);

  for (GList *value = values; value; value = value->next)
    {
      g_autoptr(BijiNotebook) notebook = NULL;
      BijiInfoSet *set = value->data;

      notebook = biji_notebook_new (G_OBJECT (self), set->tracker_urn,
                                    set->title, set->mtime);
      g_list_store_insert_sorted (self->notebooks, notebook,
                                  compare_notebook, NULL);
    }
}

void
biji_manager_load_providers_async (BijiManager         *self,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  g_autoptr(GTask) thread_task = NULL;
  GTask *task;

  g_return_if_fail (BIJI_IS_MANAGER (self));

  task = g_task_new (self, NULL, callback, user_data);
  thread_task = g_task_new (self, NULL, thread_run_cb, self);
  g_task_set_task_data (thread_task, task, g_object_unref);

  g_task_run_in_thread (thread_task, load_providers);
  biji_tracker_get_notebooks_async (self->tracker,
                                    on_tracker_get_notebooks_cb,
                                    g_object_ref (self));
}

gboolean
biji_manager_load_providers_finish (BijiManager   *self,
                                    GAsyncResult  *result,
                                    GError       **error)
{
  g_return_val_if_fail (BIJI_IS_MANAGER (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  return g_task_propagate_boolean (G_TASK (result), error);
}

/* Create the importer == switch depending on the uri.
 * That's all, the importer is responsible
 * for emitting the signal transferring the notes
 * And no need to _add_provider, it's a tmp provider. */
void
biji_manager_import_uri (BijiManager *manager,
                         const gchar *target_provider_id,
                         const gchar *uri)
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
                                  const gchar  *str,
                                  const gchar  *provider_id)
{
  BijiProvider *provider = NULL;
  BijiNoteObj *retval;

  provider = find_provider_with_id (G_LIST_MODEL (self->providers), provider_id);

  if (provider == NULL)
    provider = self->local_provider;

  retval = BIJI_PROVIDER_GET_CLASS (provider)->create_new_note (provider, str);

  if (retval)
    biji_manager_add_item (self, retval, BIJI_LIVING_ITEMS, TRUE);

  return retval;
}




BijiNoteObj *
biji_manager_note_new_full (BijiManager   *self,
                            const gchar   *provider_id,
                            const gchar   *suggested_path,
                            BijiInfoSet   *info,
                            const gchar   *html,
                            const GdkRGBA *color)
{
  BijiProvider *provider;
  BijiNoteObj *retval;

  provider = find_provider_with_id (G_LIST_MODEL (self->providers), provider_id);

  retval = BIJI_PROVIDER_GET_CLASS (provider)->create_note_full (provider,
                                                                 suggested_path,
                                                                 info,
                                                                 html,
                                                                 color);

  return retval;
}
