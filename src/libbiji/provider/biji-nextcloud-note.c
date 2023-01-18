/* biji-nextcloud-note.c
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

#define G_LOG_DOMAIN "bjb-nextcloud-note"

#include <curl/curl.h>
#include <json-glib/json-glib.h>

#include "libbiji.h"
#include "biji-nextcloud-note.h"
#include "biji-nextcloud-provider.h"
#include "biji-tracker.h"

struct _BijiNextcloudNote
{
  BijiNoteObj            parent_instance;
  BijiNextcloudProvider *provider;
  gint64                 id;

  /* BijiNoteObj::path contains the URL for the note in the server, but when
   * creating a new note, we don't have that yet, so we fulfill it with a
   * temporary fake URL.
   * We do that because BijiNoteObj::path is used as a key for BijiManager and
   * BijiController notes lists.
   * Once we save the note for the first time, we receive the new URL from the
   * server. If we replace BijiNoteObj::path property with that new URL, we'll
   * have to deal with changing BijiManager and BijiController notes lists keys.
   * Instead of doing so, it's easier to just use a new property for further
   * server interactions.
   */
  char                  *new_url;
};

G_DEFINE_TYPE (BijiNextcloudNote, biji_nextcloud_note, BIJI_TYPE_NOTE_OBJ)

static gboolean
note_no (BijiNoteObj *note)
{
  return FALSE;
}

static char *
get_basename (BijiNoteObj *note)
{
  return (char *)bjb_item_get_uid (BJB_ITEM (note));
}

static char *
get_html (BijiNoteObj *note)
{
  return html_from_plain_text (biji_note_obj_get_raw_text (note));
}

static void
set_html (BijiNoteObj *note,
          const char  *html)
{
}

static void
save_note_finish (GObject      *source_object,
                  GAsyncResult *res,
                  gpointer      user_data)
{
  BijiNoteObj *note = BIJI_NOTE_OBJ (source_object);
  BijiNextcloudNote *self = BIJI_NEXTCLOUD_NOTE (source_object);
  const BijiProviderInfo *prov_info = biji_provider_get_info (BIJI_PROVIDER (self->provider));
  BijiInfoSet *info = biji_info_set_new ();
  BijiManager *manager;

  info->url = (char *) biji_note_obj_get_path (note);
  info->title = (char *) bjb_item_get_title (BJB_ITEM (note));
  info->content = (char *) biji_note_obj_get_raw_text (note);
  info->mtime = bjb_item_get_mtime (BJB_ITEM (note));
  info->created = bjb_item_get_create_time (BJB_ITEM (note));
  info->datasource_urn = g_strdup (prov_info->datasource);

  manager = biji_note_obj_get_manager (note);
  biji_tracker_save_note (biji_manager_get_tracker (manager), info);
}

static size_t
parse_save_results_cb (void   *contents,
                       size_t  length,
                       size_t  nmemb,
                       void   *user_data)
{
  BijiNoteObj *note = BIJI_NOTE_OBJ (user_data);
  BijiNextcloudNote *self = BIJI_NEXTCLOUD_NOTE (user_data);
  size_t real_size = length * nmemb;
  g_autoptr (JsonParser) parser = json_parser_new ();
  JsonNode *root = NULL;
  JsonObject *json = NULL;
  gint64 id = 0;
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
  if (JSON_NODE_TYPE (root) == JSON_NODE_OBJECT)
    {
      json = json_node_get_object (root);

      id = json_object_get_int_member (json, "id");
      if (id > 0 && self->id != id)
        {
          self->id = id;
          g_free (self->new_url);
          self->new_url = g_strdup_printf ("%s/%ld",
                                           biji_nextcloud_provider_get_baseurl (self->provider),
                                           self->id);
        }

      bjb_item_set_mtime (BJB_ITEM (note), json_object_get_int_member (json, "modified"));
    }
  else
    return 0;

  return real_size;
}

