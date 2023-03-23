/* bjb-note-list.h
 *
 * Copyright 2023 Purism SPC
 * Copyright 2023 Mohammed Sadiq <sadiq@sadiqpk.org>
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
 *
 * Author(s):
 *   Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "items/bjb-item.h"

G_BEGIN_DECLS

#define BJB_TYPE_NOTE_LIST (bjb_note_list_get_type ())
G_DECLARE_FINAL_TYPE (BjbNoteList, bjb_note_list, BJB, NOTE_LIST, GtkBox)


void         bjb_note_list_set_model           (BjbNoteList  *self,
                                                GListModel   *model);
BjbItem     *bjb_note_list_get_selected_note   (BjbNoteList  *self);
void         bjb_note_list_set_notebook        (BjbNoteList  *self,
                                                BjbItem      *notebook);
void         bjb_note_list_show_trash          (BjbNoteList  *self,
                                                gboolean      show_trash);

G_END_DECLS
