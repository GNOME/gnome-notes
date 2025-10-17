/* bjb-local-provider.c
 *
 * Copyright (C) Pierre-Yves LUYTEN 2013 <py@luyten.fr>
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

#define G_LOG_DOMAIN "bjb-local-provider"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib/gi18n.h>

#include "serializer/biji-lazy-serializer.h"
#include "../items/bjb-xml-note.h"
#include "bjb-local-provider.h"

struct _BjbLocalProvider
{
  BjbProvider  parent_instance;

  char        *domain;
  char        *user_name;

  char        *location;
  char        *trash_location;

  GListStore  *notes;
  GListStore  *trash_notes;
};

G_DEFINE_TYPE (BjbLocalProvider, bjb_local_provider, BJB_TYPE_PROVIDER)

static void
bjb_local_provider_finalize (GObject *object)
{
  BjbLocalProvider *self = (BjbLocalProvider *)object;

  g_clear_pointer (&self->domain, g_free);
  g_clear_pointer (&self->user_name, g_free);
  g_clear_pointer (&self->location, g_free);
  g_clear_pointer (&self->trash_location, g_free);

  g_clear_object (&self->notes);
  g_clear_object (&self->trash_notes);

  G_OBJECT_CLASS (bjb_local_provider_parent_class)->finalize (object);
}

static const char *
bjb_local_provider_get_name (BjbProvider *provider)
{
  return _("Local");
}

static GIcon *
bjb_local_provider_get_icon (BjbProvider  *provider,
                             GError      **error)
{
  return g_icon_new_for_string ("user-home", error);
}

static const char *
bjb_local_provider_get_location_name (BjbProvider *provider)
{
  return _("On This Computer");
}

static gboolean
local_provider_add_item (gpointer user_data)
{
  GListStore *store;

  g_assert (BJB_IS_XML_NOTE (user_data));

  store = g_object_get_data (user_data, "store");
  g_assert (G_IS_LIST_STORE (store));

  g_list_store_append (store, user_data);
  g_object_unref (user_data);

  return G_SOURCE_REMOVE;
}

static void
bjb_local_provider_load_path (BjbLocalProvider  *self,
                              GTask             *task,
                              GCancellable      *cancellable,
                              GError            **error)
{
  g_autoptr(GFileEnumerator) enumerator = NULL;
  g_autoptr(GFile) location = NULL;
  gpointer file_info_ptr;
  GListStore *store;
  const char *path;

  g_assert (BJB_IS_LOCAL_PROVIDER (self));
  g_assert (G_IS_TASK (task));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  store = g_object_get_data (G_OBJECT (task), "store");
  path = g_object_get_data (G_OBJECT (task), "path");
  g_assert (G_IS_LIST_STORE (store));
  g_assert (path != NULL);

  location = g_file_new_for_path (path);
  enumerator = g_file_enumerate_children (location,
                                          G_FILE_ATTRIBUTE_STANDARD_NAME","
                                          G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
                                          G_FILE_QUERY_INFO_NONE,
                                          cancellable,
                                          error);
  if (*error != NULL)
    return;

  while ((file_info_ptr = g_file_enumerator_next_file (enumerator, cancellable, NULL)))
    {
      g_autoptr(GFileInfo) file_info = file_info_ptr;
      GFile *file;
      BjbItem *note;
      const char *name;

      name = g_file_info_get_name (file_info);

      if (!g_str_has_suffix (name, ".note"))
        continue;

      file = g_file_get_child (location, name);
      note = bjb_xml_note_new_from_data (g_file_peek_path (file),
                                         bjb_provider_get_tag_store (BJB_PROVIDER (self)));
      bjb_item_unset_modified (note);

      if (store == self->trash_notes)
        bjb_item_set_is_trashed (note, TRUE);

      g_object_set_data (G_OBJECT (note), "provider", self);
      g_object_set_data_full (G_OBJECT (note), "file", file, g_object_unref);
      g_object_set_data (G_OBJECT (note), "store", store);
      g_idle_add (local_provider_add_item, note);
    }
}

static void
bjb_local_provider_load_notes (GTask        *task,
                               gpointer      source_object,
                               gpointer      task_data,
                               GCancellable *cancellable)
{
  BjbLocalProvider *self = source_object;
  GError *error = NULL;

  g_assert (G_IS_TASK (task));
  g_assert (BJB_IS_LOCAL_PROVIDER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  /* bjb_local_provider_load_tags (self, cancellable); */
  g_object_set_data (G_OBJECT (task), "path", self->location);
  g_object_set_data (G_OBJECT (task), "store", self->notes);
  bjb_local_provider_load_path (self, task, cancellable, &error);

  if (error)
    {
      g_task_return_error (task, error);
      return;
    }

  /* todo */
  g_usleep (100000);

  g_object_set_data (G_OBJECT (task), "path", self->trash_location);
  g_object_set_data (G_OBJECT (task), "store", self->trash_notes);
  bjb_local_provider_load_path (self, task, cancellable, &error);
  if (error)
    g_task_return_error (task, error);
  else
    g_task_return_boolean (task, TRUE);
}

