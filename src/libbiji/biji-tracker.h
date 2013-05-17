/* biji-tracker.h
 * Copyright (C) Pierre-Yves LUYTEN 2012,2013 <py@luyten.fr>
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


#ifndef _BIJI_TRACKER_H
#define _BIJI_TRACKER_H

#include <gtk/gtk.h>
#include <tracker-sparql.h>

#include "libbiji.h"



typedef struct
{
  gchar    *urn;
  gchar    *title;
  gchar    *mtime;

} BijiTrackerInfoSet;


/* All possible query return
 * Free the containers for list & hash */


typedef void       (*BijiBoolCallback)          (gboolean result, gpointer user_data);


typedef void       (*BijiItemCallback)          (BijiItem *item, gpointer user_data);


typedef void       (*BijiItemsListCallback)     (GList *items, gpointer user_data);


typedef void       (*BijiInfoSetsHCallback)     (GHashTable *info_sets, gpointer user_data);



void        biji_get_items_with_collection_async       (BijiNoteBook *book,
                                                        const gchar *needle,
                                                        BijiItemsListCallback cb,
                                                        gpointer user_data);


void        biji_get_items_matching_async              (BijiNoteBook *book,
                                                        gchar *needle,
                                                        BijiItemsListCallback cb,
                                                        gpointer user_data);



void        biji_get_all_collections_async             (BijiNoteBook *book,
                                                        BijiInfoSetsHCallback cb,
                                                        gpointer user_data);



void        biji_create_new_collection_async           (BijiNoteBook *book,
                                                        const gchar *tag,
                                                        BijiItemCallback afterward,
                                                        gpointer user_data);



void        biji_remove_collection_from_tracker        (const gchar *urn);




void        biji_push_existing_collection_to_note      (BijiNoteObj      *note,
                                                        gchar            *title,
                                                        BijiBoolCallback  bool_cb,
                                                        gpointer          user_data);



void        biji_remove_collection_from_note           (BijiNoteObj      *note,
                                                        BijiItem         *coll,
                                                        BijiBoolCallback  bool_cb,
                                                        gpointer          user_data);


                       /* Either insert or update */

void        bijiben_push_note_to_tracker               (BijiNoteObj *note);



void        biji_note_delete_from_tracker              (BijiNoteObj *note);



#endif /*_BIJI_TRACKER_H*/
