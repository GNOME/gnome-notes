/* -*- mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* biji-tracker.c
 *
 * Copyright (C) Pierre-Yves LUYTEN 2012, 2013 <py@luyten.fr>
 * Copyright 2021 Mohammed Sadiq <sadiq@sadiqpk.org>
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

#define G_LOG_DOMAIN "bjb-tracker"


#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdbool.h>
#include <tracker-sparql.h>

#include "biji-date-time.h"
#include "biji-tracker.h"

typedef enum
{
  BIJI_URN_COL,
  BIJI_TITLE_COL,
  BIJI_MTIME_COL,
  BIJI_CONTENT_COL,
  BIJI_CREATED_COL,
  BIJI_NO_COL
} BijiTrackerColumns;

struct _BijiTracker
{
  GObject parent_instance;

  BijiManager  *manager;

  GCancellable *cancellable;
  TrackerSparqlConnection *connection;
};

G_DEFINE_TYPE (BijiTracker, biji_tracker, G_TYPE_OBJECT)

static void
on_add_notebook_cb (GObject      *object,
                    GAsyncResult *result,
                    gpointer      user_data)
{
  BijiTracker *self;
  g_autoptr(GTask) task = user_data;
  g_autofree char *key = NULL;
  g_autofree char *val = NULL;
  GVariant *variant, *child;
  BjbItem *notebook = NULL;
  char *notebook_str, *urn = NULL;
  GError *error = NULL;

  g_assert (G_IS_TASK (task));

  self = g_task_get_source_object (task);
  g_assert (BIJI_IS_TRACKER (self));

  /* variant is aaa{ss} */
  /* https://gitlab.gnome.org/GNOME/tracker/-/blob/tracker-2.3/src/libtracker-data/tracker-sparql.c#L6693 */
  variant = tracker_sparql_connection_update_blank_finish (self->connection, result, &error);

  if (error)
    {
      g_warning ("Unable to create notebook: %s", error->message);
      g_task_return_error (task, error);
      return;
    }

  child = g_variant_get_child_value (variant, 0); /* variant is now aa{ss} */
  g_variant_unref (variant);
  variant = child;

  child = g_variant_get_child_value (variant, 0); /* variant is now a{ss} */
  g_variant_unref (variant);
  variant = child;

  child = g_variant_get_child_value (variant, 0); /* variant is now {ss} */
  g_variant_unref (variant);
  variant = child;

  child = g_variant_get_child_value (variant, 0);
  key = g_variant_dup_string (child, NULL); /* get the key of dictionary */
  g_variant_unref (child);

  child = g_variant_get_child_value (variant, 1); /* get the value of dict */
  val = g_variant_dup_string (child, NULL);
  g_variant_unref (child);

  g_variant_unref (variant);

  if (g_strcmp0 (key, "res") == 0)
    urn = val;

  if (urn)
    {
      notebook_str = g_task_get_task_data (task);
      notebook = bjb_notebook_new (urn, notebook_str,
                                   g_get_real_time () / G_USEC_PER_SEC);
      biji_manager_add_item (self->manager, notebook, BIJI_LIVING_ITEMS, true);
    }

  g_task_return_pointer (task, notebook, NULL);
}

static void
add_or_update_note (BijiTracker *self,
                    BijiInfoSet *info,
                    const char  *urn_uuid)
{
  g_autoptr(GDateTime) dt_created = NULL;
  g_autoptr(GDateTime) dt_mtime = NULL;
  g_autofree char *created = NULL;
  g_autofree char *mtime = NULL;
  g_autofree char *query = NULL;
  g_autofree char *content = NULL;
  const char *info_content;

  g_assert (BIJI_IS_TRACKER (self));
  g_assert (info);

  info_content = info->content;
  dt_created = g_date_time_new_from_unix_utc (info->created);
  dt_mtime = g_date_time_new_from_unix_utc (info->mtime);
  created = g_date_time_format_iso8601 (dt_created);
  mtime = g_date_time_format_iso8601 (dt_mtime);

  if (!info_content)
    info_content = "";
  content = g_strdelimit (tracker_sparql_escape_string (info_content), "\n'", ' ');

  if (urn_uuid)
    query = g_strdup_printf ("INSERT OR REPLACE { "
                             "<%s> a nfo:Note , nie:DataObject ; "
                             "nie:url '%s' ; "
                             "nie:contentLastModified '%s' ; "
                             "nie:contentCreated '%s' ; "
                             "nie:title '%s' ; "
                             "nie:plainTextContent '%s' ; "
                             "nie:dataSource '%s' ;"
                             "nie:generator 'Bijiben' . }",
                             urn_uuid,
                             info->url,
                             mtime,
                             created,
                             info->title,
                             content,
                             info->datasource_urn);
  else
    query = g_strconcat ("INSERT { "
                         "_:res a nfo:Note ; ",
                         "a nie:DataObject ; "
                         "nie:contentLastModified '", mtime, "' ; "
                         "nie:contentCreated '", created, "' ; "
                         "nie:title '", info->title, "' ; "
                         "nie:url  '", info->url, "' ; "
                         "nie:plainTextContent '", content, "' ; "
                         "nie:dataSource '", info->datasource_urn, "' ; "
                         "nie:generator 'Bijiben' }",
                         NULL);

  if (urn_uuid)
    tracker_sparql_connection_update_async (self->connection, query,
#if !HAVE_TRACKER3
                                            0,     // priority
#endif
                                            NULL, NULL, NULL);
  else
    tracker_sparql_connection_update_blank_async (self->connection, query,
#if !HAVE_TRACKER3
                                                  G_PRIORITY_DEFAULT,
#endif
                                                  NULL, NULL, NULL);
}

