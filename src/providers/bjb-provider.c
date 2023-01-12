/* bjb-provider.c
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

#define G_LOG_DOMAIN "bjb-provider"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "bjb-provider.h"

typedef struct
{
  BjbTagStore *tag_store;
  gboolean     connected;
  gboolean     loaded;
} BjbProviderPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BjbProvider, bjb_provider, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_UID,
  PROP_NAME,
  PROP_ICON,
  PROP_STATUS,
  PROP_LOADED,
  PROP_DOMAIN,
  PROP_USER_NAME,
  N_PROPS
};

enum {
  ITEM_REMOVED,
  N_SIGNALS
};

static GParamSpec *properties[N_PROPS];
static guint signals[N_SIGNALS];

static const char *
bjb_provider_real_get_name (BjbProvider *self)
{
  g_assert (BJB_IS_PROVIDER (self));

  return "";
}

static GIcon *
bjb_provider_real_get_icon (BjbProvider  *self,
                            GError      **error)
{
  g_assert (BJB_IS_PROVIDER (self));

  return NULL;
}

static gboolean
bjb_provider_real_get_rgba (BjbProvider *self,
                            GdkRGBA     *rgba)
{
  g_assert (BJB_IS_PROVIDER (self));

  return FALSE;
}

static const char *
bjb_provider_real_get_location_name (BjbProvider *self)
{
  g_assert (BJB_IS_PROVIDER (self));

  return "";
}

static BjbStatus
bjb_provider_real_get_status (BjbProvider *self)
{
  g_assert (BJB_IS_PROVIDER (self));

  return BJB_STATUS_UNKNOWN;
}

static GListModel *
bjb_provider_real_get_notes (BjbProvider *self)
{
  g_assert (BJB_IS_PROVIDER (self));

  return NULL;
}

static GListModel *
bjb_provider_real_get_trash_notes (BjbProvider *self)
{
  g_assert (BJB_IS_PROVIDER (self));

  return NULL;
}

static void
bjb_provider_real_connect_async (BjbProvider         *self,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data)
{
  g_task_report_new_error (self, callback, user_data,
                           bjb_provider_real_connect_async,
                           G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           "Connecting items asynchronously not supported");
}

static gboolean
bjb_provider_real_connect_finish (BjbProvider   *self,
                                  GAsyncResult  *result,
                                  GError       **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
bjb_provider_real_save_item_async (BjbProvider         *self,
                                   BjbItem             *item,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  g_task_report_new_error (self, callback, user_data,
                           bjb_provider_real_save_item_async,
                           G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           "Saving item asynchronously not supported");
}

static gboolean
bjb_provider_real_save_item_finish (BjbProvider   *self,
                                    GAsyncResult  *result,
                                    GError       **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
bjb_provider_real_restore_item_async (BjbProvider         *self,
                                      BjbItem             *item,
                                      GCancellable        *cancellable,
                                      GAsyncReadyCallback  callback,
                                      gpointer             user_data)
{
  g_task_report_new_error (self, callback, user_data,
                           bjb_provider_real_restore_item_async,
                           G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           "Restoring item asynchronously not supported");
}

static gboolean
bjb_provider_real_restore_item_finish (BjbProvider   *self,
                                       GAsyncResult  *result,
                                       GError       **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
bjb_provider_real_delete_item_async (BjbProvider         *self,
                                     BjbItem             *item,
                                     GCancellable        *cancellable,
                                     GAsyncReadyCallback  callback,
                                     gpointer             user_data)
{
  g_task_report_new_error (self, callback, user_data,
                           bjb_provider_real_delete_item_async,
                           G_IO_ERROR,
                           G_IO_ERROR_NOT_SUPPORTED,
                           "Deleting item asynchronously not supported");
}

static gboolean
bjb_provider_real_delete_item_finish (BjbProvider   *self,
                                      GAsyncResult  *result,
                                      GError       **error)
{
  return g_task_propagate_boolean (G_TASK (result), error);
}

static void
bjb_provider_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BjbProvider *self = (BjbProvider *)object;

  switch (prop_id)
    {
    case PROP_UID:
      g_value_set_string (value, bjb_provider_get_uid (self));
      break;

    case PROP_NAME:
      g_value_set_string (value, bjb_provider_get_name (self));
      break;

    case PROP_ICON:
      g_value_set_object (value, bjb_provider_get_icon (self, NULL));
      break;

    case PROP_STATUS:
      g_value_set_int (value, bjb_provider_get_status (self));
      break;

    case PROP_LOADED:
      g_value_set_boolean (value, bjb_provider_get_loaded (self));
      break;

    case PROP_DOMAIN:
      g_value_set_string (value, bjb_provider_get_domain (self));
      break;

    case PROP_USER_NAME:
      g_value_set_string (value, bjb_provider_get_user_name (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bjb_provider_finalize (GObject *object)
{
  BjbProvider *self = BJB_PROVIDER (object);
  BjbProviderPrivate *priv = bjb_provider_get_instance_private (self);

  g_clear_object (&priv->tag_store);

  G_OBJECT_CLASS (bjb_provider_parent_class)->finalize (object);
}

static void
bjb_provider_class_init (BjbProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bjb_provider_get_property;
  object_class->finalize = bjb_provider_finalize;

  klass->get_name = bjb_provider_real_get_name;
  klass->get_icon = bjb_provider_real_get_icon;
  klass->get_rgba = bjb_provider_real_get_rgba;
  klass->get_location_name = bjb_provider_real_get_location_name;
  klass->get_status = bjb_provider_real_get_status;

  klass->get_notes = bjb_provider_real_get_notes;
  klass->get_trash_notes = bjb_provider_real_get_trash_notes;

  klass->connect_async = bjb_provider_real_connect_async;
  klass->connect_finish = bjb_provider_real_connect_finish;
  klass->save_item_async = bjb_provider_real_save_item_async;
  klass->save_item_finish = bjb_provider_real_save_item_finish;
  klass->restore_item_async = bjb_provider_real_restore_item_async;
  klass->restore_item_finish = bjb_provider_real_restore_item_finish;
  klass->delete_item_async = bjb_provider_real_delete_item_async;
  klass->delete_item_finish = bjb_provider_real_delete_item_finish;

  properties[PROP_UID] =
    g_param_spec_string ("uid",
                         "Uid",
                         "A unique id for the provider",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_NAME] =
    g_param_spec_string ("name",
                         "Name",
                         "The name of the provider",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_ICON] =
    g_param_spec_object ("icon",
                         "Icon",
                         "The icon for the provider",
                         G_TYPE_ICON,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_STATUS] =
    g_param_spec_boolean ("status",
                          "status",
                          "The provider connection status",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_LOADED] =
    g_param_spec_boolean ("loaded",
                          "Loaded",
                          "If provider has loaded items",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  properties[PROP_DOMAIN] =
    g_param_spec_string ("domain",
                         "Domain",
                         "The domain name of the provider",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);
  properties[PROP_USER_NAME] =
    g_param_spec_string ("user-name",
                         "User Name",
                         "The user name used for the provider",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);

  /**
   * BjbProvider::item-removed:
   * @self: a #BjbProvider
   * @item: a #BjbItem
   *
   * item-removed signal is emitted when an item is removed
   * from the provider.
   */
  signals [ITEM_REMOVED] =
    g_signal_new ("item-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1,
                  BJB_TYPE_ITEM);
}

