/* biji-tracker.c
 * Copyright (C) Pierre-Yves LUYTEN 2012, 2013 <py@luyten.fr>
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

#include "biji-item.h"
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



/* To perform something after async tracker query
 * TODO : implemet this with GObject */
typedef struct {

  BijiManager *manager;

  /* usually a query */

  gchar *str;
  BijiInfoSet *info;

  /* after the query, _one of_ the callbacks */

  BijiBoolCallback        bool_cb;
  BijiInfoCallback        info_cb;
  BijiItemCallback        item_cb;
  BijiItemsListCallback   list_cb;
  BijiInfoSetsHCallback   hash_cb;


  gpointer user_data;

} BijiTrackerFinisher;


/* finisher stores _one of the possible callbacks _
 * we could cast as well */


static BijiTrackerFinisher *
biji_tracker_finisher_new (BijiManager          *manager,
                           gchar                 *str,
                           BijiInfoSet           *info,
                           BijiBoolCallback       bool_cb,
                           BijiInfoCallback       info_cb,
                           BijiItemCallback       item_cb,
                           BijiItemsListCallback  list_cb,
                           BijiInfoSetsHCallback  hash_cb,
                           gpointer               user_data)
{
  BijiTrackerFinisher *retval = g_slice_new (BijiTrackerFinisher);

  retval->manager = manager;
  retval->str = str;
  retval->info = info;
  retval->bool_cb = bool_cb;
  retval->info_cb = info_cb;
  retval->item_cb = item_cb;
  retval->list_cb = list_cb;
  retval->hash_cb = hash_cb;
  retval->user_data = user_data;

  return retval;
}


/* Only heap is str */

static void
biji_tracker_finisher_free (BijiTrackerFinisher *f)
{
  g_clear_pointer (&f->str, g_free);
  g_slice_free (BijiTrackerFinisher, f);
}


static TrackerSparqlConnection*
get_connection (BijiManager *manager)
{
  return biji_manager_get_tracker_connection (manager);
}


/* TODO : populate the boolean */

static void
biji_finish_update (GObject *source_object,
                    GAsyncResult *res,
                    gpointer user_data)
{
  TrackerSparqlConnection *self;
  BijiTrackerFinisher *finisher;
  GError *error;
  gchar *query;


  self = TRACKER_SPARQL_CONNECTION (source_object);
  finisher = user_data;
  error = NULL;
  query = finisher->str;

  tracker_sparql_connection_update_finish (self, res, &error);

  if (error)
  {
    g_warning ("%s : query=|||%s|||", error->message, query);
    g_error_free (error);
  }

  /* See if the query has something to perform afterward */
  if (finisher->bool_cb)
    finisher->bool_cb (TRUE, finisher->user_data);

  biji_tracker_finisher_free (finisher);
}


static void
biji_perform_update_async_and_free (TrackerSparqlConnection *connection,
                                    gchar *query,
                                    BijiBoolCallback f,
                                    gpointer user_data)
{
  BijiTrackerFinisher *finisher;

  finisher = biji_tracker_finisher_new
              (NULL, query, NULL, f, NULL, NULL, NULL, NULL, user_data);
  tracker_sparql_connection_update_async (connection,
                                          query,
                                          0,     // priority
                                          NULL,
                                          biji_finish_update,
                                          finisher);
}


/* Don't worry too much. We just want plain text here */
static gchar *
tracker_str (const gchar * string )
{
  g_return_val_if_fail (string != NULL, g_strdup (""));

  return biji_str_mass_replace (string, "\n", " ", "'", " ", NULL);
}



static gchar *
get_note_url (BijiNoteObj *note)
{
  return g_strdup_printf ("file://%s", biji_item_get_uuid (BIJI_ITEM (note)));
}



