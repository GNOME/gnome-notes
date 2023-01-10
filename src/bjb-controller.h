/*
 * bjb-controller.h
 * Copyright (C) Pierre-Yves Luyten 2012, 2013 <py@luyten.fr>
 * Copyright 2017 Mohammed Sadiq <sadiq@sadiqpk.org>
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
#include <libbiji/libbiji.h>

G_BEGIN_DECLS

#define BJB_TYPE_CONTROLLER (bjb_controller_get_type ())

G_DECLARE_FINAL_TYPE (BjbController, bjb_controller, BJB, CONTROLLER, GObject)

BjbController * bjb_controller_new (BijiManager  *manager,
                                    GtkWindow     *bjb_window_base);
void bjb_controller_set_manager (BjbController * self, BijiManager * manager ) ;
GListModel   *bjb_controller_get_notes (BjbController *self);
BijiItemsGroup bjb_controller_get_group (BjbController *controller);
void bjb_controller_set_group (BjbController   *self,
                               BijiItemsGroup   group);

G_END_DECLS