static void
bjb_provider_init (BjbProvider *self)
{
  BjbProviderPrivate *priv = bjb_provider_get_instance_private (self);

  priv->tag_store = bjb_tag_store_new ();
}

/**
 * bjb_provider_get_uid:
 * @self: a #BjbProvider
 *
 * Get the unique id of the provider
 *
 * Returns: (nullable): the uid
 * of the provider. Free with g_free().
 */
const char *
bjb_provider_get_uid (BjbProvider *self)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), NULL);

  return BJB_PROVIDER_GET_CLASS (self)->get_uid (self);
}

/**
 * bjb_provider_get_name:
 * @self: a #BjbProvider
 *
 * Get the name of the provider
 *
 * Returns: (transfer none): the name of the provider.
 */
const char *
bjb_provider_get_name (BjbProvider *self)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), NULL);

  return BJB_PROVIDER_GET_CLASS (self)->get_name (self);
}

/**
 * bjb_item_get_icon:
 * @self: a #BjbProvider
 *
 * Get the #GIcon of the provider.
 *
 * Returns: (transfer full) (nullable): the icon
 * of the provider. Free with g_object_unref().
 */
GIcon *
bjb_provider_get_icon (BjbProvider  *self,
                       GError      **error)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), NULL);

  return BJB_PROVIDER_GET_CLASS (self)->get_icon (self, error);
}

/**
 * bjb_item_get_rgba:
 * @self: a #BjbProvider
 * @rgba: (out): A #GdkRGBA
 *
 * Get the #GdkRGBA color for the @provider.  If @provider
 * has an icon set, the provider may not have any #GdkRGBA
 * set.  See bjb_provider_get_icon().
 *
 * Returns: %TRUE if @rgba is set.  %FALSE otherwise.
 */
gboolean
bjb_provider_get_rgba (BjbProvider *self,
                       GdkRGBA     *rgba)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), FALSE);
  g_return_val_if_fail (rgba != NULL, FALSE);

  return BJB_PROVIDER_GET_CLASS (self)->get_rgba (self, rgba);
}

/**
 * bjb_provider_get_domain:
 * @self: a #BjbProvider
 *
 * Get the domain name of the provider
 *
 * Returns: (nullable): the domain name of the provider.
 */
