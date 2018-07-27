/* biji-own-cloud-provider.c
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
 * ownCloud does not have notebooks
 * neither colors
 * nor formating (but markdown could be used or even translated upstream
 * on web app
 */




#include "biji-own-cloud-note.h"
#include "biji-own-cloud-provider.h"


#define MINER_ID "gn:ownc:miner:96df9bc8-f542-427f-aa19-77b5a2c9a5f0"



static void get_mount (BijiOwnCloudProvider *self);

/* Properties */
enum {
  PROP_0,
  PROP_GOA_OBJECT,
  OCLOUD_PROV_PROP
};


static GParamSpec *properties[OCLOUD_PROV_PROP] = { NULL, };

/*
 * goa : object, account, providerInfo
 * gio : volume, mount, path, folder. folder get_path would return something we don't want.
 * data : notes
 * startup: tracker, queue
 * todo: monitor, cancel monitor
 */

struct _BijiOwnCloudProvider
{
  BijiProvider     parent_instance;

  BijiProviderInfo info;

  GoaObject        *object;
  GoaAccount       *account;

  GHashTable       *notes;
  GHashTable       *archives;
  GHashTable       *tracker;
  GQueue           *queue;

  GVolume          *volume; // only online
  GMount           *mount;  // only online
  gchar            *path;
  GFile            *folder; // either offline / online

  GFileMonitor     *monitor;
  GCancellable     *cancel_monitor;
};


G_DEFINE_TYPE (BijiOwnCloudProvider, biji_own_cloud_provider, BIJI_TYPE_PROVIDER)

typedef struct
{
  GFile *file;
  BijiOwnCloudProvider *self;

  BijiInfoSet set;
} BijiOCloudItem;


static BijiOCloudItem *
o_cloud_item_new (BijiOwnCloudProvider *self)
{
  BijiOCloudItem *item;

  item = g_slice_new (BijiOCloudItem);
  item->file = NULL;
  item->self = self;
  item->set.content = NULL;
  item->set.mtime = 0;
  item->set.created= 0;
  item->set.title = NULL;
  item->set.url = NULL;

  return item;
}


static void
o_cloud_item_free (BijiOCloudItem *item)
{
  g_object_unref (item->file);
  g_free (item->set.content);
  g_free (item->set.url);
  g_free (item->set.title);

  g_slice_free (BijiOCloudItem, item);
}




static void
biji_own_cloud_provider_finalize (GObject *object)
{
  BijiOwnCloudProvider *self;

  g_return_if_fail (BIJI_IS_OWN_CLOUD_PROVIDER (object));
  self = BIJI_OWN_CLOUD_PROVIDER (object);

  if (self->path != NULL)
    g_free (self->path);

  g_clear_object (&self->account);
  g_clear_object (&self->object);
  g_clear_object (&self->info.icon);
  g_clear_object (&self->folder);

  g_clear_pointer (&self->info.name, g_free);
  g_clear_pointer (&self->info.datasource, g_free);
  g_clear_pointer (&self->info.user, g_free);
  g_clear_pointer (&self->info.domain, g_free);

  g_queue_free_full (self->queue, g_object_unref);
  g_hash_table_unref (self->notes);
  g_hash_table_unref (self->tracker);

  G_OBJECT_CLASS (biji_own_cloud_provider_parent_class)->finalize (object);
}



static void
release_enum_cb (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_file_enumerator_close_finish (G_FILE_ENUMERATOR (source),
                                  res,
                                  NULL);
  g_object_unref (source);
}


static void handle_next_item (BijiOwnCloudProvider *self);


/* No color, hence we use default color */

static void
create_note_from_item (BijiOCloudItem *item)
{
  BijiNoteObj *note;
  GdkRGBA color;
  BijiManager *manager;
  gboolean online = (item->self->mount != NULL);

  manager = biji_provider_get_manager (BIJI_PROVIDER (item->self));

  note = biji_own_cloud_note_new_from_info (item->self,
                                            manager,
                                            &item->set,
                                            online);
  biji_manager_get_default_color (manager, &color);
  biji_note_obj_set_rgba (note, &color);
  g_hash_table_replace (item->self->notes,
                        g_strdup (item->set.url),
                        note);
}