static void
on_save_note_cb (GObject      *object,
                 GAsyncResult *result,
                 gpointer      user_data)
{
  BijiTracker *self;
  g_autoptr(GTask) task = user_data;
  g_autoptr(TrackerSparqlCursor) cursor = NULL;
  g_autoptr(GError) error = NULL;
  g_autofree char *match = NULL;
  BijiInfoSet *info;

  g_assert (G_IS_TASK (task));

  self = g_task_get_source_object (task);
  g_assert (BIJI_IS_TRACKER (self));

  cursor = tracker_sparql_connection_query_finish (self->connection, result, &error);
  info = g_task_get_task_data (task);

  if (error)
    {
      g_warning ("Tracker: %s", error->message);
      return;
    };

  if (tracker_sparql_cursor_next (cursor, NULL, NULL))
    match = g_strdup (tracker_sparql_cursor_get_string (cursor, 0, NULL));

  add_or_update_note (self, info, match);
}

static void
on_get_list_async_cb (GObject      *object,
                      GAsyncResult *result,
                      gpointer      user_data)
{
  BijiTracker *self;
  g_autoptr(GTask) task = user_data;
  g_autoptr(TrackerSparqlCursor) cursor = NULL;
  GError *error = NULL;
  GList *items = NULL;

  g_assert (G_IS_TASK (task));

  self = g_task_get_source_object (task);
  g_assert (BIJI_IS_TRACKER (self));

  cursor = tracker_sparql_connection_query_finish (self->connection, result, &error);

  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  if (cursor)
    {
      gpointer item = NULL;
      const char *full_path;
      char *path;

      while (tracker_sparql_cursor_next (cursor, self->cancellable, NULL))
        {
          full_path = tracker_sparql_cursor_get_string (cursor, 0, NULL);

          if (g_str_has_prefix (full_path, "file://"))
            {
              GString *string;

              string = g_string_new (full_path);
              g_string_erase (string, 0, strlen ("file://"));
              path = g_string_free (string, false);
            }
          else
            path = g_strdup (full_path);

          item = biji_manager_get_item_at_path (self->manager, path);

          /* Sorting is done in another place */
          if (item)
            items = g_list_prepend (items, item);
        }
    }

  g_task_return_pointer (task, items, (GDestroyNotify)g_list_free);
}

static void
on_tracker_update_cb (GObject      *object,
                      GAsyncResult *result,
                      gpointer      user_data)
{
  BijiTracker *self;
  g_autoptr(GTask) task = user_data;
  GError *error = NULL;

  g_assert (G_IS_TASK (task));

  self = g_task_get_source_object (task);
  g_assert (BIJI_IS_TRACKER (self));

  tracker_sparql_connection_update_finish (self->connection, result, &error);

  if (error)
    {
      g_warning ("%s", error->message);
      g_task_return_error (task, error);
    }
  else
    g_task_return_boolean (task, true);
}