const char *
bjb_provider_get_domain (BjbProvider *self)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), NULL);

  return BJB_PROVIDER_GET_CLASS (self)->get_domain (self);
}

/**
 * bjb_provider_get_user_name:
 * @self: a #BjbProvider
 *
 * Get the user name used to connect to the provider.
 *
 * Returns: (nullable): the user name connected to the provider.
 */
const char *
bjb_provider_get_user_name (BjbProvider *self)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), NULL);

  return BJB_PROVIDER_GET_CLASS (self)->get_user_name (self);
}

/**
 * bjb_provider_get_location_name:
 * @self: a #BjbProvider
 *
 * Get a human readable location name that represents the provider
 * location.
 *
 * Say for example: A local provider shall return "On this computer"
 *
 * Returns: (transfer none) : A string reperesenting the location
 * of the provider.
 */
const char *
bjb_provider_get_location_name (BjbProvider *self)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), "");

  return BJB_PROVIDER_GET_CLASS (self)->get_location_name (self);
}

BjbStatus
bjb_provider_get_status (BjbProvider *self)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), BJB_STATUS_UNKNOWN);

  return BJB_PROVIDER_GET_CLASS (self)->get_status (self);
}

/**
 * bjb_provider_get_loaded:
 * @self: a #BjbProvider
 *
 * Get if the @self has loaded items.  The provider
 * is not required to be in connected state for the
 * provider to have items loaded.  Say for example,
 * this can happen if the provider got disconnected
 * after the items got loaded.
 *
 * Returns: %TRUE if @self has successfully loaded.
 * %False otherwise.
 */
gboolean
bjb_provider_get_loaded (BjbProvider *self)
{
  BjbProviderPrivate *priv = bjb_provider_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_PROVIDER (self), FALSE);

  return priv->loaded;
}

void
bjb_provider_set_loaded (BjbProvider *self,
                         gboolean     loaded)
{
  BjbProviderPrivate *priv = bjb_provider_get_instance_private (self);

  g_return_if_fail (BJB_IS_PROVIDER (self));

  if (priv->loaded == !!loaded)
    return;

  priv->loaded = !!loaded;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_LOADED]);
}

/**
 * bjb_provider_get_notes:
 * @self: a #BjbProvider
 *
 * Get the list of notes loaded by the provider.
 *
 * Returns: (transfer none): A #GListModel of #BjbItem.
 */
GListModel *
bjb_provider_get_notes (BjbProvider *self)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), NULL);

  return BJB_PROVIDER_GET_CLASS (self)->get_notes (self);
}

/**
 * bjb_provider_get_trash_notes:
 * @self: a #BjbProvider
 *
 * Get the list of trashed notes loaded by the provider.
 *
 * Returns: (transfer none) (nullable): A #GListModel
 * of #BjbItem or %NULL if not supported.
 */
GListModel *
bjb_provider_get_trash_notes (BjbProvider *self)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), NULL);

  return BJB_PROVIDER_GET_CLASS (self)->get_trash_notes (self);
}

BjbTagStore *
bjb_provider_get_tag_store (BjbProvider *self)
{
  BjbProviderPrivate *priv = bjb_provider_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_PROVIDER (self), NULL);

  return priv->tag_store;
}

/**
 * bjb_provider_connect_async:
 * @self: a #BjbProvider
 * @cancellable: (nullable): a #GCancellable or %NULL
 * @callback: a #GAsyncReadyCallback, or %NULL
 * @user_data: closure data for @callback
 *
 * Asynchronously connect and load all items (Notes
 * and Notebooks) from the provider.
 *
 * @callback should complete the operation by calling
 * bjb_provider_connect_items_finish().
 */
void
bjb_provider_connect_async (BjbProvider         *self,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
  BjbProviderPrivate *priv = bjb_provider_get_instance_private (self);

  g_return_if_fail (BJB_IS_PROVIDER (self));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));
  g_return_if_fail (priv->connected == FALSE);

  BJB_PROVIDER_GET_CLASS (self)->connect_async (self, cancellable,
                                                callback, user_data);
}

/**
 * bjb_provider_connect_finish:
 * @self: a #BjbProvider
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError or %NULL
 *
 * Completes an asynchronous loading of items initiated
 * with bjb_provider_connect_items_async().
 *
 * Returns: %TRUE if items loaded successfully. %FALSE otherwise.
 */
