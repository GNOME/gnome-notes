/* biji-tracker.c
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

#include "biji-item.h"
#include "biji-tracker.h"

/* To perform something after async tracker query
 * TODO : implemet this with GObject */
typedef struct {

  BijiNoteBook *book;

  /* usually a query */
  gchar *str;

  /* after the query */
  BijiFunc func;
  gpointer user_data;

} BijiTrackerFinisher;

static BijiTrackerFinisher *
biji_tracker_finisher_new (BijiNoteBook *book,
                           gchar        *str,
                           BijiFunc      f,
                           gpointer      user_data)
{
  BijiTrackerFinisher *retval = g_new (BijiTrackerFinisher, 1);

  retval->book = book;
  retval->str = str;
  retval->func = f;
  retval->user_data = user_data;

  return retval;
}

static void
biji_tracker_finisher_free (BijiTrackerFinisher *f)
{
  if (f->str)
    g_free (f->str);

  g_free (f);
}

static BijiTrackerInfoSet *
biji_tracker_info_set_new ()
{
  return g_slice_new (BijiTrackerInfoSet);
}

static void
biji_tracker_info_set_free (BijiTrackerInfoSet *set)
{
  g_free (set->urn);
  g_free (set->title);

  g_slice_free (BijiTrackerInfoSet, (gpointer) set);
}

TrackerSparqlConnection *bjb_connection ;

static TrackerSparqlConnection *
get_connection_singleton(void)
{    
  if ( bjb_connection == NULL )
  {
    GError *error = NULL ;
      
    g_log(G_LOG_DOMAIN,G_LOG_LEVEL_DEBUG,"Getting tracker connection.");
    bjb_connection = tracker_sparql_connection_get (NULL,&error);
      
    if ( error ) 
      g_log(G_LOG_DOMAIN,G_LOG_LEVEL_DEBUG,"Connection errror. Tracker out.");
  }

  return bjb_connection ;
}

static void
bjb_perform_query_async (gchar *query,
                         GAsyncReadyCallback f,
                         gpointer user_data)
{
  tracker_sparql_connection_query_async (get_connection_singleton (),
                                         query,
                                         NULL,
                                         f,
                                         user_data);
}

static void
biji_finish_update (GObject *source_object,
                    GAsyncResult *res,
                    gpointer user_data)
{
  TrackerSparqlConnection *self = TRACKER_SPARQL_CONNECTION (source_object);
  GError *error = NULL;
  BijiTrackerFinisher *finisher = user_data;
  gchar *query = finisher->str;

  tracker_sparql_connection_update_finish (self, res, &error);

  if (error)
  {
    g_warning ("%s : query=|||%s|||", error->message, query);
    g_error_free (error);
  }

  /* See if the query has something to perform afterward */
  if (finisher->func)
    finisher->func (finisher->user_data);

  biji_tracker_finisher_free (finisher);
}

static void
biji_perform_update_async_and_free (gchar *query, BijiFunc f, gpointer user_data)
{
  BijiTrackerFinisher *finisher = biji_tracker_finisher_new (NULL, query, f, user_data);

  tracker_sparql_connection_update_async (get_connection_singleton(),
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
  return biji_str_mass_replace (string, "\n", " ", "'", " ", NULL);
}

static gchar *
to_8601_date( gchar * dot_iso_8601_date )
{
  gchar *result = dot_iso_8601_date ;
  return g_strdup_printf ( "%sZ",
                           g_utf8_strncpy  (result ,dot_iso_8601_date, 19) );
}

static gchar *
get_note_url (BijiNoteObj *note)
{
  return g_strdup_printf ("file://%s", biji_item_get_uuid (BIJI_ITEM (note)));
}


/* This func provides Collections, URN, mtime */
GHashTable *
biji_get_all_collections_finish (GObject *source_object,
                                 GAsyncResult *res)
{
  TrackerSparqlConnection *self = TRACKER_SPARQL_CONNECTION (source_object);
  TrackerSparqlCursor *cursor;
  GError *error = NULL;
  GHashTable *result = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              NULL,
                                              (GDestroyNotify) biji_tracker_info_set_free);

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
      BijiTrackerInfoSet *set = biji_tracker_info_set_new ();

      set->urn = g_strdup (tracker_sparql_cursor_get_string (cursor, BIJI_URN_COL, NULL));
      set->title = g_strdup (tracker_sparql_cursor_get_string (cursor, BIJI_TITLE_COL, NULL));
      set->mtime = g_strdup (tracker_sparql_cursor_get_string (cursor, BIJI_MTIME_COL, NULL));

      g_hash_table_replace (result, set->urn, set);
    }

    g_object_unref (cursor);
  }

  return result;
}
 
