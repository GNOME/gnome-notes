/* bjb-memo-provider.c
 *
 * Copyright 2013 Pierre-Yves LUYTEN <py@luyten.fr>
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

#define G_LOG_DOMAIN "bjb-memo-provider"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib/gi18n.h>

#include "../items/bjb-plain-note.h"
#include "bjb-memo-provider.h"

/*
 * Evolution memos are usually saved as iCalendar VJOURNAL.
 * See https://tools.ietf.org/html/rfc5545
 *
 * The SUMMARY field of VJOURNAL is considered as note title
 * and the DESCRIPTION field as the note content.  Currently
 * we concatinate multiple DESCRIPTION fields into one (if
 * there is more than one) and show.
 */

struct _BjbMemoProvider
{
  BjbProvider     parent_instance;

  char           *uid;
  char           *name;
  char           *icon;
  GdkRGBA         rgba;

  ESource        *source;
  ECalClient     *client;
  ECalClientView *client_view;

  GListStore     *notes;
  BjbStatus       status;
};

G_DEFINE_TYPE (BjbMemoProvider, bjb_memo_provider, BJB_TYPE_PROVIDER)

static const char *
bjb_memo_provider_get_uid (BjbProvider *provider)
{
  return BJB_MEMO_PROVIDER (provider)->uid;
}

static const char *
bjb_memo_provider_get_name (BjbProvider *provider)
{
  BjbMemoProvider *self = BJB_MEMO_PROVIDER (provider);

  return self->name ? self->name : "";
}

static GIcon *
bjb_memo_provider_get_icon (BjbProvider  *provider,
                             GError      **error)
{
  return g_icon_new_for_string ("user-home", error);
}

static const char *
bjb_memo_provider_get_location_name (BjbProvider *provider)
{
  return _("Memo Note");
}

static gboolean
bjb_memo_provider_get_rgba (BjbProvider *provider,
                            GdkRGBA     *rgba)
{
  BjbMemoProvider *self = BJB_MEMO_PROVIDER (provider);

  *rgba = self->rgba;

  return TRUE;
}

static BjbStatus
bjb_memo_provider_get_status (BjbProvider *provider)
{
  BjbMemoProvider *self = BJB_MEMO_PROVIDER (provider);

  return self->status;
}

static GListModel *
bjb_memo_provider_get_notes (BjbProvider *provider)
{
  BjbMemoProvider *self = (BjbMemoProvider *)provider;

  g_assert (BJB_IS_MEMO_PROVIDER (self));

  return G_LIST_MODEL (self->notes);
}

static void
bjb_memo_provider_objects_added_cb (ECalClientView *client_view,
                                    const GSList   *objects,
                                    gpointer        user_data)
{
  BjbMemoProvider *self = user_data;

  g_assert (E_IS_CAL_CLIENT_VIEW (client_view));
  g_assert (BJB_IS_MEMO_PROVIDER (self));

  for (GSList *node = (GSList *)objects; node != NULL; node = node->next)
    {
      g_autoptr (ECalComponent) component = NULL;
      g_autoptr(ECalComponentText) summary = NULL;
      g_autoptr(BjbItem) note = NULL;
      g_autofree char *content = NULL;
      ICalComponent *icomponent;
      ICalTime *tt;
      GSList *descriptions;
      const char *uid, *title;
      time_t time;

      icomponent = i_cal_component_clone (node->data);
      component = e_cal_component_new_from_icalcomponent (icomponent);

      uid = e_cal_component_get_uid (component);
      summary = e_cal_component_get_summary (component);
      title = summary ? e_cal_component_text_get_value (summary) : "";
      descriptions = e_cal_component_get_descriptions (component);

      if (!summary && !descriptions)
        continue;

      for (GSList *item = descriptions; item != NULL; item = item->next)
        {
          ECalComponentText *text = item->data;
          g_autofree char *old_content = content;

          if (content != NULL)
            content = g_strconcat (old_content, "\n", e_cal_component_text_get_value (text), NULL);
          else
            content = g_strdup (e_cal_component_text_get_value (text));
        }

      g_slist_free_full (descriptions, e_cal_component_text_free);

      note = bjb_plain_note_new_from_data (uid, title, content);

      tt = e_cal_component_get_created (component);
      time = i_cal_time_as_timet (tt);
      bjb_item_set_create_time (BJB_ITEM (note), time);
      g_clear_object (&tt);

      tt = e_cal_component_get_last_modified (component);
      time = i_cal_time_as_timet (tt);
      bjb_item_set_mtime (BJB_ITEM (note), time);
      g_clear_object (&tt);

      bjb_item_unset_modified (note);

      g_object_set_data (G_OBJECT (note), "provider", BJB_PROVIDER (self));
      g_object_set_data_full (G_OBJECT (note), "component",
                              g_object_ref (component),
                              g_object_unref);

      g_list_store_append (self->notes, note);
    }
}