static void
on_get_notebooks_async_cb (GObject      *object,
                           GAsyncResult *result,
                           gpointer      user_data)
{
  BijiTracker *self;
  g_autoptr(GTask) task = user_data;
  g_autoptr(TrackerSparqlCursor) cursor = NULL;
  GHashTable *items = NULL;
  GError *error = NULL;

  g_assert (G_IS_TASK (task));

  self = g_task_get_source_object (task);
  g_assert (BIJI_IS_TRACKER (self));

  cursor = tracker_sparql_connection_query_finish (self->connection, result, &error);

  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  items = g_hash_table_new_full (g_str_hash, g_str_equal, NULL,
                                 (GDestroyNotify) biji_info_set_free);

  while (tracker_sparql_cursor_next (cursor, self->cancellable, NULL))
    {
      BijiInfoSet *set = biji_info_set_new ();

      set->tracker_urn = g_strdup (tracker_sparql_cursor_get_string (cursor, BIJI_URN_COL, NULL));
      set->title = g_strdup (tracker_sparql_cursor_get_string (cursor, BIJI_TITLE_COL, NULL));
      set->mtime = iso8601_to_gint64 (tracker_sparql_cursor_get_string (cursor, BIJI_MTIME_COL, NULL));

      g_hash_table_replace (items, set->tracker_urn, set);
    }

  g_task_return_pointer (task, items, (GDestroyNotify)g_hash_table_unref);
}

static void
biji_tracker_finalize (GObject *object)
{
  BijiTracker *self = (BijiTracker *)object;

  g_cancellable_cancel (self->cancellable);
  g_clear_object (&self->cancellable);
  g_clear_object (&self->connection);

  G_OBJECT_CLASS (biji_tracker_parent_class)->finalize (object);
}

static void
biji_tracker_class_init (BijiTrackerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = biji_tracker_finalize;
}

static void
biji_tracker_init (BijiTracker *self)
{
  g_autofree char *filename = NULL;
  g_autoptr(GFile) location = NULL;
  g_autoptr(GError) error = NULL;

  self->cancellable = g_cancellable_new ();

#ifdef TRACKER_PRIVATE_STORE
  filename = g_build_filename (g_get_user_cache_dir (),
                               g_get_application_name (),
#if HAVE_TRACKER3
                               "tracker3",
#else
                               "tracker",
#endif /* HAVE_TRACKER3 */
                               NULL);

  location = g_file_new_for_path (filename);

  /* If tracker fails for some reason,
   * do not attempt anything */
#if HAVE_TRACKER3
  self->connection = tracker_sparql_connection_new (TRACKER_SPARQL_CONNECTION_FLAGS_NONE,
                                                    location,
                                                    tracker_sparql_get_ontology_nepomuk (),
                                                    self->cancellable,
                                                    &error);
#else
  self->connection = tracker_sparql_connection_local_new (TRACKER_SPARQL_CONNECTION_FLAGS_NONE,
                                                          location,
                                                          NULL, NULL,
                                                          self->cancellable,
                                                          &error);
#endif /* HAVE_TRACKER3 */

#else
  self->connection = tracker_sparql_connection_get (self->cancellable, &error);
#endif /* TRACKER_PRIVATE_STORE */

  if (error)
    g_warning ("Error: %s", error->message);
}

BijiTracker *
biji_tracker_new (BijiManager *manager)
{
  BijiTracker *self;

  g_return_val_if_fail (BIJI_IS_MANAGER (manager), NULL);

  self = g_object_new (BIJI_TYPE_TRACKER, NULL);
  self->manager = manager;
  g_object_add_weak_pointer (G_OBJECT (self->manager),
                             (gpointer *)&self->manager);

  return self;
}

gboolean
biji_tracker_is_available (BijiTracker *self)
{
  g_return_val_if_fail (BIJI_IS_TRACKER (self), false);

  return self->connection != NULL;
}

TrackerSparqlConnection *
biji_tracker_get_connection (BijiTracker *self)
{
  g_return_val_if_fail (BIJI_IS_TRACKER (self), NULL);

  return self->connection;
}

void
biji_tracker_add_notebook_async (BijiTracker         *self,
                                 const char          *notebook,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data)
{
  g_autoptr(GDateTime) date_time = NULL;
  g_autofree char *query = NULL;
  g_autofree char *time = NULL;
  GTask *task;

  g_return_if_fail (BIJI_IS_TRACKER (self));
  g_return_if_fail (callback);

  date_time = g_date_time_new_now_utc ();
  time = g_date_time_format_iso8601 (date_time);
  query = g_strdup_printf ("INSERT { "
                           "_:res a nfo:DataContainer ; a nie:DataObject ; "
                           "nie:contentLastModified '%s' ; "
                           "nie:title '%s' ; "
                           "nie:generator 'Bijiben' }",
                           time, notebook);

  task = g_task_new (self, NULL, callback, user_data);
  g_task_set_task_data (task, g_strdup (notebook), g_free);

  tracker_sparql_connection_update_blank_async (self->connection, query,
#if !HAVE_TRACKER3
                                                G_PRIORITY_DEFAULT,
#endif
                                                NULL,
                                                on_add_notebook_cb, task);
}