void
biji_get_all_collections_async (GAsyncReadyCallback f,
                                gpointer user_data)
{
  gchar *query = g_strconcat (
    "SELECT ?c ?title ?mtime ",
    "WHERE { ?c a nfo:DataContainer ;",
    "nie:title ?title ; ",
    "nie:contentLastModified ?mtime ;"
    "nie:generator 'Bijiben'}",
    NULL);

  bjb_perform_query_async (query, f, user_data);
}

GList *
biji_get_items_with_collection_finish (GObject *source_object,
                                       GAsyncResult *res,
                                       BijiNoteBook *book)
{
  TrackerSparqlConnection *self = TRACKER_SPARQL_CONNECTION (source_object);
  TrackerSparqlCursor *cursor;
  GError *error = NULL;
  GList *result = NULL;

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

      item = biji_note_book_get_item_at_path (book, path);

      /* Sorting is done in another place */
      if (item)
        result = g_list_prepend (result, item);

      g_free (path);
    }

    g_object_unref (cursor);
  }

  return result;
}

void
biji_get_items_with_collection_async (const gchar *collection,
                                      GAsyncReadyCallback f,
                                      gpointer user_data)
{
  gchar *query;

  query = g_strdup_printf ("SELECT ?s WHERE {?c nie:isPartOf ?s; nie:title '%s'}",
                           collection);

  bjb_perform_query_async (query, f, user_data);
}