static void
bjb_memo_provider_objects_removed_cb (ECalClientView *client_view,
                                      const GSList   *objects,
                                      gpointer        user_data)
{
  BjbMemoProvider *self = user_data;

  g_assert (E_IS_CAL_CLIENT_VIEW (client_view));
  g_assert (BJB_IS_MEMO_PROVIDER (self));

  for (GSList *node = (GSList *)objects; node != NULL; node = node->next)
    {
      ECalComponentId *id = node->data;
      const char *uid;
      guint n_items;

      uid = e_cal_component_id_get_uid (id);
      n_items = g_list_model_get_n_items (G_LIST_MODEL (self->notes));

      for (guint i = 0; i < n_items; i++)
        {
          g_autoptr(BjbItem) item = NULL;
          const char *item_uid;

          item = g_list_model_get_item (G_LIST_MODEL (self->notes), i);
          item_uid = bjb_item_get_uid (item);

          if (g_strcmp0 (item_uid, uid) == 0)
            {
              guint position;

              if (g_list_store_find (self->notes, item, &position))
                {
                  g_signal_emit_by_name (self, "item-removed", item);
                  g_list_store_remove (self->notes, position);
                }

              continue;
            }
        }
    }
}

static void
bjb_memo_provider_loaded_cb (ECalClientView *client_view,
                             const GError   *error,
                             gpointer        user_data)
{
  BjbMemoProvider *self;
  g_autoptr(GTask) task = user_data;

  g_assert (E_IS_CAL_CLIENT_VIEW (client_view));
  g_assert (G_IS_TASK (task));

  self = g_task_get_source_object (task);
  g_assert (BJB_IS_MEMO_PROVIDER (self));

  bjb_provider_set_loaded (BJB_PROVIDER (self), !error);

  if (error)
    g_task_return_error (task, g_error_copy (error));
  else
    g_task_return_boolean (task, TRUE);
}

static void
bjb_memo_provider_view_ready_cb (GObject      *object,
                                 GAsyncResult *result,
                                 gpointer      user_data)
{
  BjbMemoProvider *self;
  ECalClient *client = (ECalClient *)object;
  g_autoptr(GTask) task = user_data;
  GError *error = NULL;

  g_assert (E_IS_CAL_CLIENT (client));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (G_IS_TASK (task));

  self = g_task_get_source_object (task);
  g_assert (BJB_IS_MEMO_PROVIDER (self));

  if (!e_cal_client_get_view_finish (client, result,
                                     &self->client_view,
                                     &error))
    {
      g_task_return_error (task, error);

      return;
    }

  g_signal_connect_object (self->client_view, "objects-added",
                           G_CALLBACK (bjb_memo_provider_objects_added_cb),
                           self, G_CONNECT_AFTER);
  g_signal_connect_object (self->client_view, "objects-removed",
                           G_CALLBACK (bjb_memo_provider_objects_removed_cb),
                           self, G_CONNECT_AFTER);
  g_signal_connect_object (self->client_view, "complete",
                           G_CALLBACK (bjb_memo_provider_loaded_cb),
                           g_object_ref (task), G_CONNECT_AFTER);

  e_cal_client_view_start (self->client_view, &error);

  if (error)
    g_task_return_error (task, error);
}

static void
bjb_memo_provider_connected_cb (GObject      *object,
                                GAsyncResult *result,
                                gpointer      user_data)
{
  BjbMemoProvider *self;
  g_autoptr(GTask) task = user_data;
  g_autoptr(GError) error = NULL;
  g_autoptr(ECalClient) client = NULL;
  GCancellable *cancellable;

  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (G_IS_TASK (task));

  self = g_task_get_source_object (task);
  g_assert (BJB_IS_MEMO_PROVIDER (self));

  client = E_CAL_CLIENT (e_cal_client_connect_finish (result, &error));

  if (!error)
    self->status = BJB_STATUS_CONNECTED;
  else
    self->status = BJB_STATUS_DISCONNECTED;

  g_object_notify (G_OBJECT (self), "status");

  if (error)
    {
      g_task_return_error (task, g_steal_pointer (&error));

      return;
    }

  cancellable = g_task_get_cancellable (task);
  self->client = g_steal_pointer (&client);
  e_cal_client_get_view (self->client,
                         "#t",
                         cancellable,
                         bjb_memo_provider_view_ready_cb,
                         g_steal_pointer (&task));
}