gboolean
bjb_provider_connect_finish (BjbProvider   *self,
                             GAsyncResult  *result,
                             GError       **error)
{
  BjbProviderPrivate *priv = bjb_provider_get_instance_private (self);
  gboolean ret;

  g_return_val_if_fail (BJB_IS_PROVIDER (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  ret = BJB_PROVIDER_GET_CLASS (self)->connect_finish (self, result, error);

  if (ret)
    priv->loaded = TRUE;

  return ret;
}

/**
 * bjb_provider_save_item_async:
 * @self: a #BjbProvider
 * @item: a #BjbItem
 * @cancellable: (nullable): a #GCancellable or %NULL
 * @callback: a #GAsyncReadyCallback, or %NULL
 * @user_data: closure data for @callback
 *
 * Asynchronously save the @item. If the item
 * isn't saved at all, a new item (ie, a file, or database
 * entry, or whatever) is created. Else, the old item is
 * updated with the new data.
 *
 * @callback should complete the operation by calling
 * bjb_provider_save_item_finish().
 */
void
bjb_provider_save_item_async (BjbProvider         *self,
                              BjbItem             *item,
                              GCancellable        *cancellable,
                              GAsyncReadyCallback  callback,
                              gpointer             user_data)
{
  g_return_if_fail (BJB_IS_PROVIDER (self));
  g_return_if_fail (BJB_IS_ITEM (item));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  BJB_PROVIDER_GET_CLASS (self)->save_item_async (self, item,
                                                 cancellable, callback,
                                                 user_data);
}

/**
 * bjb_provider_save_item_finish:
 * @self: a #BjbProvider
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError or %NULL
 *
 * Completes saving an item initiated with
 * bjb_provider_save_item_async().
 *
 * Returns: %TRUE if the item was saved successfully.
 * %FALSE otherwise.
 */
gboolean
bjb_provider_save_item_finish (BjbProvider   *self,
                               GAsyncResult  *result,
                               GError       **error)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  return BJB_PROVIDER_GET_CLASS (self)->save_item_finish (self, result, error);
}

/**
 * bjb_provider_restore_item_async:
 * @self: a #BjbProvider
 * @item: a #BjbItem
 * @cancellable: (nullable): a #GCancellable or %NULL
 * @callback: a #GAsyncReadyCallback, or %NULL
 * @user_data: closure data for @callback
 *
 * Asynchronously restore the @item.
 *
 * @callback should complete the operation by calling
 * bjb_provider_restore_item_finish().
 */
void
bjb_provider_restore_item_async (BjbProvider         *self,
                                 BjbItem             *item,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data)
{
  g_return_if_fail (BJB_IS_PROVIDER (self));
  g_return_if_fail (BJB_IS_ITEM (item));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  BJB_PROVIDER_GET_CLASS (self)->restore_item_async (self, item,
                                                    cancellable, callback,
                                                    user_data);
}

/**
 * bjb_provider_restore_item_finish:
 * @self: a #BjbProvider
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError or %NULL
 *
 * Completes restoring an item initiated with
 * bjb_provider_restore_item_async().
 *
 * Returns: %TRUE if the item was restored successfully.
 * %FALSE otherwise.
 */
gboolean
bjb_provider_restore_item_finish (BjbProvider   *self,
                                  GAsyncResult  *result,
                                  GError       **error)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  return BJB_PROVIDER_GET_CLASS (self)->restore_item_finish (self, result, error);
}

/**
 * bjb_provider_delete_item_async:
 * @self: a #BjbProvider
 * @item: a #BjbItem
 * @cancellable: (nullable): a #GCancellable or %NULL
 * @callback: a #GAsyncReadyCallback, or %NULL
 * @user_data: closure data for @callback
 *
 * Asynchronously delete the @item.
 *
 * @callback should complete the operation by calling
 * bjb_provider_delete_item_finish().
 */
void
bjb_provider_delete_item_async (BjbProvider         *self,
                                BjbItem             *item,
                                GCancellable        *cancellable,
                                GAsyncReadyCallback  callback,
                                gpointer             user_data)
{
  g_return_if_fail (BJB_IS_PROVIDER (self));
  g_return_if_fail (BJB_IS_ITEM (item));
  g_return_if_fail (!cancellable || G_IS_CANCELLABLE (cancellable));

  BJB_PROVIDER_GET_CLASS (self)->delete_item_async (self, item,
                                                   cancellable, callback,
                                                   user_data);
}

/**
 * bjb_provider_delete_item_finish:
 * @self: a #BjbProvider
 * @result: a #GAsyncResult provided to callback
 * @error: a location for a #GError or %NULL
 *
 * Completes deleting an item initiated with
 * bjb_provider_delete_item_async().
 *
 * Returns: %TRUE if the item was deleted successfully. %FALSE otherwise.
 */
gboolean
bjb_provider_delete_item_finish (BjbProvider   *self,
                                 GAsyncResult  *result,
                                 GError       **error)
{
  g_return_val_if_fail (BJB_IS_PROVIDER (self), FALSE);
  g_return_val_if_fail (G_IS_ASYNC_RESULT (result), FALSE);

  return BJB_PROVIDER_GET_CLASS (self)->delete_item_finish (self, result, error);
}
