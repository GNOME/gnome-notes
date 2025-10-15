/* bjb-settings.h
 *
 * Copyright © 2012, 2013 Pierre-Yves LUYTEN <py@luyten.fr>
 * Copyright 2021 Mohammed Sadiq <sadiq@sadiqpk.org>
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
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <glib-object.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BJB_TYPE_SETTINGS (bjb_settings_get_type ())

G_DECLARE_FINAL_TYPE (BjbSettings, bjb_settings, BJB, SETTINGS, GObject)

typedef enum {
  BJB_TEXT_SIZE_LARGE,
  BJB_TEXT_SIZE_MEDIUM,
  BJB_TEXT_SIZE_SMALL
} BjbTextSizeType;

BjbSettings     *bjb_settings_new                    (void);
BjbSettings     *bjb_settings_get_default            (void);
char            *bjb_settings_get_font               (BjbSettings  *self);
const char      *bjb_settings_get_custom_font        (BjbSettings  *self);
const char      *bjb_settings_get_default_color      (BjbSettings  *self);
const char      *bjb_settings_get_default_location   (BjbSettings  *self);
BjbTextSizeType  bjb_settings_get_text_size          (BjbSettings  *self);
GAction         *bjb_settings_get_text_size_gaction  (BjbSettings  *self);
void             bjb_settings_set_last_opened_item   (BjbSettings  *self,
                                                      const char   *note_path);
char            *bjb_settings_get_last_opened_item   (BjbSettings  *self);
void             show_bijiben_settings_window        (GtkWidget    *parent_window);

G_END_DECLS