static void
bjb_memo_provider_connect_async (BjbProvider         *provider,
                                 GCancellable        *cancellable,
                                 GAsyncReadyCallback  callback,
                                 gpointer             user_data)
{
  BjbMemoProvider *self = (BjbMemoProvider *)provider;
  g_autoptr(GTask) task = NULL;

  g_assert (BJB_IS_MEMO_PROVIDER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, bjb_memo_provider_connect_async);

  self->status = BJB_STATUS_CONNECTING;
  g_object_notify (G_OBJECT (self), "status");

  e_cal_client_connect (self->source, E_CAL_CLIENT_SOURCE_TYPE_MEMOS,
                        10, /* seconds to wait */
                        NULL,
                        bjb_memo_provider_connected_cb,
                        g_steal_pointer (&task));
}

static void
bjb_memo_provider_create_cb (GObject      *object,
                             GAsyncResult *result,
                             gpointer      user_data)
{
  ECalClient *client = (ECalClient *)object;
  g_autoptr(GError) error = NULL;
  g_autoptr(GTask) task = user_data;
  g_autofree char *uid = NULL;
  BjbItem *item;

  g_assert (E_IS_CAL_CLIENT (client));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (G_IS_TASK (task));

  item = g_task_get_task_data (task);

  if (e_cal_client_create_object_finish (client, result, &uid, &error))
    {
      bjb_item_set_uid (item, uid);
      g_task_return_boolean (task, TRUE);
    }
  else
    g_task_return_error (task, g_steal_pointer (&error));

  g_application_release (g_application_get_default ());
}

static void
bjb_memo_provider_save_cb (GObject      *object,
                           GAsyncResult *result,
                           gpointer      user_data)
{
  ECalClient *client = (ECalClient *)object;
  g_autoptr(GError) error = NULL;
  g_autoptr(GTask) task = user_data;

  g_assert (E_IS_CAL_CLIENT (client));
  g_assert (G_IS_ASYNC_RESULT (result));
  g_assert (G_IS_TASK (task));

  if (e_cal_client_modify_object_finish (client, result, &error))
    {
      g_task_return_boolean (task, TRUE);
    }
  else
    g_task_return_error (task, g_steal_pointer (&error));

  g_application_release (g_application_get_default ());
}

static void
bjb_memo_provider_save_item_async (BjbProvider         *provider,
                                   BjbItem             *item,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  BjbMemoProvider *self = (BjbMemoProvider *)provider;
  g_autoptr(GTask) task = NULL;
  g_autofree char *content = NULL;
  g_autoptr(ECalComponentText) description = NULL;
  g_autoptr(ECalComponentText) summary = NULL;
  g_autoptr(ECalComponentDateTime) date_time = NULL;
  ECalComponent *component;
  ICalTimezone *zone;
  ICalTime *time;
  GSList descriptions;

  g_assert (BJB_IS_MEMO_PROVIDER (self));
  g_assert (BJB_IS_ITEM (item));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, bjb_memo_provider_save_item_async);
  g_task_set_task_data (task, g_object_ref (item), g_object_unref);
  g_application_hold (g_application_get_default ());

  zone = e_cal_client_get_default_timezone (self->client);
  time = i_cal_time_new_current_with_zone (zone);
  date_time = e_cal_component_datetime_new_take (time, NULL);

  component = g_object_get_data (G_OBJECT (item), "component");

  if (component == NULL)
    {
      component = e_cal_component_new ();
      e_cal_component_set_new_vtype (component, E_CAL_COMPONENT_JOURNAL);

      g_object_set_data_full (G_OBJECT (item), "component",
                              component, g_object_unref);

      e_cal_component_set_dtstart (component, date_time);
      e_cal_component_set_dtend (component, date_time);
      e_cal_component_set_created (component, time);
    }

  e_cal_component_set_last_modified (component, time);

  summary = e_cal_component_text_new (bjb_item_get_title (item), NULL);
  e_cal_component_set_summary (component, summary);

  content = bjb_note_get_raw_content (BJB_NOTE (item));
  description = e_cal_component_text_new (content, NULL);
  descriptions.data = description;
  descriptions.next = NULL;
  e_cal_component_set_descriptions (component, &descriptions);

  e_cal_component_commit_sequence (component);

  if (bjb_item_is_new (item))
    {
      e_cal_client_create_object (self->client,
                                  e_cal_component_get_icalcomponent (component),
                                  E_CAL_OPERATION_FLAG_CONFLICT_FAIL,
                                  NULL,
                                  bjb_memo_provider_create_cb,
                                  g_steal_pointer (&task));
    }
  else
    {
      e_cal_client_modify_object (self->client,
                                  e_cal_component_get_icalcomponent (component),
                                  E_CAL_OBJ_MOD_THIS,
                                  E_CAL_OPERATION_FLAG_CONFLICT_FAIL,
                                  cancellable,
                                  bjb_memo_provider_save_cb,
                                  g_steal_pointer (&task));
    }
}

