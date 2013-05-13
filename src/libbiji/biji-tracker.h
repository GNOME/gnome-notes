/* biji-tracker.h
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


#ifndef _BIJI_TRACKER_H
#define _BIJI_TRACKER_H

#include <gtk/gtk.h>
#include <tracker-sparql.h>

#include "libbiji.h"

typedef enum
{
  BIJI_URN_COL,
  BIJI_TITLE_COL,
  BIJI_MTIME_COL,
  BIJI_NO_COL
} BijiTrackerColumns;


typedef struct
{
  gchar    *urn;
  gchar    *title;
  gchar    *mtime;

} BijiTrackerInfoSet;


typedef void (*BijiFunc) (gpointer user_data);

typedef void (*BijiCallback) (BijiItem *item, gpointer user_data);




GList * biji_get_items_with_collection_finish (GObject *source_object,
                                               GAsyncResult *res,
                                               BijiNoteBook *book);

void  biji_get_items_with_collection_async (const gchar *needle,
                                            GAsyncReadyCallback f,
                                            gpointer user_data);

/* All notes matching (either content or collections) */
GList * biji_get_notes_with_strings_or_collection_finish (GObject *source_object,
                                                          GAsyncResult *res,
                                                          BijiNoteBook *book);

void biji_get_notes_with_string_or_collection_async (gchar *needle,
                                                     GAsyncReadyCallback f,
                                                     gpointer user_data);

/* Collections */

/* The URN is the... value. Collection _title_ is the key.*/
GHashTable * biji_get_all_collections_finish (GObject *source_object, GAsyncResult *res);

void biji_get_all_collections_async (GAsyncReadyCallback f, gpointer user_data);

void biji_create_new_collection_async (BijiNoteBook *book, const gchar *tag, BijiCallback afterward, gpointer user_data);

void biji_remove_collection_from_tracker (const gchar *urn);

// when adding an existing collection, use the collection title
void biji_push_existing_collection_to_note (BijiNoteObj *note,
                                            gchar       *title,
                                            BijiFunc     callback,
                                            gpointer     user_data);

// when removing, use the urn
void biji_remove_collection_from_note (BijiNoteObj *note, gchar *urn);

/* Insert or update */
void bijiben_push_note_to_tracker(BijiNoteObj *note);

void biji_note_delete_from_tracker(BijiNoteObj *note);

#endif /*_BIJI_TRACKER_H*/