static void
bjb_local_provider_connect_async (BjbProvider         *provider,
                                  GCancellable        *cancellable,
                                  GAsyncReadyCallback  callback,
                                  gpointer             user_data)
{
  BjbLocalProvider *self = (BjbLocalProvider *)provider;
  g_autoptr(GTask) task = NULL;

  g_assert (BJB_IS_LOCAL_PROVIDER (self));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, bjb_local_provider_connect_async);
  g_task_run_in_thread (task, bjb_local_provider_load_notes);
}

static void
bjb_local_provider_save_item (GTask        *task,
                              gpointer      source_object,
                              gpointer      task_data,
                              GCancellable *cancellable)
{
  BjbLocalProvider *self = source_object;
  BjbItem *item = task_data;

  g_assert (G_IS_TASK (task));
  g_assert (BJB_IS_LOCAL_PROVIDER (self));
  g_assert (BJB_IS_ITEM (item));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  /* Make sure the note has a uid. */
  if (bjb_item_get_uid (item) == NULL)
    {
      g_autofree char *name = NULL;
      g_autofree char *path = NULL;
      g_autofree char *uuid = NULL;

      uuid = g_uuid_string_random ();
      name = g_strdup_printf ("%s.note", uuid);
      path = g_build_filename (g_get_user_data_dir (), "bijiben", name, NULL);

      bjb_item_set_uid (item, path);
    }

  if (BJB_IS_NOTE (item))
    biji_lazy_serialize (BJB_NOTE (item));

  g_task_return_boolean (task, TRUE);
}

static void
bjb_local_provider_save_item_async (BjbProvider          *provider,
                                   BjbItem              *item,
                                   GCancellable        *cancellable,
                                   GAsyncReadyCallback  callback,
                                   gpointer             user_data)
{
  BjbLocalProvider *self = (BjbLocalProvider *)provider;
  g_autoptr(GTask) task = NULL;

  g_assert (BJB_IS_LOCAL_PROVIDER (self));
  g_assert (BJB_IS_ITEM (item));
  g_assert (!cancellable || G_IS_CANCELLABLE (cancellable));

  task = g_task_new (self, cancellable, callback, user_data);
  g_task_set_source_tag (task, bjb_local_provider_save_item_async);
  g_task_set_task_data (task, g_object_ref (item), g_object_unref);
  g_task_run_in_thread (task, bjb_local_provider_save_item);
}

static GListModel *
bjb_local_provider_get_notes (BjbProvider *provider)
{
  BjbLocalProvider *self = (BjbLocalProvider *)provider;

  g_assert (BJB_IS_LOCAL_PROVIDER (self));

  return G_LIST_MODEL (self->notes);
}

static GListModel *
bjb_local_provider_get_trash_notes (BjbProvider *provider)
{
  BjbLocalProvider *self = (BjbLocalProvider *)provider;

  g_assert (BJB_IS_LOCAL_PROVIDER (self));

  return G_LIST_MODEL (self->trash_notes);
}

static void
bjb_local_provider_class_init (BjbLocalProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BjbProviderClass *provider_class = BJB_PROVIDER_CLASS (klass);

  object_class->finalize = bjb_local_provider_finalize;

  provider_class->get_name = bjb_local_provider_get_name;
  provider_class->get_icon = bjb_local_provider_get_icon;
  provider_class->get_location_name = bjb_local_provider_get_location_name;
  provider_class->get_notes = bjb_local_provider_get_notes;
  provider_class->get_trash_notes = bjb_local_provider_get_trash_notes;

  provider_class->connect_async = bjb_local_provider_connect_async;
  provider_class->save_item_async = bjb_local_provider_save_item_async;
}

static void
bjb_local_provider_init (BjbLocalProvider *self)
{
  self->notes = g_list_store_new (BJB_TYPE_ITEM);
  self->trash_notes = g_list_store_new (BJB_TYPE_ITEM);
}

BjbProvider *
bjb_local_provider_new (const char *path)
{
  BjbLocalProvider *self;

  self = g_object_new (BJB_TYPE_LOCAL_PROVIDER, NULL);

  if (path && *path)
    self->location = g_strdup (path);
  else
    self->location = g_build_filename (g_get_user_data_dir (),
                                       "bijiben", NULL);

  self->trash_location = g_build_filename (self->location,
                                           ".Trash", NULL);
  g_mkdir_with_parents (self->location, 0755);
  g_mkdir_with_parents (self->trash_location, 0755);

  return BJB_PROVIDER (self);
}