static void
save_note_thread (GTask        *task,
                  gpointer      source_object,
                  gpointer      task_data,
                  GCancellable *cancellable)
{
  BijiNoteObj *note = BIJI_NOTE_OBJ (source_object);
  BijiNextcloudNote *self = BIJI_NEXTCLOUD_NOTE (source_object);
  CURL *curl = curl_easy_init ();
  CURLcode res = 0;
  struct curl_slist *headers = NULL;
  g_autofree char *json_text = NULL;
  g_autofree char *content = NULL;

  if (curl)
    {
      headers = curl_slist_append (headers, "Content-Type: application/json");
      content = biji_str_mass_replace (biji_note_obj_get_raw_text (note), "\n", "\\n", NULL);
      json_text = g_strdup_printf ("{\"title\": \"%s\", \"content\": \"%s\", \"modified\": %ld}",
                                   bjb_item_get_title (BJB_ITEM (note)),
                                   content,
                                   bjb_item_get_mtime (BJB_ITEM (note)));

      curl_easy_setopt (curl, CURLOPT_HTTPHEADER, headers);
      curl_easy_setopt (curl, CURLOPT_POSTFIELDS, json_text);
      if (self->id > 0)
        {
          if (self->new_url)
            curl_easy_setopt (curl, CURLOPT_URL, self->new_url);
          else
            curl_easy_setopt (curl, CURLOPT_URL, biji_note_obj_get_path (note));
          curl_easy_setopt (curl, CURLOPT_CUSTOMREQUEST, "PUT");
        }
      else
        curl_easy_setopt (curl, CURLOPT_URL, biji_nextcloud_provider_get_baseurl (self->provider));
      curl_easy_setopt (curl, CURLOPT_USERNAME, biji_nextcloud_provider_get_username (self->provider));
      curl_easy_setopt (curl, CURLOPT_PASSWORD, biji_nextcloud_provider_get_password (self->provider));
      curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, parse_save_results_cb);
      curl_easy_setopt (curl, CURLOPT_WRITEDATA, (void *)self);

      res = curl_easy_perform (curl);
      if (res != CURLE_OK)
        g_debug ("curl_easy_perform() failed: %s", curl_easy_strerror (res));

      curl_slist_free_all (headers);
      curl_easy_cleanup (curl);
    }
}

static void
save_note_async (BijiNextcloudNote *self)
{
  g_autoptr (GTask) task = g_task_new (self, NULL, save_note_finish, NULL);
  g_task_run_in_thread (task, save_note_thread);
}

static void
save_note (BijiNoteObj *note)
{
  const char *title = bjb_item_get_title (BJB_ITEM (note));
  const char *content = biji_note_obj_get_raw_text (note);

  if (title && strlen (title) > 0 && content && strlen (content) > 0)
    save_note_async (BIJI_NEXTCLOUD_NOTE (note));
}

/* archive function will actually remove the note. This is what the previous
 * implementation (OwnCloud file notes) was using before but and it'll be
 * changed in the future.
 */
static gboolean
archive (BijiNoteObj *note)
{
  BijiNextcloudNote *self = BIJI_NEXTCLOUD_NOTE (note);
  BijiTracker *tracker;
  CURL *curl = curl_easy_init ();
  CURLcode res = 0;

  if (curl && self->id > 0)
    {
      if (self->new_url)
        curl_easy_setopt (curl, CURLOPT_URL, self->new_url);
      else
        curl_easy_setopt (curl, CURLOPT_URL, biji_note_obj_get_path (note));

      curl_easy_setopt (curl, CURLOPT_CUSTOMREQUEST, "DELETE");
      curl_easy_setopt (curl, CURLOPT_USERNAME, biji_nextcloud_provider_get_username (self->provider));
      curl_easy_setopt (curl, CURLOPT_PASSWORD, biji_nextcloud_provider_get_password (self->provider));

      res = curl_easy_perform (curl);
      if (res == CURLE_OK)
        {
          curl_easy_cleanup (curl);

          tracker = biji_manager_get_tracker (biji_note_obj_get_manager (note));
          biji_tracker_delete_note (tracker, note);

          return TRUE;
        }

      g_warning ("curl_easy_perform() failed: %s", curl_easy_strerror (res));
      curl_easy_cleanup (curl);
    }

  return FALSE;
}

BijiNoteObj *
biji_nextcloud_note_new (BijiNextcloudProvider *provider,
                         BijiManager           *manager,
                         gint64                 id,
                         const char            *title,
                         gint64                 mtime,
                         const char            *content)
{
  /* const char *baseurl = biji_nextcloud_provider_get_baseurl (provider); */
  g_autofree char *url = NULL;
  g_autofree char *uuid = NULL;
  BijiNextcloudNote *self = NULL;

  /* if (id > 0) */
  /*   url = g_strdup_printf ("%s/%ld", baseurl, id); */
  /* else */
  /*   { */
  /*     uuid = g_uuid_string_random (); */
  /*     url = g_strdup_printf ("%s/temp_%s", baseurl, uuid); */
  /*   } */

  self = g_object_new (BIJI_TYPE_NEXTCLOUD_NOTE,
                       "manager", manager,
                       "path",    url,
                       "title",   title,
                       "mtime",   mtime,
                       "content", content,
                       NULL);

  self->provider = provider;
  self->id = id;

  return BIJI_NOTE_OBJ (self);
}

static void
finalize (GObject *object)
{
  BijiNextcloudNote *self = (BijiNextcloudNote *)object;

  g_free (self->new_url);

  G_OBJECT_CLASS (biji_nextcloud_note_parent_class)->finalize (object);
}

static void
biji_nextcloud_note_class_init (BijiNextcloudNoteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BijiNoteObjClass *note_class = BIJI_NOTE_OBJ_CLASS (klass);

  object_class->finalize = finalize;

  note_class->get_basename = get_basename;
  note_class->get_html = get_html;
  note_class->set_html = set_html;
  note_class->save_note = save_note;
  note_class->can_format = note_no;
  note_class->archive = archive;
}

static void
biji_nextcloud_note_init (BijiNextcloudNote *self)
{
}

