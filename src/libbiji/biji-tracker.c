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

#include "biji-tracker.h"

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
  gchar *query = user_data;

  tracker_sparql_connection_update_finish (self, res, &error);

  if (error)
  {
    g_warning ("%s : query=|||%s|||", error->message, query);
    g_error_free (error);
  }

  g_free (query);
}

static void
biji_perform_update_async_and_free (gchar *query)
{
  tracker_sparql_connection_update_async (get_connection_singleton(),
                                          query,
                                          0,     // priority
                                          NULL,
                                          biji_finish_update,
                                          query);
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
  return g_strdup_printf ("file://%s", biji_note_obj_get_path (note));
}

/////////////// Tags

/* This func only provides tags.
 * TODO : include number of notes / files */
GList *
biji_get_all_tags_finish (GObject *source_object,
                          GAsyncResult *res)
{
  TrackerSparqlConnection *self = TRACKER_SPARQL_CONNECTION (source_object);
  TrackerSparqlCursor *cursor;
  GError *error = NULL;
  GList *result = NULL;

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
    gchar* tag;

    while (tracker_sparql_cursor_next (cursor, NULL, NULL))
    {
      tag = g_strdup (tracker_sparql_cursor_get_string (cursor, 0, NULL));
      result = g_list_prepend (result, (gpointer) tag);
    }

    g_object_unref (cursor);
  }

  return result;
}
 
void
biji_get_all_tracker_tags_async (GAsyncReadyCallback f,
                                 gpointer user_data)
{
  gchar *query = "SELECT DISTINCT ?labels WHERE \
  { ?tags a nao:Tag ; nao:prefLabel ?labels. }";

  bjb_perform_query_async (query, f, user_data);
}

GList *
biji_get_notes_with_strings_or_tag_finish (GObject *source_object,
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
    gchar * uri;
    BijiNoteObj *note = NULL;

    while (tracker_sparql_cursor_next (cursor, NULL, NULL))
    {
      uri = g_strdup (tracker_sparql_cursor_get_string (cursor, 0, NULL));
      note = note_book_get_note_at_path (book, uri);

      /* Sorting is done in another place */
      if (note)
        result = g_list_prepend (result, note);
    }

    g_object_unref (cursor);
  }

  return result;
}

/* TODO : not case sensitive */
void
biji_get_notes_with_string_or_tag_async (gchar *needle, GAsyncReadyCallback f, gpointer user_data)
{
  gchar *query;

  query = g_strdup_printf ("SELECT ?s WHERE {{ ?s nie:generator 'Bijiben'. \
                           ?s a nfo:Note ;nao:hasTag [nao:prefLabel'%s'] } \
                           UNION { ?s fts:match '%s'. ?s nie:generator 'Bijiben'}}",
                           needle, needle);

  bjb_perform_query_async (query, f, user_data);
  g_free (query);
}

void 
push_tag_to_tracker(gchar *tag)
{ 
  gchar *query = g_strdup_printf ("INSERT {_:tag a nao:Tag ; \
  nao:prefLabel '%s' . } \
  WHERE { OPTIONAL {?tag a nao:Tag ; nao:prefLabel '%s'} . \
  FILTER (!bound(?tag)) }",tag,tag);

  biji_perform_update_async_and_free (query) ;
}

// removes the tag EVEN if files associated.
void
remove_tag_from_tracker(gchar *tag)
{
  gchar *value = tracker_str (tag);
  gchar *query = g_strdup_printf ("DELETE { ?tag a nao:Tag } \
  WHERE { ?tag nao:prefLabel '%s' }",value);

  biji_perform_update_async_and_free (query);
  g_free (tag);
}

void
push_existing_or_new_tag_to_note (gchar *tag,BijiNoteObj *note)
{
  gchar *url = get_note_url (note);
  gchar *query = g_strdup_printf (
            "INSERT {_:tag a nao:Tag ; nao:prefLabel '%s'. \
            ?unknown nao:hasTag _:tag} WHERE {?unknown nie:url '%s'}",
            tag, url);

  biji_perform_update_async_and_free (query);

  g_free (url);
}

/* This one is to be fixed */
void
remove_tag_from_note (gchar *tag, BijiNoteObj *note)
{
  gchar *url = get_note_url (note);
  gchar *query = g_strdup_printf ("DELETE { ?urn nao:hasTag ?label } \
                    WHERE { ?urn nie:url ?f . ?label nao:prefLabel '%s' .  \
                    FILTER (?f = '%s') }", tag, url);

  biji_perform_update_async_and_free (query);

  g_free (url);
}

void
biji_note_delete_from_tracker (BijiNoteObj *note)
{
  gchar *query = g_strdup_printf ("DELETE { <%s> a rdfs:Resource }",
                                        biji_note_obj_get_path(note));

  biji_perform_update_async_and_free (query);
}

void
bijiben_push_note_to_tracker (BijiNoteObj *note)
{
  gchar *title,*content,*file,*create_date,*last_change_date ;
    
  title = tracker_str (biji_note_obj_get_title (note));
  file = g_strdup_printf ("file://%s", biji_note_obj_get_path(note));
  create_date = to_8601_date (biji_note_obj_get_last_change_date (note));
  last_change_date = to_8601_date (biji_note_obj_get_last_change_date (note));
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
                           biji_note_obj_get_path(note),
                           file,
                           last_change_date,
                           create_date,
                           title,
                           content) ;

  biji_perform_update_async_and_free (query);

  g_free(title);
  g_free(file);
  g_free(content); 
  g_free(create_date);
  g_free(last_change_date);
}