static void
on_content (GObject *source,
            GAsyncResult *res,
            gpointer user_data)
{
  BijiOCloudItem *item;
  BijiOwnCloudProvider *self;
  GFile *file;
  GError *error;
  gboolean ok;
  gchar *contents;
  gsize length;

  file = G_FILE (source);
  item = user_data;
  self = item->self;
  error = NULL;
  length = 0;

  ok = g_file_load_contents_finish (file,
                                    res,
                                    &contents,
                                    &length,
                                    NULL,
                                    &error);

  if (!ok)
  {
    if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
    }

    o_cloud_item_free (item);
  }

  /*
   * File has been read, both create the note
   * then update db
   */

  else
  {
     item->set.content = contents;
     create_note_from_item (item);
     biji_tracker_ensure_ressource_from_info (
       biji_provider_get_manager (BIJI_PROVIDER (self)), &item->set);

     // TODO --> ensure_ressource callback.... o_cloud_item_free (item);
  }

  handle_next_item (self);
}



/*
 * If the tracker db is up  to date it provides the Info.
 * Otherwise the file is more recent than tracker :
 * do it the hard way
 *
 * but ownCloud mtime are not reliable so this does not work well.
 * editing file changes mtime. Not editing from Notes...
 *
 */

static void
check_info_maybe_read_file (BijiInfoSet *info,
                            gpointer user_data)
{
  BijiOCloudItem *item;
  BijiOwnCloudProvider *self;

  item = user_data;
  self = item->self;


  /* Create the note from cache (tracker store)
   * My escape char for plainTextContent are buggy */

  if (info != NULL)
  {
    item->set.content = g_strdup (info->content);
    item->set.created = info->created;
    create_note_from_item (item);
    o_cloud_item_free (item);
    biji_info_set_free (info);
    handle_next_item (self);
  }


  /* Store not up to date. We need to read the file */

  else
  {
    g_file_load_contents_async (item->file, NULL, on_content, item);
  }

}


static void
trash (gpointer urn_uuid, gpointer self)
{
  biji_tracker_trash_ressource (
      biji_provider_get_manager (BIJI_PROVIDER (self)), (gchar*) urn_uuid);
}



static void
handle_next_item (BijiOwnCloudProvider *self)
{
  BijiOCloudItem *item;
  GList *list;

  item = g_queue_pop_head (self->queue);

  if (item != NULL)
  {
    g_hash_table_remove (self->tracker, item->set.url);

    biji_tracker_check_for_info (biji_provider_get_manager (BIJI_PROVIDER (self)),
                                 item->set.url,
                                 item->set.mtime,
                                 check_info_maybe_read_file,
                                 item);
  }


  else
  {
    /* Post load tracker db clean-up */
    list = g_hash_table_get_values (self->tracker);
    g_list_foreach (list, trash, self);
    g_list_free (list);


    /* Now simply provide data to controller */
    list = g_hash_table_get_values (self->notes);
    BIJI_PROVIDER_GET_CLASS (self)->notify_loaded (BIJI_PROVIDER (self), list, BIJI_LIVING_ITEMS);
    g_list_free (list);
  }
}


