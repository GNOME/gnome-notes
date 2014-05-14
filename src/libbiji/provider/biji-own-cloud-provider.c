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

struct BijiOwnCloudProviderPrivate_
{
  BijiProviderInfo info;

  GoaObject        *object;
  GoaAccount       *account;

  GHashTable       *notes;
  GHashTable       *archives;
  GHashTable       *tracker;
  GQueue           *queue;

  GVolume          *volume;
  GMount           *mount;
  gchar            *path;
  GFile            *folder;

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

  if (self->priv->path != NULL)
    g_free (self->priv->path);

  g_object_unref (self->priv->account);
  g_object_unref (self->priv->object);
  g_object_unref (self->priv->info.icon);

  g_clear_pointer (&self->priv->info.name, g_free);

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

  manager = biji_provider_get_manager (BIJI_PROVIDER (item->self));

  note = biji_own_cloud_note_new_from_info (item->self,
                                            manager,
                                            &item->set);
  biji_manager_get_default_color (manager, &color);
  biji_note_obj_set_rgba (note, &color);
  g_hash_table_replace (item->self->priv->notes,
                        item->set.url,
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

  item = g_queue_pop_head (self->priv->queue);

  if (item != NULL)
  {
    g_hash_table_remove (self->priv->tracker, item->set.url);

    biji_tracker_check_for_info (biji_provider_get_manager (BIJI_PROVIDER (self)),
                                 item->set.url,
                                 item->set.mtime,
                                 check_info_maybe_read_file,
                                 item);
  }


  else
  {
    /* Post load tracker db clean-up */
    list = g_hash_table_get_values (self->priv->tracker);
    g_list_foreach (list, trash, self);
    g_list_free (list);


    /* Now simply provide data to controller */
    list = g_hash_table_get_values (self->priv->notes);
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
    item->set.url = g_strconcat
      (g_file_get_parse_name (self->priv->folder),
       "/", item->set.title, NULL);

    g_file_info_get_modification_time (info, &time);
    item->set.mtime = time.tv_sec;
    item->set.created = g_file_info_get_attribute_uint64 (info, "time:created");
    item->set.datasource_urn = g_strdup (self->priv->info.datasource);
    item->file = g_file_new_for_uri (item->set.url);
    g_queue_push_head (self->priv->queue, item);
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
      g_hash_table_insert (self->priv->tracker,
                           g_strdup (tracker_sparql_cursor_get_string (cursor, 0, NULL)),
                           g_strdup (tracker_sparql_cursor_get_string (cursor, 1, NULL)));

    }
  }

