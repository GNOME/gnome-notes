/*
 * bjb-list-view.h
 * Copyright 2018 Isaque Galdino <igaldino@gmail.com>
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
#include <bjb-controller.h>

G_BEGIN_DECLS

#define BJB_TYPE_LIST_VIEW (bjb_list_view_get_type ())

G_DECLARE_FINAL_TYPE (BjbListView, bjb_list_view, BJB, LIST_VIEW, GtkScrolledWindow)

BjbListView   *bjb_list_view_new                (void);

void           bjb_list_view_setup              (BjbListView   *self,
                                                 BjbController *controller);

void           bjb_list_view_update             (BjbListView   *self);

GtkListBox    *bjb_list_view_get_list_box       (BjbListView   *self);

BjbController *bjb_list_view_get_controller     (BjbListView   *self);

G_END_DECLS