static void
enumerate_next_files_ready_cb (GObject *source,
                               GAsyncResult *res,
                               gpointer user_data)
{
  GFileEnumerator *enumerator;
  GError *error;
  BijiOwnCloudProvider *self;
  GList *files, *l;

  g_return_if_fail (BIJI_IS_OWN_CLOUD_PROVIDER (user_data));

  enumerator = G_FILE_ENUMERATOR (source);
  error = NULL;
  self = BIJI_OWN_CLOUD_PROVIDER (user_data);

  files = g_file_enumerator_next_files_finish (enumerator, res, &error);
  g_file_enumerator_close_async (enumerator, G_PRIORITY_DEFAULT, NULL,
                                 release_enum_cb, NULL);

  if (error != NULL)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    return;
  }

  /* Open the file, have a dedicated deserializer */
  for (l = files; l != NULL; l = l->next)
  {
    GFileInfo *info;
    GTimeVal time = {0,0};
    BijiOCloudItem *item;

    info = l->data;
    item = o_cloud_item_new (self);
    item->set.title = g_strdup (g_file_info_get_name (info));
    item->set.url = g_build_filename
      (g_file_get_parse_name (self->folder),
       "/", item->set.title, NULL);

    g_file_info_get_modification_time (info, &time);
    item->set.mtime = time.tv_sec;
    item->set.created = g_file_info_get_attribute_uint64 (info, "time:created");
    item->set.datasource_urn = g_strdup (self->info.datasource);

    if (self->mount == NULL) /* offline (synced mode) */
      item->file = g_file_new_for_path (item->set.url);
    else                          /* online (webdav) */
      item->file = g_file_new_for_uri (item->set.url);

    g_queue_push_head (self->queue, item);
  }

  // TODO - create the dir monitor
  handle_next_item (self);
  g_list_free_full (files, g_object_unref);
}





static void
enumerate_children_ready_cb (GObject *source,
                             GAsyncResult *res,
                             gpointer user_data)
{
  GFile *location;
  GFileEnumerator *enumerator;
  GError *error;
  BijiOwnCloudProvider *self;

  g_return_if_fail (BIJI_IS_OWN_CLOUD_PROVIDER (user_data));

  error = NULL;
  location = G_FILE (source);
  self = BIJI_OWN_CLOUD_PROVIDER (user_data);
  enumerator = g_file_enumerate_children_finish (location, res, &error);

  if (error != NULL)
  {
    g_warning ("could not enumerate... %s", error->message);
    g_error_free (error);

    /* TODO : nice place to reset everything
     * and start again with fresh air ?
     * but we don't want to umount for the user */
    return;
  }

   g_file_enumerator_next_files_async (enumerator, G_MAXINT,
                                       G_PRIORITY_DEFAULT,
                                       NULL,
                                       enumerate_next_files_ready_cb,
                                       self);
}


/* Stock all existing urn-uuid. Then parse files */
static void
on_notes_mined (GObject       *source_object,
                GAsyncResult  *res,
                gpointer       user_data)
{
  BijiOwnCloudProvider *self;
  TrackerSparqlConnection *connect;
  TrackerSparqlCursor *cursor;
  GError *error;

  self = user_data;
  connect = TRACKER_SPARQL_CONNECTION (source_object);
  error = NULL;
  cursor = tracker_sparql_connection_query_finish (connect, res, &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  if (cursor)
  {
    while (tracker_sparql_cursor_next (cursor, NULL, NULL))
    {
      g_hash_table_insert (self->tracker,
                           g_strdup (tracker_sparql_cursor_get_string (cursor, 0, NULL)),
                           g_strdup (tracker_sparql_cursor_get_string (cursor, 1, NULL)));

    }
  }

  g_file_enumerate_children_async (self->folder,
                                   "standard::name,time::modified,time::created", 0,
                                   G_PRIORITY_DEFAULT,
                                   NULL,
                                   enumerate_children_ready_cb,
                                   self);
}


/* TODO use datasource so func is somewhat std:: */
static void
mine_notes (gboolean result, gpointer user_data)
{
  BijiOwnCloudProvider    *self;
  BijiProvider            *provider;
  const BijiProviderInfo  *info;
  gchar                   *query;


  self = user_data;
  provider = user_data;
  info = biji_provider_get_info (provider);

  /*
   * We could as well use nie:url to lookup existing db
   * but rather use dataSource since this might go to generic bProvider
   *
   * query = g_strdup_printf ("SELECT ?urn ?url WHERE {?urn a nfo:Note ; "
                           "nie:url ?url . FILTER ("
                           "fn:contains (?url,'%s'))}",
                           root);
   *
   */

  query = g_strdup_printf ("SELECT ?url ?urn WHERE {?urn a nfo:Note; "
                           " nie:dataSource '%s' ; nie:url ?url}",
                           info->datasource);

  tracker_sparql_connection_query_async (
      biji_manager_get_tracker_connection (
        biji_provider_get_manager (provider)),
      query,
      NULL,
      on_notes_mined,
      self);

  g_free (query);
}



static void
handle_mount (BijiOwnCloudProvider *self)
{
  GFile *root;

  root = NULL;
  if (G_IS_MOUNT (self->mount))
    root = g_mount_get_root (self->mount);


  if (G_IS_FILE (root))
  {
    /* OwnCloud Notes folder is not localized.
     * https://github.com/owncloud/notes/issues/7 */

    self->folder = g_file_get_child (root, "Notes");
    g_file_make_directory (self->folder, NULL, NULL);
    self->monitor = g_file_monitor_directory
      (self->folder, G_FILE_MONITOR_NONE, NULL, NULL); // cancel, error


    g_object_unref (root);
    biji_tracker_ensure_datasource (
      biji_provider_get_manager (BIJI_PROVIDER (self)),
      self->info.datasource,
      MINER_ID,
      mine_notes,
      self);

    return;
  }

  biji_provider_abort (BIJI_PROVIDER (self));
}



static void
on_owncloud_volume_mounted (GObject *source_object,
                            GAsyncResult *res,
                            gpointer user_data)
{
  BijiOwnCloudProvider *self;
  GError *error;

  error = NULL;
  self = BIJI_OWN_CLOUD_PROVIDER (user_data);

  /* bug #701021 makes this fail */
  g_volume_mount_finish (self->volume, res, &error);

  if (error != NULL)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    biji_provider_abort (BIJI_PROVIDER (self));
    return;
  }

  self->mount = g_volume_get_mount (self->volume);

  if (!G_IS_MOUNT (self->mount))
  {
    g_warning ("OwnCloud Provider : !G_IS_MOUNT");
    biji_provider_abort (BIJI_PROVIDER (self));
    return;
  }

  else
    handle_mount (self);
}


