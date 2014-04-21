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
#include "../biji-timeout.h"
#include "../serializer/biji-lazy-serializer.h"



struct BijiOwnCloudNotePrivate_
{
  BijiOwnCloudProvider *prov;
  BijiNoteID *id;

  BijiTimeout           *timeout;


  GFile *location;
  gchar *basename;
  GCancellable *cancellable; //TODO cancel write to file
};



G_DEFINE_TYPE (BijiOwnCloudNote, biji_own_cloud_note, BIJI_TYPE_NOTE_OBJ)



const gchar *
ocloud_note_get_place (BijiItem *local)
{
  BijiOwnCloudNote *self;
  const BijiProviderInfo *info;

  g_return_val_if_fail (BIJI_IS_OWN_CLOUD_NOTE (local), NULL);

  self = BIJI_OWN_CLOUD_NOTE (local);
  info = biji_provider_get_info (BIJI_PROVIDER (self->priv->prov));

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

  if (g_file_load_contents (self->priv->location, NULL, &content, 0, NULL, &err))
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
                      gchar *html)
{

}


/* we definitely need to be sure we push to tracker
 * the exact same mtime as reported by GFile
 * the real fix is to set mtime when save happens.. */

static void
ocloud_note_ensure_ressource (BijiNoteObj *note)
{
  BijiInfoSet *info;
  BijiItem *item;
  BijiOwnCloudNote *ocnote;
  GFile *file;
  GFileInfo *file_info;
  const BijiProviderInfo *provider;

  g_return_if_fail (BIJI_IS_OWN_CLOUD_NOTE (note));
  g_return_if_fail (G_IS_FILE (BIJI_OWN_CLOUD_NOTE (note)->priv->location));

  item = BIJI_ITEM (note);
  ocnote = BIJI_OWN_CLOUD_NOTE (note);
  file = ocnote->priv->location;
  file_info = g_file_query_info (file, "time::modified", G_FILE_QUERY_INFO_NONE, NULL, NULL);
  provider = biji_provider_get_info (BIJI_PROVIDER (ocnote->priv->prov));

  info = biji_info_set_new ();
  info->url = (gchar*) biji_item_get_uuid (item);
  info->title = (gchar*) biji_item_get_title (item);
  info->content = (gchar*) biji_note_obj_get_raw_text (note);
  info->mtime = g_file_info_get_attribute_uint64 (file_info, "time::modified");
  info->created = biji_note_obj_get_create_date (note);
  info->datasource_urn = g_strdup (provider->datasource);

  biji_tracker_ensure_ressource_from_info  (biji_item_get_manager (item),
                                            info);
}


/* TODO: propagate error if any
 * through generic provider -> manager */
void
on_content_replaced  (GObject *source_object,
                      GAsyncResult *res,
                      gpointer user_data)
{
  GError *error = NULL;

  if (!g_file_replace_contents_finish (G_FILE (source_object),
                                       res,
                                       NULL,  //etag
                                       &error))
  {
    if (error)
    {
      g_warning ("ownCloud note not saved. %s", error->message);
      g_error_free (error);
    }
  }


  else
  {
    biji_note_obj_save_icon (user_data);
    ocloud_note_ensure_ressource (user_data);
  }
}





static void
ocloud_note_save (BijiNoteObj *note)
{
  BijiOwnCloudNote *self;
  GString *str;

  g_return_if_fail (BIJI_IS_OWN_CLOUD_NOTE (note));
  self = BIJI_OWN_CLOUD_NOTE (note);
  str = g_string_new (biji_note_obj_get_raw_text (note));


  /* backup would fail for some reason.
   * gfilemove for workaround? */
  g_file_replace_contents_async  (
      self->priv->location,
      str->str,
      str->len,
      NULL,   // etag
      FALSE,  //backup
      G_FILE_CREATE_REPLACE_DESTINATION,
      self->priv->cancellable,
      on_content_replaced,
      self);


  g_string_free (str, FALSE);
}








/* Rename the file
 * when note title change
 * Also handle new notes being populated
 * the noteID still keep some path, which we will remove */

static void
create_new_file (BijiOwnCloudNote *self, const gchar *basename)
{
  GFile *folder;
  BijiNoteObj *note;
  gchar *key;

  note = BIJI_NOTE_OBJ (self);
  folder = biji_own_cloud_provider_get_folder (self->priv->prov);

  /* TODO just free old location before, but check for no mistake */

  self->priv->location = g_file_get_child (folder, basename);
  key = g_strdup_printf (
               "%s/%s",
               biji_own_cloud_provider_get_readable_path (self->priv->prov),
               basename);
  g_object_set (self->priv->id, "path", key, NULL);

  ocloud_note_save (note);
  ocloud_note_ensure_ressource (note);

  g_free (key);
}


/* When the title is stable, handle io */
static void
on_timeout (BijiOwnCloudNote *self)
{
  create_new_file (self, self->priv->basename);
}


static void
on_title_change                     (BijiOwnCloudNote *self)
{
  const gchar *new_title;

  g_return_if_fail (BIJI_IS_OWN_CLOUD_NOTE (self));

  g_free (self->priv->basename);
  new_title = biji_note_id_get_title (self->priv->id);
  self->priv->basename = g_strdup_printf ("%s.txt", new_title);


  g_file_delete_async (self->priv->location,
                       G_PRIORITY_LOW,
                       NULL,
                       NULL,
                       NULL);

  biji_timeout_reset (self->priv->timeout, 3000);
}




static void
biji_own_cloud_note_finalize (GObject *object)
{
  BijiOwnCloudNote *self;

  g_return_if_fail (BIJI_IS_OWN_CLOUD_NOTE (object));

  self = BIJI_OWN_CLOUD_NOTE (object);

  g_object_unref (self->priv->cancellable);
  g_object_unref (self->priv->timeout);
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
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_OWN_CLOUD_NOTE, BijiOwnCloudNotePrivate);
  self->priv->cancellable = g_cancellable_new ();
  self->priv->id = NULL;

  self->priv->timeout = biji_timeout_new ();
  g_signal_connect_swapped (self->priv->timeout, "timeout",
                            G_CALLBACK (on_timeout), self);
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
  return g_file_delete (self->priv->location, NULL, NULL);
}

static gchar *
ocloud_note_get_basename (BijiNoteObj *note)
{
  return BIJI_OWN_CLOUD_NOTE (note)->priv->basename;
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

  g_type_class_add_private ((gpointer)klass, sizeof (BijiOwnCloudNotePrivate));
}


BijiNoteObj        *biji_own_cloud_note_new_from_info           (BijiOwnCloudProvider *prov,
                                                                 BijiManager *manager,
                                                                 BijiInfoSet *info)
{
  BijiNoteID *id;
  gchar *sane_title;
  BijiNoteObj *retval;
  BijiOwnCloudNote *ocloud;

  /* First, sanitize the title, assuming no other thread
   * mess up with the InfoSet */
  sane_title = biji_str_replace (info->title, ".txt", "");
  g_free (info->title);
  info->title = sane_title;


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
  ocloud->priv->id = id;
  ocloud->priv->prov = prov;
  biji_note_obj_set_create_date (retval, info->created);
  g_signal_connect_swapped (id, "notify::title",
                            G_CALLBACK (on_title_change), retval);


  ocloud->priv->location = g_file_new_for_commandline_arg (info->url);
  ocloud->priv->basename = g_file_get_basename (ocloud->priv->location);

  return retval;
}
