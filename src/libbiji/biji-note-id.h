/* bjb-note-id.h
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


#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>

#include "biji-info-set.h"
#include "biji-tracker.h"

G_BEGIN_DECLS

#define BIJI_TYPE_NOTE_ID (biji_note_id_get_type ())

G_DECLARE_FINAL_TYPE (BijiNoteID, biji_note_id, BIJI, NOTE_ID, GObject)

gboolean          biji_note_id_equal                              (BijiNoteID *a, BijiNoteID *b);


const gchar      *biji_note_id_get_path                           (BijiNoteID *note);


void              biji_note_id_set_title                          (BijiNoteID* n, gchar* title);


const gchar      *biji_note_id_get_title                          (BijiNoteID* n);


gboolean          biji_note_id_set_content                        (BijiNoteID *id, const gchar *content);


const gchar      *biji_note_id_get_content                        (BijiNoteID *id);


gint64            biji_note_id_get_mtime                          (BijiNoteID *n);


gboolean          biji_note_id_set_mtime                          (BijiNoteID* n, gint64 mtime);


gint64            biji_note_id_get_last_metadata_change_date      (BijiNoteID* n);


gboolean          biji_note_id_set_last_metadata_change_date      (BijiNoteID* n, gint64 mtime);


gint64            biji_note_id_get_create_date                    (BijiNoteID* n);


gboolean          biji_note_id_set_create_date                    (BijiNoteID* n, gint64 mtime);


BijiNoteID        *biji_note_id_new_from_info                     (BijiInfoSet *info);


G_END_DECLS