static void
get_mount (BijiOwnCloudProvider *self)
{
  GVolumeMonitor *monitor;
  GoaFiles *files;
  const gchar *uri;

  monitor = g_volume_monitor_get ();

  if (!GOA_IS_OBJECT (self->object))
  {
    biji_provider_abort (BIJI_PROVIDER (self));
    return;
  }

  files = goa_object_peek_files (self->object);

  if (GOA_IS_FILES (files))
  {
    uri = goa_files_get_uri  (files);
    self->path = g_strdup_printf ("%sNotes", uri);
    self->volume = g_volume_monitor_get_volume_for_uuid (monitor, uri);
    self->mount = g_volume_get_mount (self->volume);


    if (self->mount != NULL &&
        G_IS_MOUNT (self->mount))
    {
      handle_mount (self);
    }


    /* not already mounted. no matter, we mount. in theory. */
    //else if (self->mount == NULL)
    else
    {
      g_volume_mount (self->volume,
                      G_MOUNT_MOUNT_NONE,
                      NULL,
                      NULL,
                      on_owncloud_volume_mounted,
                      self);
    }
  }

  else /* GOA_IS_FILES */
  {
    biji_provider_abort (BIJI_PROVIDER (self));
  }

  g_object_unref (monitor);
}


/*
 * Try to read, from ownCloud client config,
 * where are notes (synced notes).
 *
 * If this fails, fallback to direct online work.
 */