static void
memo_provider_delete_item_cb (GObject      *object,
                              GAsyncResult *result,
                              gpointer      user_data)
{
  g_autoptr(GTask) task = user_data;
  GError *error = NULL;
  gboolean success;

  g_assert (G_IS_TASK (task));

  success = e_cal_client_remove_object_finish (E_CAL_CLIENT (object), result, &error);

  if (error)
    g_warning ("Could not delete memo: %s", error->message);

  if (success)
    g_task_return_boolean (task, success);
  else
    g_task_return_error (task, error);
}


static void
bjb_memo_provider_delete_item_async (BjbProvider         *provider,
                                     BjbItem             *item,
                                     GCancellable        *cancellable,
                                     GAsyncReadyCallback  callback,
                                     gpointer             user_data)
{
  BjbMemoProvider *self = BJB_MEMO_PROVIDER (provider);
  GTask *task;

  g_assert (BJB_IS_ITEM (item));

  task = g_task_new (self, cancellable, callback, user_data);
  e_cal_client_remove_object (self->client,
                              bjb_item_get_uid (item),
                              NULL,
                              E_CAL_OBJ_MOD_ALL,
                              E_CAL_OPERATION_FLAG_NONE,
                              cancellable,
                              memo_provider_delete_item_cb,
                              task);
}


static void
bjb_memo_provider_finalize (GObject *object)
{
  BjbMemoProvider *self = (BjbMemoProvider *)object;

  g_clear_object (&self->client_view);
  g_clear_object (&self->client);
  g_clear_object (&self->source);
  g_clear_object (&self->notes);

  g_clear_pointer (&self->uid, g_free);
  g_clear_pointer (&self->name, g_free);

  G_OBJECT_CLASS (bjb_memo_provider_parent_class)->finalize (object);
}

static void
bjb_memo_provider_class_init (BjbMemoProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BjbProviderClass *provider_class = BJB_PROVIDER_CLASS (klass);

  object_class->finalize = bjb_memo_provider_finalize;

  provider_class->get_uid = bjb_memo_provider_get_uid;
  provider_class->get_name = bjb_memo_provider_get_name;
  provider_class->get_icon = bjb_memo_provider_get_icon;
  provider_class->get_location_name = bjb_memo_provider_get_location_name;
  provider_class->get_rgba = bjb_memo_provider_get_rgba;
  provider_class->get_status = bjb_memo_provider_get_status;

  provider_class->get_notes = bjb_memo_provider_get_notes;

  provider_class->connect_async = bjb_memo_provider_connect_async;
  provider_class->save_item_async = bjb_memo_provider_save_item_async;
  provider_class->delete_item_async = bjb_memo_provider_delete_item_async;
}

static void
bjb_memo_provider_init (BjbMemoProvider *self)
{
  self->notes = g_list_store_new (BJB_TYPE_ITEM);
}

BjbProvider *
bjb_memo_provider_new (ESource *source)
{
  BjbMemoProvider *self;
  ESourceExtension *extension;
  const char *color;

  g_return_val_if_fail (E_IS_SOURCE (source), NULL);

  self = g_object_new (BJB_TYPE_MEMO_PROVIDER, NULL);
  self->source = g_object_ref (source);
  self->uid = e_source_dup_uid (source);
  self->name = e_source_dup_display_name (source);

  extension = e_source_get_extension (source, E_SOURCE_EXTENSION_MEMO_LIST);
  color = e_source_selectable_get_color (E_SOURCE_SELECTABLE (extension));
  gdk_rgba_parse (&self->rgba, color);

  return BJB_PROVIDER (self);
}