  g_file_enumerate_children_async (self->priv->folder,
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
  if (G_IS_MOUNT (self->priv->mount))
    root = g_mount_get_root (self->priv->mount);


  if (G_IS_FILE (root))
  {
    /* OwnCloud Notes folder is not localized.
     * https://github.com/owncloud/notes/issues/7 */

    self->priv->folder = g_file_get_child (root, "Notes");
    self->priv->monitor = g_file_monitor_directory
      (self->priv->folder, G_FILE_MONITOR_NONE, NULL, NULL); // cancel, error


    g_object_unref (root);
    biji_tracker_ensure_datasource (
      biji_provider_get_manager (BIJI_PROVIDER (self)),
      self->priv->info.datasource,
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
  g_volume_mount_finish (self->priv->volume, res, &error);

  if (error != NULL)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    biji_provider_abort (BIJI_PROVIDER (self));
    return;
  }

  self->priv->mount = g_volume_get_mount (self->priv->volume);

  if (!G_IS_MOUNT (self->priv->mount))
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

  if (!GOA_IS_OBJECT (self->priv->object))
  {
    biji_provider_abort (BIJI_PROVIDER (self));
    return;
  }

  files = goa_object_peek_files (self->priv->object);

  if (GOA_IS_FILES (files))
  {
    uri = goa_files_get_uri  (files);
    self->priv->path = g_strdup_printf ("%sNotes", uri);
    self->priv->volume = g_volume_monitor_get_volume_for_uuid (monitor, uri);
    self->priv->mount = g_volume_get_mount (self->priv->volume);


    if (self->priv->mount != NULL &&
        G_IS_MOUNT (self->priv->mount))
    {
      handle_mount (self);
    }


    /* not already mounted. no matter, we mount. in theory. */
    //else if (self->priv->mount == NULL)
    else
    {
      g_volume_mount (self->priv->volume,
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


static void
biji_own_cloud_provider_constructed (GObject *obj)
{
  BijiOwnCloudProvider *self;
  BijiOwnCloudProviderPrivate *priv;
  GError *error;
  GIcon *icon;

  G_OBJECT_CLASS (biji_own_cloud_provider_parent_class)->constructed (obj);

  self = BIJI_OWN_CLOUD_PROVIDER (obj);
  priv = self->priv;


  if (!GOA_IS_OBJECT (priv->object))
  {
   biji_provider_abort (BIJI_PROVIDER (self));
   return;
  }

  priv->account = goa_object_get_account (priv->object);

  if (priv->account != NULL)
  {

    priv->info.unique_id = goa_account_get_id (priv->account);
    priv->info.datasource = g_strdup_printf ("gn:goa-account:%s",
                                             priv->info.unique_id);
    priv->info.name = g_strdup (goa_account_get_provider_name (priv->account));

    error = NULL;
    icon = g_icon_new_for_string (goa_account_get_provider_icon (priv->account),
                                  &error);
    if (error)
    {
      g_warning ("%s", error->message);
      g_error_free (error);
      priv->info.icon = NULL;
    }

    else
    {
      priv->info.icon = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_INVALID);
      gtk_image_set_pixel_size (GTK_IMAGE (priv->info.icon), 48);
      g_object_ref (priv->info.icon);
    }

    get_mount (self);
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
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_OWN_CLOUD_PROVIDER, BijiOwnCloudProviderPrivate);

  self->priv->notes = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  self->priv->tracker = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  self->priv->queue = g_queue_new ();
  self->priv->path = NULL;
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
      self->priv->object = g_value_dup_object (value);
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
      g_value_set_object (value, self->priv->object);
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
  return &(self->priv->info);
}



/*
 * Note is created from sratch, without any file or tracker metadata
 * But as soon as note title changes,
 * things will go right.
 * Promise. */

BijiNoteObj *
own_cloud_create_note         (BijiProvider *provider,
                               gchar        *str)
{
  BijiInfoSet info;

  info.url = NULL;
  info.title = NULL;
  info.mtime = g_get_real_time ();
  info.content = "";
  info.created = g_get_real_time ();

  return biji_own_cloud_note_new_from_info (
       BIJI_OWN_CLOUD_PROVIDER (provider),
       biji_provider_get_manager (provider),
       &info);
}



/* This is a dummy func. we can create a note with extra args
 * but can't use path, nor color, nor html. */

BijiNoteObj *
own_cloud_create_full (BijiProvider *provider,
                       gchar        *suggested_path,
                       BijiInfoSet  *info,
                       gchar        *html,
                       GdkRGBA      *color)
{
  BijiOwnCloudProvider *self;
  BijiNoteObj *retval;
  GdkRGBA override_color;
  BijiManager *manager;

  self = BIJI_OWN_CLOUD_PROVIDER (provider);
  manager = biji_provider_get_manager (provider);

  retval = biji_own_cloud_note_new_from_info (self, manager, info);
  biji_note_obj_set_html (retval, html);

  /* We do not use suggested color.
   * Rather use ook default */

  biji_manager_get_default_color (manager, &override_color);
  biji_note_obj_set_rgba (retval, &override_color);

  return retval;
}



GFile *
biji_own_cloud_provider_get_folder     (BijiOwnCloudProvider *provider)
{
  return provider->priv->folder;
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

  g_type_class_add_private ((gpointer)klass, sizeof (BijiOwnCloudProviderPrivate));
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
biji_own_cloud_provider_get_readable_path (BijiOwnCloudProvider *p)
{
  return p->priv->path;
}