static void
on_owncloudclient_read (GObject *source_object,
                        GAsyncResult *res,
                        gpointer user_data)
{
  BijiOwnCloudProvider *self = user_data;
  GFileInputStream *stream;
	GDataInputStream *reader;
  GError           *error = NULL;
  gchar            *buffer = NULL;
  gsize             length = 0;
  GString          *string;

  stream = g_file_read_finish (G_FILE (source_object),
                               res,
                               &error);

  /*
   * Any error will fallback to online.
   * Whenever file does not exist, do not warn */
  if (error)
  {
    if (error->domain == G_IO_ERROR_NOT_FOUND)
      g_message ("No ownCloud client configuration found.\nWork online.");

		else
      g_warning ("%s", error->message);

		g_error_free (error);
    get_mount (self);
	}

  /* Checkout `localPath' where notes are. */
  reader = g_data_input_stream_new (G_INPUT_STREAM (stream));
	while (!g_str_has_prefix (buffer, "localPath"))
  {
    g_free (buffer); /* NULL on 1st, this is fine. */
    buffer = g_data_input_stream_read_line (reader,
                                            &length,
                                            NULL,
                                            &error);

    if (error) /* very unlikely! */
      g_warning ("%s", error->message);

    if (!buffer) /* error. Or, localPath not found, end of file. */
    {
      get_mount (self);
      break;
		}
  }

  string = g_string_new (buffer);
  g_string_erase (string, 0, 10); // localPath=

  /* ok we have the path. Now create the notes */
  self->path = g_build_filename (string->str, "Notes", NULL);
  self->folder = g_file_new_for_path (self->path);
  g_file_make_directory (self->folder, NULL, &error);


  if (error)
  {
    /* This error is not a blocker - the file may already exist. */
    g_message ("Cannot create %s - %s", self->path, error->message);

    /* if so, just print the message and go on */
    if (error->code == G_IO_ERROR_EXISTS)
    {
      g_file_enumerate_children_async (self->folder,
          "standard::name,time::modified,time::created", 0,
          G_PRIORITY_DEFAULT,
          NULL,
          enumerate_children_ready_cb,
          self);
    }

    /* if the creation really failed, fall back to webdav */
    else
    {
      g_warning ("Cannot create %s - trying webdav", self->path);
      get_mount (self);
    }


    g_error_free (error);
  }


  g_free (buffer);
  g_string_free (string, TRUE);
  g_object_unref (stream);
  g_object_unref (reader);
}


/* Retrieve basic information,
 * then check for an ownCloud Client. */
static void
biji_own_cloud_provider_constructed (GObject *obj)
{
  BijiOwnCloudProvider *self;
  GError *error;
  GIcon *icon;
  gchar** identity;
  gchar *owncloudclient;
  GFile *client;

  G_OBJECT_CLASS (biji_own_cloud_provider_parent_class)->constructed (obj);

  self = BIJI_OWN_CLOUD_PROVIDER (obj);

  if (!GOA_IS_OBJECT (self->object))
  {
   biji_provider_abort (BIJI_PROVIDER (self));
   return;
  }

  self->account = goa_object_get_account (self->object);

  if (self->account != NULL)
  {
    const gchar *presentation_identity;

    self->info.unique_id = goa_account_get_id (self->account);
    self->info.datasource = g_strdup_printf ("gn:goa-account:%s",
                                             self->info.unique_id);
    self->info.name = g_strdup (goa_account_get_provider_name (self->account));

    presentation_identity = goa_account_get_presentation_identity (self->account);

    if (presentation_identity)
      {
        identity = g_strsplit (presentation_identity, "@", 2);

        if (identity[0])
          {
            self->info.user = g_strdup (identity[0]);
            if (identity[1])
              self->info.domain = g_strdup (identity[1]);
          }

        g_strfreev(identity);
      }

    error = NULL;
    icon = g_icon_new_for_string (goa_account_get_provider_icon (self->account),
                                  &error);
    if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      self->info.icon = NULL;
    }

    else
    {
      self->info.icon = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_INVALID);
      gtk_image_set_pixel_size (GTK_IMAGE (self->info.icon), 48);
      g_object_ref_sink (self->info.icon);
    }

    g_object_unref (icon);

    owncloudclient = g_build_filename (
        g_get_home_dir (),
        ".local/share/data/ownCloud/folders/ownCloud",
        NULL);
    client = g_file_new_for_path (owncloudclient);
    g_free (owncloudclient);

    if (g_file_query_exists (client, NULL))
      g_file_read_async (client, G_PRIORITY_DEFAULT_IDLE,
                       NULL, on_owncloudclient_read, self);
    else
			get_mount (self);

    g_object_unref (client);

    return;
  }

  biji_provider_abort (BIJI_PROVIDER (self));

}


/* ownCloud has no archive at the moment */
static void
ocloud_prov_load_archives (BijiProvider *provider)
{
  return;
}


