/* bjb-note-view.h
 * Copyright © 2012, 2013 Pierre-Yves LUYTEN <py@luyten.fr>
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

#include <libbiji/libbiji.h>

G_BEGIN_DECLS

#define BJB_TYPE_NOTE_VIEW (bjb_note_view_get_type ())

G_DECLARE_FINAL_TYPE (BjbNoteView, bjb_note_view, BJB, NOTE_VIEW, GtkOverlay)

void                bjb_note_view_set_detached       (BjbNoteView *self,
                                                      gboolean     detached);

BjbNoteView        *bjb_note_view_new                (BijiNoteObj *note);

G_END_DECLS
