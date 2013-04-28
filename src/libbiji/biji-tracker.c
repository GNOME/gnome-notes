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

/* To perform something after async tracker query */
typedef struct {

  /* query, could add the cancellable */
  gchar *query;

  /* after the query */
  BijiFunc func;
  gpointer user_data;

} BijiTrackerFinisher;

static BijiTrackerFinisher *
biji_tracker_finisher_new (gchar *query, BijiFunc f, gpointer user_data)
{
  BijiTrackerFinisher *retval = g_new (BijiTrackerFinisher, 1);

  retval->query = query;
  retval->func = f;
  retval->user_data = user_data;

  return retval;
}

static void
biji_tracker_finisher_free (BijiTrackerFinisher *f)
{
  if (f->query)
    g_free (f->query);

  g_free (f);
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
  gchar *query = finisher->query;

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
  BijiTrackerFinisher *finisher = biji_tracker_finisher_new (query, f, user_data);

  tracker_sparql_connection_update_async (get_connection_singleton(),
                                          query,
                                          0,     // priority
                                          NULL,
                                          biji_finish_update,
                                          finisher);
}

/* Don't worry too much. We just want plain text here */
static gchar *
tracker_str ( gchar * string )
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
  gchar *path, *retval;

  path = biji_item_get_uuid (BIJI_ITEM (note));
  retval = g_strdup_printf ("file://%s", path);
  g_free (path);
  return retval;
}

/////////////// Tags

/* This func only provides collections.
 * TODO : include number of notes / files */
GHashTable *
biji_get_all_collections_finish (GObject *source_object,
                                 GAsyncResult *res)
{
  TrackerSparqlConnection *self = TRACKER_SPARQL_CONNECTION (source_object);
  TrackerSparqlCursor *cursor;
  GError *error = NULL;
  GHashTable *result = g_hash_table_new_full (g_str_hash,
                                              g_str_equal,
                                              g_free,
                                              g_free);

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
    gchar *urn, *collection;

    while (tracker_sparql_cursor_next (cursor, NULL, NULL))
    {
      urn = g_strdup (tracker_sparql_cursor_get_string (cursor, 0, NULL));
      collection = g_strdup (tracker_sparql_cursor_get_string (cursor, 1, NULL));
      g_hash_table_replace (result, collection, urn);
    }

    g_object_unref (cursor);
  }

  return result;
}
 
void
biji_get_all_collections_async (GAsyncReadyCallback f,
                                gpointer user_data)
{
  gchar *query = "SELECT ?c ?title WHERE { ?c a nfo:DataContainer ; nie:title ?title ; nie:generator 'Bijiben'}";

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

/* FIXME : the nie:isPartOf returns file://$path, while
 *         union fts returns $path which leads to uggly code
 * TODO : not case sensitive */
void
biji_get_notes_with_string_or_collection_async (gchar *needle, GAsyncReadyCallback f, gpointer user_data)
{
  gchar *query;

  query = g_strdup_printf ("SELECT ?s WHERE {{?c nie:isPartOf ?s; nie:title '%s'} \
                           UNION {?s fts:match '%s'. ?s nie:generator 'Bijiben'}}",
                           needle, needle);

  bjb_perform_query_async (query, f, user_data);
}

void 
biji_create_new_collection (const gchar *tag, BijiFunc afterward, gpointer user_data)
{ 
  gchar *query = g_strdup_printf ("INSERT {_:result a nfo:DataContainer;a nie:DataObject;nie:title '%s' ; nie:generator 'Bijiben'}"
                                  ,tag);

  biji_perform_update_async_and_free (query, afterward, user_data);
}

/* removes the tag EVEN if files associated.
 * TODO : afterward */
void
biji_remove_collection_from_tracker (gchar *urn)
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
  gchar *query, *path;

  path = biji_item_get_uuid (BIJI_ITEM (note));
  query = g_strdup_printf ("DELETE { <%s> a rdfs:Resource }", path);
  g_free (path);

  biji_perform_update_async_and_free (query, NULL, NULL);
}

void
bijiben_push_note_to_tracker (BijiNoteObj *note)
{
  gchar *title,*content,*file,*date, *create_date,*last_change_date, *path;

  path = biji_item_get_uuid (BIJI_ITEM (note));
  title = tracker_str (biji_item_get_title (BIJI_ITEM (note)));
  file = g_strdup_printf ("file://%s", path);

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
  g_free (path);
}

