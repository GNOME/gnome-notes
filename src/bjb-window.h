/* bjb-window.h
 *
 * Copyright 2012, 2013 Pierre-Yves Luyten <py@luyten.fr>
 * Copyright 2020 Jonathan Kang <jonathankang@gnome.org>
 * Copyright 2021 Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


#pragma once

#include <gtk/gtk.h>
#include <handy.h>

#define BJB_TYPE_WINDOW (bjb_window_get_type ())

G_DECLARE_FINAL_TYPE (BjbWindow, bjb_window, BJB, WINDOW, HdyApplicationWindow)

GtkWidget       *bjb_window_new            (void);
gboolean         bjb_window_get_is_main    (BjbWindow     *self);
void             bjb_window_set_is_main    (BjbWindow     *self,
                                            gboolean       is_main);
BjbNote         *bjb_window_get_note       (BjbWindow     *self);
void             bjb_window_set_note       (BjbWindow     *self,
                                            BjbNote       *note);
