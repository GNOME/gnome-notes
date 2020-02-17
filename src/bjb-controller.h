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

typedef enum {
  BJB_MODEL_COLUMN_UUID,
  BJB_MODEL_COLUMN_TITLE,
  BJB_MODEL_COLUMN_TEXT,
  BJB_MODEL_COLUMN_MTIME,
  BJB_MODEL_COLUMN_SELECTED,
  BJB_MODEL_COLUMN_LAST
} BjbModelColumnsType;

BjbController * bjb_controller_new (BijiManager  *manager,
                                    GtkWindow     *bjb_window_base,
                                    gchar         *needle);

void bjb_controller_apply_needle (BjbController *self);

void bjb_controller_update_view (BjbController *self);

void bjb_controller_set_manager (BjbController * self, BijiManager * manager ) ;

void bjb_controller_set_needle (BjbController *self, const gchar *needle ) ; 

gchar * bjb_controller_get_needle (BjbController *self ) ;

GtkTreeModel * bjb_controller_get_model  (BjbController *self) ;

void bjb_controller_disconnect (BjbController *self);

gboolean bjb_controller_shows_item (BjbController *self);

BijiNotebook * bjb_controller_get_notebook (BjbController *self);

void bjb_controller_set_notebook (BjbController *self, BijiNotebook *coll);


BijiItemsGroup bjb_controller_get_group (BjbController *controller);


void bjb_controller_set_group (BjbController   *self,
                               BijiItemsGroup   group);

void bjb_controller_show_more (BjbController *controller);


gboolean bjb_controller_get_remaining_items (BjbController *self);

gboolean  bjb_controller_get_selection_mode (BjbController *self);

void      bjb_controller_set_selection_mode (BjbController *self,
                                             gboolean       selection_mode);

GList    *bjb_controller_get_selection      (BjbController *self);

void      bjb_controller_select_all         (BjbController *self);

void      bjb_controller_unselect_all       (BjbController *self);

G_END_DECLS
