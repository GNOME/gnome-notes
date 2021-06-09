/* biji-nextcloud-provider.c
 *
 * Copyright 2020 Isaque Galdino <igaldino@gmail.com>
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
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <curl/curl.h>
#include <json-glib/json-glib.h>

#include "biji-nextcloud-note.h"
#include "biji-nextcloud-provider.h"

struct _BijiNextcloudProvider
{
  BijiProvider      parent_instance;
  BijiProviderInfo  info;
  GoaObject        *goa;
  char             *password;
  char             *baseurl;
  GHashTable       *items;
};

G_DEFINE_TYPE (BijiNextcloudProvider, biji_nextcloud_provider, BIJI_TYPE_PROVIDER)

enum {
  PROP_0,
  PROP_GOA_OBJECT,
  NEXTCLOUD_PROV_PROP
};

static GParamSpec *properties [NEXTCLOUD_PROV_PROP] = { NULL, };

static void
load_all_notes_finish (GObject      *source_object,
                       GAsyncResult *res,
                       gpointer      user_data)
{
  BijiNextcloudProvider *self = BIJI_NEXTCLOUD_PROVIDER (source_object);
  g_autoptr (GList) list = g_hash_table_get_values (self->items);

  BIJI_PROVIDER_GET_CLASS (self)->notify_loaded (BIJI_PROVIDER (self), list, BIJI_LIVING_ITEMS);
}

static void
parse_json_array_cb (JsonArray *array,
                     guint      index_,
                     JsonNode  *node,
                     gpointer   user_data)
{
  BijiNextcloudProvider *self = BIJI_NEXTCLOUD_PROVIDER (user_data);
  JsonObject *json = json_node_get_object (node);

  gint64 id = json_object_get_int_member (json, "id");
  const char *title = json_object_get_string_member (json, "title");
  gint64 modified = json_object_get_int_member (json, "modified");
  /*const char *category = json_object_get_string_member (json, "category");*/
  const char *content  = json_object_get_string_member (json, "content");

  BijiNoteObj *note = biji_nextcloud_note_new (self,
                                               biji_provider_get_manager (BIJI_PROVIDER (self)),
                                               id,
                                               title,
                                               modified,
                                               content);
  g_hash_table_replace (self->items, g_strdup_printf ("%ld", id), note);
}

static size_t
parse_all_notes_cb (void   *contents,
                    size_t  length,
                    size_t  nmemb,
                    void   *user_data)
{
  BijiNextcloudProvider *self = BIJI_NEXTCLOUD_PROVIDER (user_data);
  size_t real_size = length * nmemb;
  g_autoptr (JsonParser) parser = json_parser_new ();
  JsonNode *root = NULL;
  const char *data = (const char *) contents;
  g_autoptr (GError) error = NULL;

  if (!data)
    return 0;

  if (!json_parser_load_from_data (parser, data, real_size, &error))
    {
      if (error)
        g_debug ("error:%s", error->message);
      return 0;
    }

  root = json_parser_get_root (parser);
  if (JSON_NODE_TYPE (root) == JSON_NODE_ARRAY)
    json_array_foreach_element (json_node_get_array (root), parse_json_array_cb, self);
  else
    return 0;

  return real_size;
}

static void
load_all_notes_thread(GTask        *task,
                      gpointer      source_object,
                      gpointer      task_data,
                      GCancellable *cancellable)
{
  BijiNextcloudProvider *self = BIJI_NEXTCLOUD_PROVIDER (source_object);
  CURL *curl = curl_easy_init ();
  CURLcode res = 0;

  if (curl)
    {
      curl_easy_setopt (curl, CURLOPT_URL, self->baseurl);
      curl_easy_setopt (curl, CURLOPT_USERNAME, self->info.user);
      curl_easy_setopt (curl, CURLOPT_PASSWORD, self->password);
      curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, parse_all_notes_cb);
      curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *)self);

      res = curl_easy_perform (curl);
      if (res != CURLE_OK)
        g_debug ("curl_easy_perform() failed: %s", curl_easy_strerror (res));

      curl_easy_cleanup (curl);
    }
}

static void
load_all_notes_async (BijiNextcloudProvider *self)
{
  g_autoptr (GTask) task = g_task_new (self, NULL, load_all_notes_finish, NULL);
  g_task_run_in_thread (task, load_all_notes_thread);
}

static const BijiProviderInfo *
get_info (BijiProvider *provider)
{
  BijiNextcloudProvider *self = BIJI_NEXTCLOUD_PROVIDER (provider);
  return &(self->info);
}

static BijiNoteObj *
create_new_note (BijiProvider *provider,
                 const char   *str)
{
  BijiManager *manager = biji_provider_get_manager (provider);
  BijiNextcloudProvider *self = BIJI_NEXTCLOUD_PROVIDER (provider);

  return biji_nextcloud_note_new (self, manager, 0, NULL, 0, "");
}

static BijiNoteObj *
create_note_full (BijiProvider  *provider,
                  const char    *suggested_path,
                  BijiInfoSet   *info,
                  const char    *html,
                  const GdkRGBA *color)
{
  BijiManager *manager = biji_provider_get_manager (provider);
  BijiNextcloudProvider *self = BIJI_NEXTCLOUD_PROVIDER (provider);
  BijiNoteObj *note = biji_nextcloud_note_new (self, manager, 0, info->title, info->mtime, info->content);
  GdkRGBA override_color;

  biji_manager_get_default_color (biji_provider_get_manager (provider), &override_color);
  biji_note_obj_set_html (note, html);
  biji_note_obj_set_rgba (note, &override_color);

  return note;
}

