/* bjb-own-cloud-note.c
 * Copyright (C) Pierre-Yves LUYTEN 2013 <py@luyten.fr>
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


/*
 * Trash bin is not implemented.
 * Markdown is not implemented.
 */

#include <uuid/uuid.h>

#include "biji-info-set.h"
#include "biji-item.h"
#include "biji-own-cloud-note.h"
#include "biji-own-cloud-provider.h"
#include "../serializer/biji-lazy-serializer.h"

struct _BijiOwnCloudNote
{
  BijiNoteObj           parent_instance;
  BijiOwnCloudProvider *prov;

  GFile *location;
  gchar *basename;
  GCancellable *cancellable; //TODO cancel write to file
};



G_DEFINE_TYPE (BijiOwnCloudNote, biji_own_cloud_note, BIJI_TYPE_NOTE_OBJ)



static const gchar *
ocloud_note_get_place (BijiItem *local)
{
  BijiOwnCloudNote *self;
  const BijiProviderInfo *info;

  g_return_val_if_fail (BIJI_IS_OWN_CLOUD_NOTE (local), NULL);

  self = BIJI_OWN_CLOUD_NOTE (local);
  info = biji_provider_get_info (BIJI_PROVIDER (self->prov));

  return info->name;
}


/* Better not to keep any cache here
 * We just want cache for overview,
 * not for actual content
 *
 * TODO: this means opening a note has to a be async
 * which should have been obvious from start */

static gchar *
ocloud_note_get_html (BijiNoteObj *note)
{
  BijiOwnCloudNote *self;
  gchar *content, *html;
  GError *err;

  g_return_val_if_fail (BIJI_IS_OWN_CLOUD_NOTE (note), NULL);

  err = NULL;
  self = BIJI_OWN_CLOUD_NOTE (note);

  if (g_file_load_contents (self->location, NULL, &content, 0, NULL, &err))
  {
    html = html_from_plain_text (content);
    g_free (content);
    return html;
  }


  if (err != NULL)
  {
    g_warning ("%s", err->message);
    g_error_free (err);
  }


  return NULL;
}


/* We don't put any html to note. We do not need this */
static void
ocloud_note_set_html (BijiNoteObj *note,
                      const gchar *html)
{

}


/* we definitely need to be sure we push to tracker
 * the exact same mtime as reported by GFile
 * the real fix is to set mtime when save happens.. */

static void
ocloud_note_ensure_resource (BijiNoteObj *note)
{
  BijiInfoSet *info;
  BijiItem *item;
  BijiOwnCloudNote *ocnote;
  g_autoptr(GFileInfo) file_info = NULL;
  const BijiProviderInfo *provider;

  g_return_if_fail (BIJI_IS_OWN_CLOUD_NOTE (note));
  g_return_if_fail (G_IS_FILE (BIJI_OWN_CLOUD_NOTE (note)->location));

  item = BIJI_ITEM (note);
  ocnote = BIJI_OWN_CLOUD_NOTE (note);
  file_info = g_file_query_info (ocnote->location, G_FILE_ATTRIBUTE_TIME_MODIFIED,
                                 G_FILE_QUERY_INFO_NONE, NULL, NULL);
  provider = biji_provider_get_info (BIJI_PROVIDER (ocnote->prov));

  info = biji_info_set_new ();
  info->url = (gchar*) biji_item_get_uuid (item);
  info->title = (gchar*) biji_item_get_title (item);
  info->content = (gchar*) biji_note_obj_get_raw_text (note);
  info->mtime = g_file_info_get_attribute_uint64 (file_info, G_FILE_ATTRIBUTE_TIME_MODIFIED);
  info->created = biji_note_obj_get_create_date (note);
  info->datasource_urn = g_strdup (provider->datasource);

  biji_tracker_ensure_resource_from_info  (biji_item_get_manager (item),
                                            info);
}

