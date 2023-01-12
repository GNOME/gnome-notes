/* -*- mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* biji-tracker.h
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


#pragma once

#include <glib-object.h>
#include <tracker-sparql.h>

#include "biji-manager.h"

G_BEGIN_DECLS

#define BIJI_TYPE_TRACKER (biji_tracker_get_type ())

G_DECLARE_FINAL_TYPE (BijiTracker, biji_tracker, BIJI, TRACKER, GObject)

BijiTracker *biji_tracker_new                            (BijiManager         *manager);
gboolean    biji_tracker_is_available                    (BijiTracker         *self);
TrackerSparqlConnection *
            biji_tracker_get_connection                  (BijiTracker         *self);
void        biji_tracker_add_notebook_async              (BijiTracker         *self,
                                                          const char          *notebook,
                                                          GAsyncReadyCallback  callback,
                                                          gpointer             user_data);
BjbItem    *biji_tracker_add_notebook_finish             (BijiTracker         *self,
                                                          GAsyncResult        *result,
                                                          GError             **error);
void        biji_tracker_remove_notebook                 (BijiTracker         *self,
                                                          const char          *notebook_urn);
void        biji_tracker_get_notes_async                 (BijiTracker         *self,
                                                          BijiItemsGroup       group,
                                                          const char          *needle,
                                                          GAsyncReadyCallback  callback,
                                                          gpointer             user_data);
GList      *biji_tracker_get_notes_finish                (BijiTracker         *self,
                                                          GAsyncResult        *result,
                                                          GError             **error);
void        biji_tracker_get_notebooks_async             (BijiTracker         *self,
                                                          GAsyncReadyCallback  callback,
                                                          gpointer             user_data);
GHashTable *biji_tracker_get_notebooks_finish            (BijiTracker         *self,
                                                          GAsyncResult        *result,
                                                          GError             **error);
void        biji_tracker_remove_note_notebook_async      (BijiTracker         *self,
                                                          BijiNoteObj         *note,
                                                          BjbItem             *notebook,
                                                          GAsyncReadyCallback  callback,
                                                          gpointer             user_data);
gboolean    biji_tracker_remove_note_notebook_finish     (BijiTracker         *self,
                                                          GAsyncResult        *result,
                                                          GError             **error);
void        biji_tracker_add_note_to_notebook_async      (BijiTracker         *self,
                                                          BijiNoteObj         *note,
                                                          const char          *notebook,
                                                          GAsyncReadyCallback  callback,
                                                          gpointer             user_data);
gboolean    biji_tracker_add_note_to_notebook_finish     (BijiTracker         *self,
                                                          GAsyncResult        *result,
                                                          GError             **error);
void        biji_tracker_get_notes_with_notebook_async   (BijiTracker         *self,
                                                          const char          *notebooks,
                                                          GAsyncReadyCallback  callback,
                                                          gpointer             user_data);
GList      *biji_tracker_get_notes_with_notebook_finish  (BijiTracker         *self,
                                                          GAsyncResult        *result,
                                                          GError             **error);
void        biji_tracker_delete_note                     (BijiTracker         *self,
                                                          BijiNoteObj         *note);
void        biji_tracker_trash_resource                  (BijiTracker          *self,
                                                          const char          *tracker_urn);
void        biji_tracker_save_note                       (BijiTracker         *self,
                                                          BijiInfoSet         *info);

G_END_DECLS
