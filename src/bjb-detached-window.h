/*
 * bjb-detached-window.h
 * Copyright (C) 2020 Jonathan Kang <jonathankang@gnome.org>
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

#include <gtk/gtk.h>
#include <handy.h>
#include <libbiji/libbiji.h>

#include "bjb-note-view.h"
#include "bjb-window-base.h"

G_BEGIN_DECLS

#define BJB_TYPE_DETACHED_WINDOW (bjb_detached_window_get_type ())

G_DECLARE_FINAL_TYPE (BjbDetachedWindow, bjb_detached_window, BJB, DETACHED_WINDOW, HdyApplicationWindow)

BjbDetachedWindow *bjb_detached_window_new (BjbNoteView   *note_view,
                                            BijiNoteObj   *note,
                                            int            width,
                                            int            height,
                                            BjbWindowBase *main_win);

G_END_DECLS
