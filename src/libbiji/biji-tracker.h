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
#include "biji-info-set.h"


/* All possible query return
 * Free the containers for list & hash<BijiInfoSets> */


typedef void       (*BijiBoolCallback)          (gboolean result, gpointer user_data);


typedef void       (*BijiItemCallback)          (BijiItem *item, gpointer user_data);


typedef void       (*BijiItemsListCallback)     (GList *items, gpointer user_data);


typedef void       (*BijiInfoSetsHCallback)     (GHashTable *info_sets, gpointer user_data);


/* CALLER IS RESPONSIBLE FOR FREEING INFO SET */

typedef void       (*BijiInfoCallback)          (BijiInfoSet *info, gpointer user_data);



void        biji_get_items_with_notebook_async       (BijiManager *manager,
                                                        const gchar *needle,
                                                        BijiItemsListCallback cb,
                                                        gpointer user_data);


void        biji_get_items_matching_async              (BijiManager *manager,
                                                        BijiItemsGroup group,
                                                        gchar *needle,
                                                        BijiItemsListCallback cb,
                                                        gpointer user_data);



void        biji_get_all_notebooks_async             (BijiManager *manager,
                                                        BijiInfoSetsHCallback cb,
                                                        gpointer user_data);



void        biji_create_new_notebook_async           (BijiManager *manager,
                                                        const gchar *tag,
                                                        BijiItemCallback afterward,
                                                        gpointer user_data);



void        biji_remove_notebook_from_tracker        (BijiManager *manager,
                                                        const gchar *urn);




void        biji_push_existing_notebook_to_note      (BijiNoteObj      *note,
                                                        gchar            *title,
                                                        BijiBoolCallback  bool_cb,
                                                        gpointer          user_data);



void        biji_remove_notebook_from_note           (BijiNoteObj      *note,
                                                        BijiItem         *coll,
                                                        BijiBoolCallback  bool_cb,
                                                        gpointer          user_data);



void        biji_note_delete_from_tracker              (BijiNoteObj *note);


void        biji_tracker_trash_ressource               (BijiManager *manager,
                                                        gchar *tracker_urn);


void        biji_tracker_ensure_ressource_from_info    (BijiManager     *manager,
                                                        BijiInfoSet *info);


void        biji_tracker_ensure_datasource             (BijiManager *manager, 
                                                        const gchar *datasource_id,
                                                        const gchar *identifier,
                                                        BijiBoolCallback cb,
                                                        gpointer user_data);


void        biji_tracker_check_for_info                (BijiManager *manager, 
                                                        gchar *url,
                                                        gint64 mtime,
                                                        BijiInfoCallback callback,
                                                        gpointer user_data);

#endif /*_BIJI_TRACKER_H*/
