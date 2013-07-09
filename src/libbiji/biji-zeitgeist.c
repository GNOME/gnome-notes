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

#include "biji-zeitgeist.h"

#include <libbiji.h>
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


void 
insert_zeitgeist (BijiNoteObj *note, gchar *zg_interpretation)
{
  gchar *uri;
  ZeitgeistEvent     *event;
  ZeitgeistSubject   *subject ;
  ZeitgeistLog       *log;

  log = biji_note_book_get_zg_log (biji_item_get_book (BIJI_ITEM (note)));
  uri = g_strdup_printf ("file://%s", biji_item_get_uuid (BIJI_ITEM (note)));

  subject = zeitgeist_subject_new_full (uri,
                                        ZEITGEIST_NFO_DOCUMENT,
                                        ZEITGEIST_NFO_FILE_DATA_OBJECT,
                                        "application/x-note",
                                        "",
                                        biji_item_get_title (BIJI_ITEM (note)),
                                        "");

  event = zeitgeist_event_new_full (zg_interpretation,
                                    ZEITGEIST_ZG_USER_ACTIVITY,
                                    "application://bijiben.desktop",
                                    "",
                                    subject);

  zeitgeist_log_insert_event_no_reply (log, event, NULL);
  g_free (uri);
}