GList *
biji_get_notes_with_strings_or_collection_finish (GObject *source_object,
                                                  GAsyncResult *res,
                                                  BijiNoteBook *book)
{
  TrackerSparqlConnection *self = TRACKER_SPARQL_CONNECTION (source_object);
  TrackerSparqlCursor *cursor;
  GError *error = NULL;
  GList *result = NULL;

  cursor = tracker_sparql_connection_query_finish (self, res, &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  if (cursor)
  {
    const gchar *path;
    BijiItem *item = NULL;

    while (tracker_sparql_cursor_next (cursor, NULL, NULL))
    {
      path = tracker_sparql_cursor_get_string (cursor, 0, NULL);
      item = biji_note_book_get_item_at_path (book, path);

      /* Sorting is done in another place */
      if (item)
        result = g_list_prepend (result, item);
    }

    g_object_unref (cursor);
  }

  return result;
}


void
biji_get_notes_with_string_or_collection_async (gchar *needle, GAsyncReadyCallback f, gpointer user_data)
{
  gchar *lower;
  gchar *query;

  lower = g_utf8_strdown (needle, -1);
  query = g_strconcat (
    "SELECT ?urn WHERE {",
    "  { ?urn a nie:DataObject ;",
    "    nie:title ?title ; nie:plainTextContent ?content ;",
    "    nie:generator 'Bijiben' . FILTER (",
    "    fn:contains (fn:lower-case (?content), '", lower, "' ) || ",
    "    fn:contains (fn:lower-case (?title)  , '", lower, "'))} ",
    "UNION",
    "  { ?urn a nfo:DataContainer ;",
    "    nie:title ?title ; nie:generator 'Bijiben' . FILTER (",
    "    fn:contains (fn:lower-case (?title), '", lower, "'))}}",
    NULL);

  g_free (lower);
  bjb_perform_query_async (query, f, user_data);
}

static void
on_new_collection_query_executed (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  BijiTrackerFinisher *finisher = user_data;
  TrackerSparqlConnection *connection = TRACKER_SPARQL_CONNECTION (source_object);
  GError *error;
  GVariant *variant;
  GVariant *child;
  gchar *key = NULL;
  gchar *val = NULL;
  gchar *urn = NULL;

  error = NULL;
  variant = tracker_sparql_connection_update_blank_finish (connection, res, &error);
  if (error != NULL)
    {
      g_warning ("Unable to create collection: %s", error->message);
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

  /* Update the note book */
  if (urn)
  {
    gint64 timestamp;
    GTimeVal tv;
    gchar *time;

    timestamp = g_get_real_time () / G_USEC_PER_SEC;
    tv.tv_sec = timestamp;
    tv.tv_usec = 0;
    time = g_time_val_to_iso8601 (&tv);

    BijiCollection *collection;

    collection = biji_collection_new (
                       G_OBJECT (finisher->book),
                       urn,
                       finisher->str,
                       time);
    biji_note_book_add_item (finisher->book, BIJI_ITEM (collection), TRUE);
  }

  /* Run the callback from the caller */

 out:
  if (finisher->func != NULL)
    (*finisher->func) (finisher->user_data);

  g_free (val);
  g_free (key);
  biji_tracker_finisher_free (finisher);
}

/* This func creates the collection,
 * gives the urn to the notebook,
 * then run the 'afterward' callback */
void
biji_create_new_collection_async (BijiNoteBook *book,
                                  const gchar  *name,
                                  BijiFunc      afterward,
                                  gpointer      user_data)
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
  finisher = biji_tracker_finisher_new (book, g_strdup (name), afterward, user_data);
  tracker_sparql_connection_update_blank_async (get_connection_singleton (),
                                                query,
                                                G_PRIORITY_DEFAULT,
                                                NULL,
                                                on_new_collection_query_executed,
                                                finisher);
}

/* removes the tag EVEN if files associated.
 * TODO : afterward */
void
biji_remove_collection_from_tracker (const gchar *urn)
{
  gchar *query = g_strdup_printf ("DELETE {'%s' a nfo:DataContainer}", urn);
  biji_perform_update_async_and_free (query, NULL, NULL);
}

void
biji_push_existing_collection_to_note (BijiNoteObj *note, gchar *title)
{
  gchar *url = get_note_url (note);
  gchar *query = g_strdup_printf ("INSERT {?urn nie:isPartOf '%s'} WHERE {?urn a nfo:DataContainer; nie:title '%s'; nie:generator 'Bijiben'}",
                                  url, title);

  biji_perform_update_async_and_free (query, NULL, NULL);
  g_free (url);
}

/* This one is to be fixed */
void
biji_remove_collection_from_note (BijiNoteObj *note, gchar *urn)
{
  gchar *url = get_note_url (note);
  gchar *query = g_strdup_printf ("DELETE {'%s' nie:isPartOf '%s'}", urn, url);

  biji_perform_update_async_and_free (query, NULL, NULL);
  g_free (url);
}

void
biji_note_delete_from_tracker (BijiNoteObj *note)
{
  gchar *query;

  query = g_strdup_printf ("DELETE { <%s> a rdfs:Resource }",
                           biji_item_get_uuid (BIJI_ITEM (note)));

  biji_perform_update_async_and_free (query, NULL, NULL);
}

void
bijiben_push_note_to_tracker (BijiNoteObj *note)
{
  gchar *title,*content,*file,*date, *create_date,*last_change_date;
  const gchar *path;

  g_return_if_fail (BIJI_IS_NOTE_OBJ (note));

  path = biji_item_get_uuid (BIJI_ITEM (note));
  title = tracker_str (biji_item_get_title (BIJI_ITEM (note)));
  file = get_note_url (note);

  date = biji_note_obj_get_create_date (note);
  create_date = to_8601_date (date);
  g_free (date);

  date = biji_note_obj_get_last_change_date (note);
  last_change_date = to_8601_date (date);
  g_free (date);

  content = tracker_str (biji_note_get_raw_text (note));

  /* TODO : nie:mimeType Note ;
   * All these properties are unique and thus can be "updated"
   * which is not the case of tags */
  gchar *query = g_strdup_printf (
                           "INSERT OR REPLACE { <%s> a nfo:Note , nie:DataObject ; \
                            nie:url '%s' ; \
                            nie:contentLastModified '%s' ; \
                            nie:contentCreated '%s' ; \
                            nie:title '%s' ; \
                            nie:plainTextContent '%s' ; \
                            nie:generator 'Bijiben' . }",
                           path,
                           file,
                           last_change_date,
                           create_date,
                           title,
                           content) ;

  biji_perform_update_async_and_free (query, NULL, NULL);

  g_free(title);
  g_free(file);
  g_free(content); 
  g_free(create_date);
  g_free(last_change_date);
}