static void
biji_query_info_hash_finish (GObject      *source_object,
                             GAsyncResult *res,
                             gpointer      user_data)
{
  TrackerSparqlConnection *self;
  TrackerSparqlCursor *cursor;
  GError *error;
  GHashTable *result;
  BijiTrackerFinisher *finisher;

  self = TRACKER_SPARQL_CONNECTION (source_object);
  finisher = (BijiTrackerFinisher*) user_data;
  error = NULL;
  result = g_hash_table_new_full (
    g_str_hash, g_str_equal, NULL, (GDestroyNotify) biji_info_set_free);

  cursor = tracker_sparql_connection_query_finish (self,
                                                   res,
                                                   &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  if (cursor)
  {

    while (tracker_sparql_cursor_next (cursor, NULL, NULL))
    {
      BijiInfoSet *set = biji_info_set_new ();
      GTimeVal time = {0,0};

      set->tracker_urn = g_strdup (tracker_sparql_cursor_get_string (cursor, BIJI_URN_COL, NULL));
      set->title = g_strdup (tracker_sparql_cursor_get_string (cursor, BIJI_TITLE_COL, NULL));
      
      if (g_time_val_from_iso8601 (tracker_sparql_cursor_get_string (cursor, BIJI_MTIME_COL, NULL), &time))
        set->mtime = time.tv_sec;

      else
        set->mtime = 0;

      g_hash_table_replace (result, set->tracker_urn, set);
    }

    g_object_unref (cursor);
  }

  finisher->hash_cb (result, finisher->user_data);
  biji_tracker_finisher_free (finisher);
  return;
}


static void
biji_query_items_list_finish (GObject              *source_object,
                              GAsyncResult         *res,
                              gpointer              user_data)
{
  TrackerSparqlConnection *self;
  TrackerSparqlCursor *cursor;
  BijiTrackerFinisher  *finisher;
  GError *error;
  GList *result;

  self =  TRACKER_SPARQL_CONNECTION (source_object);
  result = NULL;
  error = NULL;
  finisher = (BijiTrackerFinisher *) user_data;

  if (finisher->list_cb == NULL)
    return;

  cursor = tracker_sparql_connection_query_finish (self, res, &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  if (cursor)
  {
    const gchar *full_path;
    gchar *path;
    BijiItem *item = NULL;

    while (tracker_sparql_cursor_next (cursor, NULL, NULL))
    {
      full_path = tracker_sparql_cursor_get_string (cursor, 0, NULL);

      if (g_str_has_prefix (full_path, "file://"))
      {
        GString *string;

        string = g_string_new (full_path);
        g_string_erase (string, 0, 7);
        path = g_string_free (string, FALSE);
      }

      else
      {
        path = g_strdup (full_path);
      }

      item = biji_manager_get_item_at_path (finisher->manager, path);

      /* Sorting is done in another place */
      if (item)
        result = g_list_prepend (result, item);
    }

    g_object_unref (cursor);
  }

  finisher->list_cb (result, finisher->user_data);
  biji_tracker_finisher_free (finisher);
}



static void
bjb_query_async (BijiManager           *manager,
                 gchar                  *query,
                 BijiInfoSetsHCallback   hash_cb,
                 BijiItemsListCallback  list_cb,
                 gpointer                user_data)
{
  BijiTrackerFinisher     *finisher;
  GAsyncReadyCallback     callback = NULL;

  finisher = biji_tracker_finisher_new (manager, NULL, NULL, NULL, NULL, NULL, list_cb, hash_cb, user_data);

  if (hash_cb != NULL)
    callback = biji_query_info_hash_finish;

  else if (list_cb != NULL)
    callback = biji_query_items_list_finish;

  if (callback)
   tracker_sparql_connection_query_async (
      get_connection (manager), query, NULL, callback, finisher);
}


void
biji_get_all_notebooks_async (BijiManager *manager,
                                BijiInfoSetsHCallback cb,
                                gpointer user_data)
{
  gchar *query = g_strconcat (
    "SELECT ?c ?title ?mtime ",
    "WHERE { ?c a nfo:DataContainer ;",
    "nie:title ?title ; ",
    "nie:contentLastModified ?mtime ;"
    "nie:generator 'Bijiben'}",
    NULL);

  bjb_query_async (manager, query, cb, NULL, user_data);
}




/* FIXME: returns file://$PATH while we want $PATH
 *        workaround in biji_query_items_list_finish */
void
biji_get_items_with_notebook_async (BijiManager          *manager,
                                      const gchar           *notebook,
                                      BijiItemsListCallback  list_cb,
                                      gpointer               user_data)
{
  gchar *query;

  query = g_strdup_printf ("SELECT ?s WHERE {?c nie:isPartOf ?s; nie:title '%s'}",
                           notebook);

  bjb_query_async (manager, query, NULL, list_cb, user_data);
}




void
biji_get_items_matching_async (BijiManager           *manager,
                               BijiItemsGroup         group,
                               gchar                 *needle,
                               BijiItemsListCallback  list_cb,
                               gpointer               user_data)
{
  gchar *lower;
  gchar *query;


  lower = g_utf8_strdown (needle, -1);

  /* We want to retrieve the key that noteBook uses.
   * for notes: that is url. A file path is unique.
   * for notebooks: we have no url, directly use urn:uuid */

  query = g_strconcat (
    "SELECT tracker:coalesce (?url, ?urn) WHERE ",
    "{",
    "  {  ?urn a nfo:Note",
    "    .?urn nie:title ?title",
    "    .?urn nie:plainTextContent ?content",
    "    .?urn nie:url ?url",
    "    .?urn nie:generator 'Bijiben'",
    "    .FILTER (",
    "    fn:contains (fn:lower-case (?content), '", lower, "' ) || ",
    "    fn:contains (fn:lower-case (?title)  , '", lower, "'))} ",
    "UNION",
    "  {  ?urn a nfo:DataContainer",
    "    .?urn nie:title ?title",
    "    .?urn nie:generator 'Bijiben'",
    "    .FILTER (",
    "    fn:contains (fn:lower-case (?title), '", lower, "'))}",
    "}",
    NULL);

  g_free (lower);
  bjb_query_async (manager, query, NULL, list_cb, user_data);
}


static void
on_new_notebook_query_executed (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  BijiTrackerFinisher *finisher = user_data;
  TrackerSparqlConnection *connection = TRACKER_SPARQL_CONNECTION (source_object);
  GError *error;
  GVariant *variant;
  GVariant *child;
  gchar *key = NULL;
  gchar *val = NULL;
  gchar *urn = NULL;
  BijiNotebook *notebook = NULL;

  error = NULL;
  variant = tracker_sparql_connection_update_blank_finish (connection, res, &error);
  if (error != NULL)
    {
      g_warning ("Unable to create notebook: %s", error->message);
      g_error_free (error);
      goto out;
    }

  child = g_variant_get_child_value (variant, 0); /* variant is now aa{ss} */
  g_variant_unref (variant);
  variant = child;

  child = g_variant_get_child_value (variant, 0); /* variant is now s{ss} */
  g_variant_unref (variant);
  variant = child;

  child = g_variant_get_child_value (variant, 0); /* variant is now {ss} */
  g_variant_unref (variant);
  variant = child;

  child = g_variant_get_child_value (variant, 0);
  key = g_variant_dup_string (child, NULL);
  g_variant_unref (child);

  child = g_variant_get_child_value (variant, 1);
  val = g_variant_dup_string (child, NULL);
  g_variant_unref (child);

  g_variant_unref (variant);

  if (g_strcmp0 (key, "res") == 0)
    urn = val;

  /* Update the note manager */
  if (urn)
  {
    notebook = biji_notebook_new (
                       G_OBJECT (finisher->manager),
                       urn,
                       finisher->str,
                       g_get_real_time () / G_USEC_PER_SEC);
    biji_manager_add_item (finisher->manager, BIJI_ITEM (notebook), BIJI_LIVING_ITEMS, TRUE);
  }

  /* Run the callback from the caller */

 out:
  if (finisher->item_cb != NULL)
    (*finisher->item_cb) (BIJI_ITEM (notebook), finisher->user_data);

  g_free (val);
  g_free (key);
  biji_tracker_finisher_free (finisher);
}


/* This func creates the notebook,
 * gives the urn to the notemanager,
 * then run the 'afterward' callback */
void
biji_create_new_notebook_async (BijiManager     *manager,
                                const gchar      *name,
                                BijiItemCallback  item_cb,
                                gpointer          user_data)
{
  gchar *query;
  GTimeVal tv;
  gchar *time;
  gint64 timestamp;
  BijiTrackerFinisher *finisher;

  timestamp = g_get_real_time () / G_USEC_PER_SEC;
  tv.tv_sec = timestamp;
  tv.tv_usec = 0;
  time = g_time_val_to_iso8601 (&tv);

  query = g_strdup_printf ("INSERT { _:res a nfo:DataContainer ; a nie:DataObject ; "
                            "nie:contentLastModified '%s' ; "
                            "nie:title '%s' ; "
                            "nie:generator 'Bijiben' }",
                            time,
                            name);
  g_free (time);

  /* The finisher has all the pointers we want.
   * And the callback will free it */
  finisher = biji_tracker_finisher_new (manager, g_strdup (name), NULL, NULL, NULL, item_cb, NULL, NULL, user_data);
  tracker_sparql_connection_update_blank_async (get_connection (manager),
                                                query,
                                                G_PRIORITY_DEFAULT,
                                                NULL,
                                                on_new_notebook_query_executed,
                                                finisher);
}


/* removes the tag EVEN if files associated. */

void
biji_remove_notebook_from_tracker (BijiManager *manager, const gchar *urn)
{
  gchar *query;

  query = g_strdup_printf ("DELETE {'%s' a nfo:DataContainer}", urn);
  biji_perform_update_async_and_free (get_connection (manager), query, NULL, NULL);
}


void
biji_push_existing_notebook_to_note (BijiNoteObj       *note,
                                       gchar             *title,
                                       BijiBoolCallback   afterward,
                                       gpointer           user_data)
{
  gchar *url, *query;

  url = get_note_url (note);
  query = g_strdup_printf ("INSERT {?urn nie:isPartOf '%s'} WHERE {?urn a nfo:DataContainer; nie:title '%s'; nie:generator 'Bijiben'}",
                           url, title);

  biji_perform_update_async_and_free (
      get_connection (biji_item_get_manager (BIJI_ITEM (note))), query, afterward, user_data);
  g_free (url);
}



void
biji_remove_notebook_from_note (BijiNoteObj       *note,
                                  BijiItem          *coll,
                                  BijiBoolCallback   afterward,
                                  gpointer           user_data)
{
  gchar *url, *query;

  url = get_note_url (note);

  query = g_strdup_printf (
    "DELETE {'%s' nie:isPartOf '%s'}",
    biji_item_get_uuid (coll), url);


  biji_perform_update_async_and_free (get_connection (biji_item_get_manager (coll)), query, afterward, user_data);
  g_free (url);
}


void
biji_note_delete_from_tracker (BijiNoteObj *note)
{
  BijiItem *item;
  gchar *query;
  const gchar *uuid;
  BijiManager *manager;

  item = BIJI_ITEM (note);
  manager = biji_item_get_manager (item);
  uuid = biji_item_get_uuid (item);
  query = g_strdup_printf ("DELETE { <%s> a rdfs:Resource }", uuid);
  biji_perform_update_async_and_free (get_connection (manager), query, NULL, NULL);
}



void
biji_tracker_trash_ressource (BijiManager *manager,
                              gchar *tracker_urn)
{
  gchar *query;

  query = g_strdup_printf ("DELETE { <%s> a rdfs:Resource }", tracker_urn);
  biji_perform_update_async_and_free (get_connection (manager), query, NULL, NULL);
}


static void
update_ressource (BijiTrackerFinisher *finisher, gchar *tracker_urn_uuid )
{
  BijiManager *manager;
  BijiInfoSet *info;
  gchar *query, *created, *mtime, *content;
  GTimeVal t;

  manager = finisher->manager;
  info = finisher->info;

  t.tv_usec = 0;
  t.tv_sec = info->mtime;
  mtime = g_time_val_to_iso8601 (&t);

  t.tv_sec = info->created;
  created =  g_time_val_to_iso8601 (&t);


  content = tracker_str (info->content);

  g_message ("Updating ressource <%s> %s", info->title, tracker_urn_uuid);

  query = g_strdup_printf (
      "INSERT OR REPLACE { <%s> a nfo:Note , nie:DataObject ; "
      "nie:url '%s' ; "
      "nie:contentLastModified '%s' ; "
      "nie:contentCreated '%s' ; "
      "nie:title '%s' ; "
      "nie:plainTextContent '%s' ; "
      "nie:dataSource '%s' ;"
      "nie:generator 'Bijiben' . }",
      tracker_urn_uuid,
      info->url,
      mtime,
      created,
      info->title,
      content,
      info->datasource_urn);

  biji_perform_update_async_and_free (get_connection (manager), query, NULL, NULL);

  g_free (tracker_urn_uuid);
  g_free (mtime);
  g_free (created);
  g_free (content);
  biji_tracker_finisher_free (finisher);
}


static void
push_new_note (BijiTrackerFinisher *finisher)
{
  BijiManager *manager;
  BijiInfoSet *info;
  gchar *query, *content, *created_time, *mtime;
  GTimeVal t;

  manager = finisher->manager;
  info = finisher->info;
  g_message ("Creating ressource <%s> %s", info->title, info->url);

  content = tracker_str (info->content);
  t.tv_usec = 0;
  t.tv_sec = info->mtime;
  mtime = g_time_val_to_iso8601 (&t);


  t.tv_sec = info->created;
  created_time =  g_time_val_to_iso8601 (&t);


  query = g_strconcat (
    "INSERT { _:res a nfo:Note ; ",
    "     a nie:DataObject ; ",
    "     nie:contentLastModified '", mtime,                "' ;",
    "     nie:contentCreated      '", created_time,         "' ;",
    "     nie:title               '", info->title,          "' ;",
    "     nie:url                 '", info->url,            "' ;",
    "     nie:plainTextContent    '", content,              "' ;",
    "     nie:dataSource          '", info->datasource_urn, "' ;",
    "     nie:generator                              'Bijiben' }",
    NULL);


  g_debug ("%s", query);

  tracker_sparql_connection_update_blank_async (get_connection (manager),
                                                query,
                                                G_PRIORITY_DEFAULT,
                                                NULL,
                                                NULL,  // callback,
                                                NULL); // user_data);


  g_free (query);
  g_free (content);
  g_clear_pointer (&mtime, g_free);
  g_clear_pointer (&created_time, g_free);
  biji_tracker_finisher_free (finisher);
}


static void
ensure_ressource_callback (GObject *source_object,
                           GAsyncResult *res,
                           gpointer user_data)
{
  TrackerSparqlConnection *connection;
  TrackerSparqlCursor     *cursor;
  BijiTrackerFinisher     *finisher;
  GError                  *error;
  gchar                   *urn_found;


  connection = TRACKER_SPARQL_CONNECTION (source_object);
  finisher = user_data;
  error = NULL;
  urn_found = NULL;
  cursor = tracker_sparql_connection_query_finish (connection, res, &error);

  if (error)
  {
    g_warning ("ENSURE RESSOURCE : error %s", error->message);
    g_error_free (error);
    biji_tracker_finisher_free (finisher);
    return;
  }

  /* Queried ressource found into tracker */

  if (cursor)
  {

    if (tracker_sparql_cursor_next (cursor, NULL, NULL))
      urn_found = g_strdup (tracker_sparql_cursor_get_string (cursor, 0, NULL));

    g_object_unref (cursor);
  }


  if (urn_found != NULL)
    update_ressource (finisher, urn_found);

  else
    push_new_note (finisher);
}



void
biji_tracker_ensure_ressource_from_info (BijiManager *manager, 
                                         BijiInfoSet *info)
{
  gchar *query;
  BijiTrackerFinisher *finisher;


  query = g_strconcat (
    "SELECT ?urn ?time WHERE { ?urn a nfo:Note ;",
    "                          nie:title ?title ;",
    "                          nie:contentLastModified ?time ;",
    "                          nie:url '", info->url, "' }",
                               NULL);


  /* No matter user callback or not,
   * we'll need our own to push if needed */

  finisher = biji_tracker_finisher_new (
               manager,
               NULL,
               info,
               NULL,
               NULL,
               NULL,
               NULL,
               NULL,
               NULL);  // user_data);

  tracker_sparql_connection_query_async (
      get_connection (manager), query, NULL, ensure_ressource_callback, finisher);
}





void
biji_tracker_ensure_datasource (BijiManager *manager,
                                const gchar *datasource,
                                const gchar *identifier,
                                BijiBoolCallback cb,
                                gpointer user_data)
{
  gchar *query;


  query = g_strdup_printf (
    "INSERT OR REPLACE INTO <%s> {"
    "  <%s> a nie:DataSource ; nao:identifier \"%s\" }",
    datasource,
    datasource,
    identifier);

  biji_perform_update_async_and_free (
    get_connection (manager), query, cb, user_data);
}



static void
on_info_queried (GObject *source_object,
                 GAsyncResult *res,
                 gpointer user_data)
{
  TrackerSparqlConnection *connection;
  TrackerSparqlCursor     *cursor;
  BijiTrackerFinisher     *finisher;
  GError                  *error;
  BijiInfoSet             *retval;
  GTimeVal                 t;


  connection = TRACKER_SPARQL_CONNECTION (source_object);
  finisher = user_data;
  error = NULL;
  retval = NULL;
  cursor = tracker_sparql_connection_query_finish (connection, res, &error);

  if (error)
  {
    g_warning ("Check for Info : error %s", error->message);
    g_error_free (error);

    /* Something went wrong, callback and free memory
       & leave tracker alone */
    if (finisher->info_cb != NULL)
      finisher->info_cb (retval, finisher->user_data);
    biji_tracker_finisher_free (finisher);
    return;
  }

  /* Queried ressource found into tracker */
  if (cursor)
  {
    if (tracker_sparql_cursor_next (cursor, NULL, NULL))
    {
      retval = biji_info_set_new ();

      retval->url = g_strdup (tracker_sparql_cursor_get_string (cursor, 0, NULL));
      retval->title = g_strdup (tracker_sparql_cursor_get_string (cursor, 1, NULL));

      if (g_time_val_from_iso8601 (tracker_sparql_cursor_get_string (cursor, 2, NULL), &t))
        retval->mtime = t.tv_sec;

      retval->content = biji_str_replace (
        tracker_sparql_cursor_get_string (cursor, 3, NULL), "b&lt;br/&gt", "\n");

      if (g_time_val_from_iso8601 (tracker_sparql_cursor_get_string (cursor, 4, NULL), &t))
        retval->created = t.tv_sec;


      /* Check if the ressource is up to date */

      if (finisher->info->mtime != retval->mtime)
        g_clear_pointer (&retval, biji_info_set_free);
    }

    g_object_unref (cursor);
  }


  /* No matter retval or not, we are supposed to callback
   * (REMEMBER CALLER IS RESPONSIBLE FOR FREEING INFO SET) */
  if (finisher->info_cb != NULL)
    finisher->info_cb (retval, finisher->user_data);


  biji_tracker_finisher_free (finisher);
}


void
biji_tracker_check_for_info                (BijiManager *manager,
                                            gchar *url,
                                            gint64 mtime,
                                            BijiInfoCallback callback,
                                            gpointer user_data)
{
  BijiInfoSet *info;
  BijiTrackerFinisher *finisher;
  gchar *query;

  query = g_strconcat (
    " SELECT ?urn ?title ?time ?content ?created",
    " WHERE { ?urn a nfo:Note ;",
    "         nie:title ?title ;",
    "         nie:contentLastModified ?time ;",
    "         nie:plainTextContent ?content ;",
    "         nie:contentCreated ?created ;",
    "         nie:url '", url, "' }",
    NULL);

  /* No matter user callback or not,
   * we'll need our own to push if needed */
  info = biji_info_set_new ();
  info->url = url;
  info->mtime = mtime;

  finisher = biji_tracker_finisher_new (
               manager,
               NULL,
               info,
               NULL,
               callback,
               NULL,
               NULL,
               NULL,
               user_data);

  tracker_sparql_connection_query_async (
      get_connection (manager), query, NULL, on_info_queried, finisher);


  g_free (query);
}