BjbItem *
biji_tracker_add_notebook_finish (BijiTracker  *self,
                                  GAsyncResult *result,
                                  GError       **error)
{
  g_return_val_if_fail (BIJI_IS_TRACKER (self), false);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), false);

  return g_task_propagate_pointer (G_TASK (result), error);
}

void
biji_tracker_remove_notebook (BijiTracker *self,
                              const char  *notebook_urn)
{
  g_autofree char *query = NULL;

  g_return_if_fail (BIJI_IS_TRACKER (self));

  query = g_strdup_printf ("DELETE {'%s' a nfo:DataContainer}", notebook_urn);
  tracker_sparql_connection_update_async (self->connection, query,
#if !HAVE_TRACKER3
                                          0,     // priority
#endif
                                          NULL, NULL, NULL);
}

void
biji_tracker_get_notes_async (BijiTracker         *self,
                              BijiItemsGroup       group,
                              const char          *needle,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
  g_autofree char *query = NULL;
  g_autofree char *str = NULL;
  GTask *task;

  g_return_if_fail (BIJI_IS_TRACKER (self));
  g_return_if_fail (needle && *needle);
  g_return_if_fail (callback);

  str = g_utf8_strdown (needle, -1);
  query = g_strconcat ("SELECT tracker:coalesce (?url, ?urn) WHERE "
                       "{"
                       "  {  ?urn a nfo:Note"
                       "    .?urn nie:title ?title"
                       "    .?urn nie:plainTextContent ?content"
                       "    .?urn nie:url ?url"
                       "    .?urn nie:generator 'Bijiben'"
                       "    .FILTER ("
                       "    fn:contains (fn:lower-case (?content), '", str, "' ) || "
                       "    fn:contains (fn:lower-case (?title)  , '", str, "'))} "
                       "UNION"
                       "  {  ?urn a nfo:DataContainer"
                       "    .?urn nie:title ?title"
                       "    .?urn nie:generator 'Bijiben'"
                       "    .FILTER ("
                       "    fn:contains (fn:lower-case (?title), '", str, "'))}"
                       "}",
                       NULL);

  task = g_task_new (self, self->cancellable, callback, user_data);
  tracker_sparql_connection_query_async (self->connection, query, self->cancellable,
                                         on_get_list_async_cb, task);
}