static void
ocloud_note_set_id (BijiOwnCloudNote *self)
{
  BijiItem         *item     = BIJI_ITEM (self);
  BijiNoteObj      *note_obj = BIJI_NOTE_OBJ (self);
  BijiManager      *manager  = NULL;
  const char       *old_key  = NULL;
  g_autofree gchar *key      = NULL;

  manager = biji_item_get_manager (item);
  old_key = biji_note_obj_get_path (note_obj);
  key = g_strdup_printf ("%s/%s",
                         biji_own_cloud_provider_get_readable_path (self->prov),
                         self->basename);

  biji_manager_remove_item_at_path (manager, old_key);
  biji_note_obj_set_path (note_obj, key);
  biji_manager_add_item (manager, item, BIJI_LIVING_ITEMS, TRUE);
}

static void
on_file_updated_cb (GObject *source_object,
                    GAsyncResult *res,
                    gpointer user_data)
{
  BijiOwnCloudNote *self = BIJI_OWN_CLOUD_NOTE (user_data);
  g_autoptr(GError) error = NULL;

  if (!g_file_replace_contents_finish (self->location, res, NULL, &error))
    {
      g_warning ("ownCloud note not updated. %s", error->message);
      return;
    }

  ocloud_note_ensure_resource (BIJI_NOTE_OBJ (self));
}

static void
biji_own_cloud_note_file_update (BijiOwnCloudNote *self)
{
  const gchar *str = biji_note_obj_get_raw_text (BIJI_NOTE_OBJ (self));

  g_file_replace_contents_async (self->location,
                                 str,
                                 strlen(str),
                                 NULL,
                                 FALSE,
                                 G_FILE_CREATE_REPLACE_DESTINATION,
                                 self->cancellable,
                                 on_file_updated_cb,
                                 self);
}

static void
on_file_renamed_cb (GObject *source_object,
                    GAsyncResult *res,
                    gpointer user_data)
{
  BijiOwnCloudNote *self = BIJI_OWN_CLOUD_NOTE (user_data);
  GFile *tmp_file = self->location;
  g_autoptr(GError) error = NULL;

  self->location = g_file_set_display_name_finish (self->location, res, &error);
  if (error)
    {
      g_warning ("ownCloud note not renamed. %s", error->message);
      self->location = tmp_file;
      return;
    }
  g_clear_object (&tmp_file);

  ocloud_note_set_id (self);
  biji_own_cloud_note_file_update (self);
}

static void
biji_own_cloud_note_file_rename (BijiOwnCloudNote *self)
{
  g_file_set_display_name_async (self->location,
                                 self->basename,
                                 G_PRIORITY_DEFAULT,
                                 NULL,
                                 on_file_renamed_cb,
                                 self);
}

static void
on_file_created_cb (GObject *source_object,
                    GAsyncResult *res,
                    gpointer user_data)
{
  BijiOwnCloudNote *self = BIJI_OWN_CLOUD_NOTE (user_data);
  GFileOutputStream *new_file;
  g_autoptr(GError) error = NULL;

  new_file = g_file_create_finish (self->location, res, &error);
  if (error)
    {
      g_warning ("ownCloud note not created. %s", error->message);
      return;
    }
  g_clear_object (&new_file);

  ocloud_note_set_id (self);
  biji_own_cloud_note_file_update (self);
}

static void
biji_own_cloud_note_file_create (BijiOwnCloudNote *self)
{
  if (self->location)
    g_object_unref (self->location);

  self->location = g_file_get_child (biji_own_cloud_provider_get_folder (self->prov),
                                     self->basename);

  g_file_create_async (self->location,
                       G_FILE_CREATE_NONE,
                       G_PRIORITY_DEFAULT,
                       NULL,
                       on_file_created_cb,
                       self);
}

static void
ocloud_note_save (BijiNoteObj *note)
{
  BijiOwnCloudNote *self         = BIJI_OWN_CLOUD_NOTE (note);
  g_autofree char  *new_basename = NULL;
  gboolean          needs_rename = FALSE;

  new_basename = g_strdup_printf ("%s.txt", biji_note_obj_get_title (BIJI_NOTE_OBJ (self)));
  if (g_strcmp0 (new_basename, self->basename) != 0)
    {
      g_free (self->basename);
      self->basename = g_strdup (new_basename);
      needs_rename = TRUE;
    }

  /* create note file if it doesn't exist */
  if (self->location == NULL || !g_file_query_exists (self->location, NULL))
    {
      biji_own_cloud_note_file_create (self);
    }
  /* if file exists and its title was changed */
  else if (needs_rename)
    {
      biji_own_cloud_note_file_rename (self);
    }
  /* or just update it */
  else
    {
      biji_own_cloud_note_file_update (self);
    }
}