static void
load_archives (BijiProvider *provider)
{
  return;
}

BijiProvider *
biji_nextcloud_provider_new (BijiManager *manager,
                             GoaObject   *goa)
{
  return g_object_new (BIJI_TYPE_NEXTCLOUD_PROVIDER,
                       "manager", manager,
                       "goa",     goa,
                       NULL);
}

const char *
biji_nextcloud_provider_get_baseurl (BijiNextcloudProvider *self)
{
  return self->baseurl;
}

const char *
biji_nextcloud_provider_get_username (BijiNextcloudProvider *self)
{
  return self->info.user;
}

const char *
biji_nextcloud_provider_get_password (BijiNextcloudProvider *self)
{
  return self->password;
}

static void
constructed (GObject *object)
{
  BijiNextcloudProvider *self = (BijiNextcloudProvider *)object;
  GoaAccount *account = NULL;
  const char *goa_id;
  GoaPasswordBased *goa_pass = NULL;
  g_autoptr (GIcon) icon = NULL;
  g_autoptr (GError) error = NULL;

  G_OBJECT_CLASS (biji_nextcloud_provider_parent_class)->constructed (object);

  if (!GOA_IS_OBJECT (self->goa))
    {
      biji_provider_abort (BIJI_PROVIDER (self));
      return;
    }

  account = goa_object_peek_account (self->goa);
  if (!account)
    {
      biji_provider_abort (BIJI_PROVIDER (self));
      return;
    }

  self->items = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);

  self->info.unique_id = goa_account_get_id (account);
  self->info.datasource = g_strdup_printf ("gn:goa-account:%s", self->info.unique_id);
  self->info.name = g_strdup (goa_account_get_provider_name (account));
  self->info.user = goa_account_dup_identity (account);

  goa_id = goa_account_get_presentation_identity (account);
  if (goa_id)
    self->info.domain = g_strdup (g_strrstr (goa_id, "@") + 1);

  if (!self->info.user || !self->info.domain)
    {
      biji_provider_abort (BIJI_PROVIDER (self));
      return;
    }

  goa_pass = goa_object_peek_password_based (self->goa);
  if (!goa_pass ||
      !goa_password_based_call_get_password_sync (goa_pass,
                                                  self->info.user,
                                                  &self->password,
                                                  NULL, NULL))
    {
      g_debug ("FAILED: goa_password_based_call_get_password_sync");
      biji_provider_abort (BIJI_PROVIDER (self));
      return;
    }

  icon = g_icon_new_for_string (goa_account_get_provider_icon (account), &error);
  if (error)
    {
      g_warning ("%s", error->message);
      self->info.icon = NULL;
    }
  else
    {
      self->info.icon = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_INVALID);
      gtk_image_set_pixel_size (GTK_IMAGE (self->info.icon), 48);
      g_object_ref_sink (self->info.icon);
    }

  self->baseurl = g_strdup_printf ("https://%s/index.php/apps/notes/api/v1/notes",
                                   self->info.domain);

  curl_global_init (CURL_GLOBAL_DEFAULT);

  load_all_notes_async (self);
}

static void
finalize (GObject *object)
{
  BijiNextcloudProvider *self = (BijiNextcloudProvider *)object;

  curl_global_cleanup ();

  g_clear_object (&self->goa);
  g_clear_object (&self->info.icon);

  g_clear_pointer (&self->info.name, g_free);
  g_clear_pointer (&self->info.user, g_free);
  g_clear_pointer (&self->info.domain, g_free);
  g_clear_pointer (&self->password, g_free);
  g_clear_pointer (&self->baseurl, g_free);

  g_hash_table_unref (self->items);

  G_OBJECT_CLASS (biji_nextcloud_provider_parent_class)->finalize (object);
}

static void
get_property (GObject    *object,
              guint       prop_id,
              GValue     *value,
              GParamSpec *pspec)
{
  BijiNextcloudProvider *self = BIJI_NEXTCLOUD_PROVIDER (object);

  switch (prop_id)
    {
    case PROP_GOA_OBJECT:
      g_value_set_object (value, self->goa);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
set_property (GObject      *object,
              guint         prop_id,
              const GValue *value,
              GParamSpec   *pspec)
{
  BijiNextcloudProvider *self = BIJI_NEXTCLOUD_PROVIDER (object);

  switch (prop_id)
    {
    case PROP_GOA_OBJECT:
      self->goa = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
biji_nextcloud_provider_class_init (BijiNextcloudProviderClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BijiProviderClass *provider_class = BIJI_PROVIDER_CLASS (klass);

  object_class->constructed = constructed;
  object_class->finalize = finalize;
  object_class->get_property = get_property;
  object_class->set_property = set_property;

  provider_class->get_info = get_info;
  provider_class->create_new_note = create_new_note;
  provider_class->create_note_full = create_note_full;
  provider_class->load_archives = load_archives;

  properties[PROP_GOA_OBJECT] =
    g_param_spec_object("goa",
                        "The Goa Object",
                        "The Goa Object providing notes",
                        GOA_TYPE_OBJECT,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, NEXTCLOUD_PROV_PROP, properties);
}

static void
biji_nextcloud_provider_init (BijiNextcloudProvider *self)
{
}