static void
biji_own_cloud_provider_init (BijiOwnCloudProvider *self)
{
  self->notes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_object_unref);
  self->tracker = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  self->queue = g_queue_new ();
}


static void
biji_own_cloud_provider_set_property (GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  BijiOwnCloudProvider *self = BIJI_OWN_CLOUD_PROVIDER (object);


  switch (property_id)
    {
    case PROP_GOA_OBJECT:
      self->object = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_own_cloud_provider_get_property (GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  BijiOwnCloudProvider *self = BIJI_OWN_CLOUD_PROVIDER (object);

  switch (property_id)
    {
    case PROP_GOA_OBJECT:
      g_value_set_object (value, self->object);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static const BijiProviderInfo *
own_cloud_get_info       (BijiProvider *provider)
{
  BijiOwnCloudProvider *self;

  self = BIJI_OWN_CLOUD_PROVIDER (provider);
  return &(self->info);
}



/*
 * Note is created from sratch, without any file or tracker metadata
 * But as soon as note title changes,
 * things will go right.
 * Promise. */

static BijiNoteObj *
own_cloud_create_note (BijiProvider *provider,
                       const gchar  *str)
{
  BijiInfoSet info;
  BijiOwnCloudProvider *self;

  self = BIJI_OWN_CLOUD_PROVIDER (provider);
  info.url = NULL;
  info.title = NULL;
  info.mtime = g_get_real_time ();
  info.content = (gchar *) "";
  info.created = g_get_real_time ();

  return biji_own_cloud_note_new_from_info (
		   self,
       biji_provider_get_manager (provider),
       &info,
       (self->mount != NULL));
}



/* This is a dummy func. we can create a note with extra args
 * but can't use path, nor color, nor html. */

static BijiNoteObj *
own_cloud_create_full (BijiProvider  *provider,
                       const gchar   *suggested_path,
                       BijiInfoSet   *info,
                       const gchar   *html,
                       const GdkRGBA *color)
{
  BijiOwnCloudProvider *self;
  BijiNoteObj *retval;
  GdkRGBA override_color;
  BijiManager *manager;

  self = BIJI_OWN_CLOUD_PROVIDER (provider);
  manager = biji_provider_get_manager (provider);

  retval = biji_own_cloud_note_new_from_info (self, manager, info, (self->mount != NULL));
  biji_note_obj_set_html (retval, html);

  /* We do not use suggested color.
   * Rather use ook default */

  biji_manager_get_default_color (manager, &override_color);
  biji_note_obj_set_rgba (retval, &override_color);

  return retval;
}



GFile *
biji_own_cloud_provider_get_folder     (BijiOwnCloudProvider *self)
{
  return self->folder;
}


static void
biji_own_cloud_provider_class_init (BijiOwnCloudProviderClass *klass)
{
  GObjectClass *g_object_class;
  BijiProviderClass *provider_class;

  g_object_class = G_OBJECT_CLASS (klass);
  provider_class = BIJI_PROVIDER_CLASS (klass);

  g_object_class->finalize = biji_own_cloud_provider_finalize;
  g_object_class->get_property = biji_own_cloud_provider_get_property;
  g_object_class->set_property = biji_own_cloud_provider_set_property;
  g_object_class->constructed = biji_own_cloud_provider_constructed;

  provider_class->get_info = own_cloud_get_info;
  provider_class->create_new_note = own_cloud_create_note;
  provider_class->create_note_full = own_cloud_create_full;
  provider_class->load_archives = ocloud_prov_load_archives;

  properties[PROP_GOA_OBJECT] =
    g_param_spec_object("goa",
                        "The Goa Object",
                        "The Goa Object providing notes",
                        GOA_TYPE_OBJECT,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (g_object_class, OCLOUD_PROV_PROP, properties);
}


BijiProvider *
biji_own_cloud_provider_new (BijiManager *manager,
                             GoaObject *object)
{
  return g_object_new (BIJI_TYPE_OWN_CLOUD_PROVIDER,
                       "manager", manager,
                       "goa", object,
                       NULL);
}


gchar *
biji_own_cloud_provider_get_readable_path (BijiOwnCloudProvider *self)
{
  return self->path;
}
