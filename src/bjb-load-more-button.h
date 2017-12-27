/*
 * Bjb - access, organize and share your bjb on GNOME
 * Copyright © 2013 Red Hat, Inc.
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

/* Chained based on Photos, Documents */

#pragma once

#include <gtk/gtk.h>

#include "bjb-controller.h"

G_BEGIN_DECLS

#define BJB_TYPE_LOAD_MORE_BUTTON (bjb_load_more_button_get_type ())

G_DECLARE_FINAL_TYPE (BjbLoadMoreButton, bjb_load_more_button, BJB, LOAD_MORE_BUTTON, GtkButton)


              /* Does not return self, but its revealer */

GtkWidget             *bjb_load_more_button_new                    (BjbController *controller);


GtkWidget             *bjb_load_more_button_get_revealer           (BjbLoadMoreButton* button);


void                   bjb_load_more_button_set_block              (BjbLoadMoreButton *button, gboolean block);

G_END_DECLS