GList *
biji_tracker_get_notes_finish (BijiTracker   *self,
                               GAsyncResult  *result,
                               GError       **error)
{
  g_return_val_if_fail (BIJI_IS_TRACKER (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

void
biji_tracker_get_notebooks_async (BijiTracker         *self,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  const char *query;
  GTask *task;

  g_return_if_fail (BIJI_IS_TRACKER (self));
  g_return_if_fail (callback);

  query = "SELECT ?c ?title ?mtime "
    "WHERE { ?c a nfo:DataContainer ;"
    "nie:title ?title ; "
    "nie:contentLastModified ?mtime ;"
    "nie:generator 'Bijiben'}";

  task = g_task_new (self, self->cancellable, callback, user_data);

  tracker_sparql_connection_query_async (self->connection, query,
                                         self->cancellable,
                                         on_get_notebooks_async_cb, task);
}

GHashTable *
biji_tracker_get_notebooks_finish (BijiTracker   *self,
                                   GAsyncResult  *result,
                                   GError       **error)
{
  g_return_val_if_fail (BIJI_IS_TRACKER (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

void
biji_tracker_remove_note_notebook_async (BijiTracker         *self,
                                         BijiNoteObj         *note,
                                         BjbItem             *notebook,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  g_autofree char *url = NULL;
  g_autofree char *query = NULL;
  GTask *task;

  g_return_if_fail (BIJI_IS_TRACKER (self));
  g_return_if_fail (BIJI_IS_NOTE_OBJ (note));
  g_return_if_fail (BJB_IS_NOTEBOOK (notebook));
  g_return_if_fail (callback);

  url = g_strdup_printf ("file://%s", bjb_item_get_uid (BJB_ITEM (note)));
  query = g_strdup_printf ("DELETE {'%s' nie:isPartOf '%s'}",
                           bjb_item_get_uid (notebook), url);

  task = g_task_new (self, NULL, callback, user_data);

  tracker_sparql_connection_update_async (self->connection, query,
#if !HAVE_TRACKER3
                                          0,     // priority
#endif
                                          NULL,
                                          on_tracker_update_cb, task);
}

gboolean
biji_tracker_remove_note_notebook_finish (BijiTracker   *self,
                                          GAsyncResult  *result,
                                          GError       **error)
{
  g_return_val_if_fail (BIJI_IS_TRACKER (self), false);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), false);

  return g_task_propagate_boolean (G_TASK (result), error);
}

void
biji_tracker_add_note_to_notebook_async (BijiTracker         *self,
                                         BijiNoteObj         *note,
                                         const char          *notebook,
                                         GAsyncReadyCallback  callback,
                                         gpointer             user_data)
{
  g_autofree char *query = NULL;
  g_autofree char *url = NULL;
  GTask *task;

  g_return_if_fail (BIJI_IS_TRACKER (self));
  g_return_if_fail (BIJI_IS_NOTE_OBJ (note));
  g_return_if_fail (notebook && *notebook);
  g_return_if_fail (callback);

  url = g_strdup_printf ("file://%s", bjb_item_get_uid (BJB_ITEM (note)));
  query = g_strdup_printf ("INSERT {?urn nie:isPartOf '%s'} "
                           "WHERE {?urn a nfo:DataContainer; nie:title '%s'; nie:generator 'Bijiben'}",
                           url, notebook);

  task = g_task_new (self, NULL, callback, user_data);

  tracker_sparql_connection_update_async (self->connection, query,
#if !HAVE_TRACKER3
                                          0,     // priority
#endif
                                          NULL,
                                          on_tracker_update_cb, task);
}

gboolean
biji_tracker_add_note_to_notebook_finish (BijiTracker   *self,
                                          GAsyncResult  *result,
                                          GError       **error)
{
  g_return_val_if_fail (BIJI_IS_TRACKER (self), false);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), false);

  return g_task_propagate_boolean (G_TASK (result), error);
}

void
biji_tracker_get_notes_with_notebook_async (BijiTracker         *self,
                                            const char          *notebook,
                                            GAsyncReadyCallback  callback,
                                            gpointer             user_data)
{
  g_autofree char *query = NULL;
  GTask *task;

  g_return_if_fail (BIJI_IS_TRACKER (self));
  g_return_if_fail (callback);

  query = g_strdup_printf ("SELECT ?s WHERE {?c nie:isPartOf ?s; nie:title '%s'}", notebook);
  task = g_task_new (self, self->cancellable, callback, user_data);

  tracker_sparql_connection_query_async (self->connection, query, self->cancellable,
                                         on_get_list_async_cb, task);
}

GList *
biji_tracker_get_notes_with_notebook_finish (BijiTracker   *self,
                                             GAsyncResult  *result,
                                             GError       **error)
{
  g_return_val_if_fail (BIJI_IS_TRACKER (self), NULL);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), NULL);

  return g_task_propagate_pointer (G_TASK (result), error);
}

void
biji_tracker_delete_note (BijiTracker *self,
                          BijiNoteObj *note)
{
  g_autofree char *query = NULL;

  g_return_if_fail (BIJI_IS_TRACKER (self));
  g_return_if_fail (BIJI_IS_NOTE_OBJ (note));

  query = g_strdup_printf ("DELETE { <%s> a rdfs:Resource }",
                           bjb_item_get_uid (BJB_ITEM (note)));

  tracker_sparql_connection_update_async (self->connection, query,
#if !HAVE_TRACKER3
                                          0,     // priority
#endif
                                          NULL, NULL, NULL);
}

void
biji_tracker_trash_resource (BijiTracker *self,
                             const char  *tracker_urn)
{
  g_autofree char *query = NULL;

  g_return_if_fail (BIJI_IS_TRACKER (self));
  g_return_if_fail (tracker_urn);

  query = g_strdup_printf ("DELETE { <%s> a rdfs:Resource }", tracker_urn);

  tracker_sparql_connection_update_async (self->connection, query,
#if !HAVE_TRACKER3
                                          0,     // priority
#endif
                                          NULL, NULL, NULL);
}

void
biji_tracker_save_note (BijiTracker *self,
                        BijiInfoSet *info)
{
  g_autofree char *query = NULL;
  GTask *task;

  g_return_if_fail (BIJI_IS_TRACKER (self));

  query = g_strconcat ("SELECT ?urn ?time WHERE { "
                       "?urn a nfo:Note ; "
                       "nie:title ?title ; "
                       "nie:contentLastModified ?time ; "
                       "nie:url '", info->url, "' }",
                       NULL);

  task = g_task_new (self, NULL, NULL, NULL);
  g_task_set_task_data (task, info, NULL);

  tracker_sparql_connection_query_async (self->connection, query, NULL,
                                         on_save_note_cb, task);
}
