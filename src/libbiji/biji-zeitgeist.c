/* biji-zeitgeist.c
 * Copyright (C) Pierre-Yves LUYTEN 2011 <py@luyten.fr>
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.*/


#ifdef BUILD_ZEITGEIST

#include <libbiji.h>
#include "biji-manager.h"
#include "biji-zeitgeist.h"


ZeitgeistLog *
biji_zeitgeist_init (void)
{
  GPtrArray *ptr_arr;
  ZeitgeistEvent *event;
  ZeitgeistDataSource *ds;
  ZeitgeistDataSourceRegistry *zg_dsr = NULL;
  ZeitgeistLog *log;


  log = zeitgeist_log_new ();
  event = zeitgeist_event_new_full (
    NULL, NULL, "application://bijiben.desktop", NULL, NULL);

  ptr_arr = g_ptr_array_new ();
  g_ptr_array_add (ptr_arr, event);

  ds = zeitgeist_data_source_new_full ("org.gnome.bijiben,dataprovider",
                                       "Notes dataprovider",
                                       "Logs events about accessed notes",
                                       ptr_arr),

  zg_dsr = zeitgeist_data_source_registry_new ();
  zeitgeist_data_source_registry_register_data_source (zg_dsr, ds,
                                                       NULL, NULL, NULL);
  g_ptr_array_set_free_func (ptr_arr, g_object_unref);
  g_ptr_array_unref (ptr_arr);

  return log;
}


static void
on_find_create_event (GObject *log,
                      GAsyncResult *res,
                      gpointer user_data)
{
  GError             *error;
  ZeitgeistResultSet *events;

  error = NULL;
  events = zeitgeist_log_find_events_finish (ZEITGEIST_LOG (log), res, &error);

  if (error)
  {
    g_message ("Error reading results: %s", error->message);
    g_error_free (error);
    return;
  }


  if (zeitgeist_result_set_size (events) == 0)
    insert_zeitgeist (BIJI_NOTE_OBJ (user_data), ZEITGEIST_ZG_CREATE_EVENT);

  g_object_unref (events);
}


static void
check_insert_create_zeitgeist (BijiNoteObj *note)
{
  gchar *uri;
  ZeitgeistLog       *log;
  GPtrArray          *templates;
  ZeitgeistEvent     *event;
  ZeitgeistSubject   *subject;
  
  uri = g_strdup_printf ("file://%s", biji_item_get_uuid (BIJI_ITEM (note)));
  log = biji_manager_get_zg_log (biji_item_get_manager (BIJI_ITEM (note)));
  
  templates = g_ptr_array_new ();
  event = zeitgeist_event_new_full (ZEITGEIST_ZG_CREATE_EVENT, 
                                    NULL,
                                    "application://bijiben.desktop",
                                    NULL, NULL);
  subject = zeitgeist_subject_new ();
  zeitgeist_subject_set_uri (subject, uri);
  zeitgeist_event_add_subject (event, subject);
  g_ptr_array_add (templates, event);
  
  zeitgeist_log_find_events (log,
                             zeitgeist_time_range_new_to_now (),
                             templates,
                             ZEITGEIST_STORAGE_STATE_ANY,
                             10,
                             ZEITGEIST_RESULT_TYPE_LEAST_RECENT_EVENTS,
                             NULL,
                             (GAsyncReadyCallback) on_find_create_event,
                             note);
}


void
insert_zeitgeist (BijiNoteObj *note,
                  gchar *zg_interpretation)
{
  gchar *uri;
  const gchar *title;
  ZeitgeistEvent     *event;
  ZeitgeistSubject   *subject;
  ZeitgeistLog       *log;

  /* Make sure that only notes with a title log their events.
  If a note is closed without a title, it is deleted. This
  section prevents the ACCESS_EVENT being called immediately
  after the note is created and the note is empty */

  title = biji_item_get_title (BIJI_ITEM (note));
  if (title == NULL ||
      g_utf8_strlen (title, -1) <= 0)
    return;

  /* Insert requested log */

  log = biji_manager_get_zg_log (biji_item_get_manager (BIJI_ITEM (note)));
  uri = g_strdup_printf ("file://%s", biji_item_get_uuid (BIJI_ITEM (note)));

  subject = zeitgeist_subject_new_full (uri,
                                        ZEITGEIST_NFO_DOCUMENT,
                                        ZEITGEIST_NFO_FILE_DATA_OBJECT,
                                        "application/x-note",
                                        "",
                                        title,
                                        "");

  event = zeitgeist_event_new_full (zg_interpretation,
                                    ZEITGEIST_ZG_USER_ACTIVITY,
                                    "application://bijiben.desktop",
                                    "",
                                    subject,
                                    NULL);


  if (g_strcmp0 (zg_interpretation, ZEITGEIST_ZG_CREATE_EVENT) ==0)
    zeitgeist_event_set_timestamp (event,
                                   biji_note_obj_get_create_date (note)/1000);

  zeitgeist_log_insert_event_no_reply (log, event, NULL);
  g_free (uri);


  /* 
   * Check if the note
   * was already created into zeitgeist
   */

  if (g_strcmp0 (zg_interpretation, ZEITGEIST_ZG_MODIFY_EVENT) ==0)
    check_insert_create_zeitgeist (note);
}

#endif /* BUILD_ZEITGEIST */
