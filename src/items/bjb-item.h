/* bjb-item.h
 *
 * Copyright 2023 Mohammed Sadiq <sadiq@sadiqpk.org>
 * Copyright 2023 Purism SPC
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
 * Author(s):
 *   Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gdk/gdk.h>

#include "../bjb-enums.h"

G_BEGIN_DECLS

#define BJB_TYPE_ITEM (bjb_item_get_type ())
G_DECLARE_DERIVABLE_TYPE (BjbItem, bjb_item, BJB, ITEM, GObject)

struct _BjbItemClass
{
  GObjectClass   parent_class;

  gboolean      (*is_modified)     (BjbItem     *self);
  void          (*unset_modified)  (BjbItem     *self);
  gboolean      (*match)           (BjbItem     *self,
                                    const char  *needle);
  BjbFeature    (*get_features)    (BjbItem     *self);
};

const char    *bjb_item_get_uid              (BjbItem        *self);
void           bjb_item_set_uid              (BjbItem        *self,
                                              const char     *uid);
const char    *bjb_item_get_title            (BjbItem        *self);
void           bjb_item_set_title            (BjbItem        *self,
                                              const char     *title);
gboolean       bjb_item_get_rgba             (BjbItem        *self,
                                              GdkRGBA       *color);
void           bjb_item_set_rgba             (BjbItem        *self,
                                              const GdkRGBA *color);
gint64         bjb_item_get_create_time      (BjbItem        *self);
void           bjb_item_set_create_time      (BjbItem        *self,
                                              gint64          time);
gint64         bjb_item_get_mtime            (BjbItem        *self);
void           bjb_item_set_mtime            (BjbItem        *self,
                                              gint64          time);
gint64         bjb_item_get_meta_mtime       (BjbItem        *self);
void           bjb_item_set_meta_mtime       (BjbItem        *self,
                                              gint64          time);
gboolean       bjb_item_is_trashed           (BjbItem        *self);
void           bjb_item_set_is_trashed       (BjbItem        *self,
                                              gboolean        is_trashed);
gboolean       bjb_item_is_modified          (BjbItem        *self);
void           bjb_item_set_modified         (BjbItem        *self);
void           bjb_item_unset_modified       (BjbItem        *self);
gboolean       bjb_item_is_new               (BjbItem        *self);
int            bjb_item_compare              (gconstpointer   a,
                                              gconstpointer   b,
                                              gpointer        user_data);;
gboolean       bjb_item_match                (BjbItem        *self,
                                              const char     *needle);
BjbFeature     bjb_item_get_features         (BjbItem        *self);

G_END_DECLS
