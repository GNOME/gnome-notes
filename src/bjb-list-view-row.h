/*
 * bjb-list-view-row.h
 * Copyright 2020 Isaque Galdino <igaldino@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#pragma once

#include <gtk/gtk.h>
#include "items/bjb-note.h"

G_BEGIN_DECLS

#define BJB_TYPE_LIST_VIEW_ROW (bjb_list_view_row_get_type ())

G_DECLARE_FINAL_TYPE (BjbListViewRow, bjb_list_view_row, BJB, LIST_VIEW_ROW, GtkListBoxRow)

BjbListViewRow *bjb_list_view_row_new                (void);
GtkWidget      *bjb_list_view_row_new_with_note      (BjbNote *note);
BjbItem        *bjb_list_view_row_get_note           (BjbListViewRow *self);

G_END_DECLS