static void
biji_own_cloud_note_finalize (GObject *object)
{
  BijiOwnCloudNote *self;


  g_return_if_fail (BIJI_IS_OWN_CLOUD_NOTE (object));

  self = BIJI_OWN_CLOUD_NOTE (object);

  g_object_unref (self->location);
  g_object_unref (self->cancellable);

  G_OBJECT_CLASS (biji_own_cloud_note_parent_class)->finalize (object);
}


static void
biji_own_cloud_note_constructed (GObject *object)
{
  g_return_if_fail (BIJI_IS_OWN_CLOUD_NOTE (object));

  G_OBJECT_CLASS (biji_own_cloud_note_parent_class)->constructed (object);
}


static void
biji_own_cloud_note_init (BijiOwnCloudNote *self)
{
  self->cancellable = g_cancellable_new ();
}


static gboolean
item_no         (BijiItem * item)
{
  return FALSE;
}


static gboolean
note_no         (BijiNoteObj *item)
{
  return FALSE;
}


static gboolean
ocloud_note_delete (BijiNoteObj *note)
{
  BijiOwnCloudNote *self;

  self = BIJI_OWN_CLOUD_NOTE (note);
  biji_note_delete_from_tracker (BIJI_NOTE_OBJ (self));
  return g_file_delete (self->location, NULL, NULL);
}


static gchar *
ocloud_note_get_basename (BijiNoteObj *note)
{
  return BIJI_OWN_CLOUD_NOTE (note)->basename;
}


static void
biji_own_cloud_note_class_init (BijiOwnCloudNoteClass *klass)
{
  GObjectClass *g_object_class;
  BijiItemClass *item_class;
  BijiNoteObjClass *note_class;

  g_object_class = G_OBJECT_CLASS (klass);
  item_class = BIJI_ITEM_CLASS (klass);
  note_class = BIJI_NOTE_OBJ_CLASS (klass);

  g_object_class->finalize = biji_own_cloud_note_finalize;
  g_object_class->constructed = biji_own_cloud_note_constructed;

  item_class->is_collectable = item_no;
  item_class->has_color = item_no;
  item_class->get_place = ocloud_note_get_place;

  note_class->get_basename = ocloud_note_get_basename;
  note_class->get_html = ocloud_note_get_html;
  note_class->set_html = ocloud_note_set_html;
  note_class->save_note = ocloud_note_save;
  note_class->can_format = note_no;
  note_class->archive = ocloud_note_delete;
  note_class->is_trashed = note_no;
}


BijiNoteObj        *biji_own_cloud_note_new_from_info           (BijiOwnCloudProvider *prov,
                                                                 BijiManager *manager,
                                                                 BijiInfoSet *info,
                                                                 gboolean online)
{
  BijiNoteID *id;
  gchar *sane_title;
  BijiNoteObj *retval;
  BijiOwnCloudNote *ocloud;

  /* First, sanitize the title, assuming no other thread
   * mess up with the InfoSet */
  if (info->title != NULL)
  {
    sane_title = biji_str_replace (info->title, ".txt", "");
    g_free (info->title);
    info->title = sane_title;
  }


  /* Hmm, even if the note starts blank we want some path...*/

  if (info->url == NULL)
  {
    uuid_t unique;
    char out[40];

    uuid_generate (unique);
    uuid_unparse_lower (unique, out);

    info->url = g_strdup_printf ("%s/%s.txt",
                  biji_own_cloud_provider_get_readable_path (prov),
                  out);
  }

  /* Now actually create the stuff */

  id = biji_note_id_new_from_info (info);

  retval = g_object_new (BIJI_TYPE_OWN_CLOUD_NOTE,
                         "manager", manager,
                         "id", id,
                         NULL);

  ocloud = BIJI_OWN_CLOUD_NOTE (retval);
  ocloud->prov = prov;
  biji_note_obj_set_create_date (retval, info->created);

  if (online)
    ocloud->location = g_file_new_for_commandline_arg (info->url);
  else
    ocloud->location = g_file_new_for_path (info->url);

  ocloud->basename = g_file_get_basename (ocloud->location);

  return retval;
}
