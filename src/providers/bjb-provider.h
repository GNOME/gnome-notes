/* bjb-provider.h
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

#include <glib-object.h>
#include <gio/gio.h>

#include "../items/bjb-tag-store.h"
#include "../items/bjb-item.h"

G_BEGIN_DECLS

#define BJB_TYPE_PROVIDER (bjb_provider_get_type ())
G_DECLARE_DERIVABLE_TYPE (BjbProvider, bjb_provider, BJB, PROVIDER, GObject)

struct _BjbProviderClass
{
  GObjectClass parent_class;

  const char  *(*get_uid)              (BjbProvider          *self);
  const char  *(*get_name)             (BjbProvider          *self);
  GIcon       *(*get_icon)             (BjbProvider          *self,
                                        GError              **error);
  gboolean     (*get_rgba)             (BjbProvider          *self,
                                        GdkRGBA              *rgba);
  const char  *(*get_domain)           (BjbProvider          *self);
  const char  *(*get_user_name)        (BjbProvider          *self);
  const char  *(*get_location_name)    (BjbProvider          *self);
  BjbStatus    (*get_status)           (BjbProvider          *self);

  GListModel  *(*get_notes)            (BjbProvider          *self);
  GListModel  *(*get_trash_notes)      (BjbProvider          *self);

  void         (*connect_async)        (BjbProvider          *self,
                                        GCancellable         *cancellable,
                                        GAsyncReadyCallback   callback,
                                        gpointer              user_data);
  gboolean     (*connect_finish)       (BjbProvider          *self,
                                        GAsyncResult         *result,
                                        GError              **error);
  void         (*save_item_async)      (BjbProvider          *self,
                                        BjbItem              *item,
                                        GCancellable         *cancellable,
                                        GAsyncReadyCallback   callback,
                                        gpointer              user_data);
  gboolean     (*save_item_finish)     (BjbProvider          *self,
                                        GAsyncResult         *result,
                                        GError              **error);
  void         (*restore_item_async)   (BjbProvider          *self,
                                        BjbItem              *item,
                                        GCancellable         *cancellable,
                                        GAsyncReadyCallback   callback,
                                        gpointer              user_data);
  gboolean     (*restore_item_finish)  (BjbProvider          *self,
                                        GAsyncResult         *result,
                                        GError              **error);
  void         (*delete_item_async)    (BjbProvider          *self,
                                        BjbItem              *item,
                                        GCancellable         *cancellable,
                                        GAsyncReadyCallback   callback,
                                        gpointer              user_data);
  gboolean     (*delete_item_finish)   (BjbProvider          *self,
                                        GAsyncResult         *result,
                                        GError              **error);
};

const char  *bjb_provider_get_uid              (BjbProvider          *self);
const char  *bjb_provider_get_name             (BjbProvider          *self);
GIcon       *bjb_provider_get_icon             (BjbProvider          *self,
                                                GError              **error);
gboolean     bjb_provider_get_rgba             (BjbProvider          *self,
                                                GdkRGBA              *rgba);
const char  *bjb_provider_get_domain           (BjbProvider          *self);
const char  *bjb_provider_get_user_name        (BjbProvider          *self);
const char  *bjb_provider_get_location_name    (BjbProvider          *self);

BjbStatus    bjb_provider_get_status           (BjbProvider          *self);
gboolean     bjb_provider_get_loaded           (BjbProvider          *self);
void         bjb_provider_set_loaded           (BjbProvider          *self,
                                                gboolean              loaded);

GListModel  *bjb_provider_get_notes            (BjbProvider          *self);
GListModel  *bjb_provider_get_trash_notes      (BjbProvider          *self);
BjbTagStore *bjb_provider_get_tag_store        (BjbProvider          *self);

void         bjb_provider_connect_async        (BjbProvider          *self,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
gboolean     bjb_provider_connect_finish       (BjbProvider          *self,
                                                GAsyncResult         *result,
                                                GError              **error);
void         bjb_provider_save_item_async      (BjbProvider          *self,
                                                BjbItem              *item,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
gboolean     bjb_provider_save_item_finish     (BjbProvider          *self,
                                                GAsyncResult         *result,
                                                GError              **error);
void         bjb_provider_restore_item_async   (BjbProvider          *self,
                                                BjbItem              *item,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
gboolean     bjb_provider_restore_item_finish  (BjbProvider          *self,
                                                GAsyncResult         *result,
                                                GError              **error);
void         bjb_provider_delete_item_async    (BjbProvider          *self,
                                                BjbItem              *item,
                                                GCancellable         *cancellable,
                                                GAsyncReadyCallback   callback,
                                                gpointer              user_data);
gboolean     bjb_provider_delete_item_finish   (BjbProvider          *self,
                                                GAsyncResult         *result,
                                                GError              **error);
G_END_DECLS
