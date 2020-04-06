/*
 * bijiben
 * Copyright (C) Pierre-Yves Luyten 2012, 2013 <py@luyten.fr>
 * 
bijiben is free software: you can redistribute it and/or modify it
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

#include "bjb-main-view.h"

G_BEGIN_DECLS

#define BJB_TYPE_MAIN_TOOLBAR (bjb_main_toolbar_get_type ())

G_DECLARE_FINAL_TYPE (BjbMainToolbar, bjb_main_toolbar, BJB, MAIN_TOOLBAR, GtkHeaderBar)

BjbMainToolbar        *bjb_main_toolbar_new                       (BjbMainView *parent,
                                                                   BjbController *controller);

void                   bjb_main_toolbar_title_focus               (BjbMainToolbar *self);

void                   bjb_main_toolbar_open_menu                 (BjbMainToolbar *self);

G_END_DECLS
