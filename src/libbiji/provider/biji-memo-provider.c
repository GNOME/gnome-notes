/*
 * biji-memo-provider.c
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
 * http://tools.ietf.org/html/rfc2445
 * 
 * Evolution UI offers to sync Memo to local computer
 * TODO: check this
 */


#include <libecal/libecal.h>        /* ECalClient      */
#include <libgd/gd.h>               /* Embed in frame  */

#include "biji-memo-provider.h"
#include "biji-memo-note.h"


#define MINER_ID "gn:memo:miner:fd48e15b-2460-4761-a7be-942616102fa6"

struct _BijiMemoProviderPrivate
{
  BijiProviderInfo     info;
  ESource             *source;
  ECalClient          *client;
  GtkWidget           *icon;

  /* Startup */
  GSList              *memos;
  GHashTable          *tracker;
  GHashTable          *items;
  GHashTable          *archives;
  GQueue              *queue;
};




G_DEFINE_TYPE (BijiMemoProvider, biji_memo_provider, BIJI_TYPE_PROVIDER);


/* Properties */
enum {
  PROP_0,
  PROP_SOURCE,
  BIJI_MEMO_PROP
};



static GParamSpec *properties[BIJI_MEMO_PROP] = { NULL, };

/* Memos */


gboolean
time_val_from_icaltime (icaltimetype *itt, glong *result)
{
  GTimeVal t;
  gchar *iso;

  t.tv_sec = 0;
  t.tv_usec = 0;

  iso = isodate_from_time_t (icaltime_as_timet (*itt));

  if (g_time_val_from_iso8601 (iso, &t))
  {
    *result = t.tv_sec;
    g_free (iso);
    return TRUE;
  }

  return FALSE;
}



gboolean
icaltime_from_time_val (glong t, icaltimetype *out)
{
  GTimeVal tv;
  GDate    *date;

  tv.tv_sec = t;
  tv.tv_usec = 0;

  date = g_date_new ();
  g_date_set_time_val (date, &tv);
  *out = icaltime_from_day_of_year (
    g_date_get_day_of_year (date),
    g_date_get_year (date));

  g_date_free (date);
  return TRUE;
}



typedef struct
{
  ECalComponent     *ecal;
  BijiMemoProvider  *self;
  BijiInfoSet        set;
} BijiMemoItem;



static BijiMemoItem *
memo_item_new (BijiMemoProvider *self)
{
  BijiMemoItem *item;

  item = g_slice_new (BijiMemoItem);
  item->ecal = NULL;
  item->self = self;

  item->set.content = NULL;
  item->set.mtime = 0;
  item->set.created = 0;
  item->set.title = NULL;
  item->set.url = NULL;

  return item;
}


/* ECalComponent do not transfer much values */
static void
memo_item_free (BijiMemoItem *item)
{
  g_free (item->set.content);
  g_slice_free (BijiMemoItem, item);
}



/* No color, hence we use default color */

static void
create_note_from_item (BijiMemoItem *item)
{
  BijiNoteObj *note;
  GdkRGBA color;
  BijiManager *manager;

  manager = biji_provider_get_manager (BIJI_PROVIDER (item->self));
  note = biji_memo_note_new_from_info (item->self,
                                       manager,
                                       &item->set,
				       item->ecal,
                                       item->set.content,
                                       item->self->priv->client);

  biji_manager_get_default_color (manager, &color);
  biji_note_obj_set_rgba (note, &color);
  g_hash_table_replace (item->self->priv->items,
                        item->set.url,
                        note);
}



static void
trash (gpointer urn_uuid, gpointer self)
{
  biji_tracker_trash_ressource (
      biji_provider_get_manager (BIJI_PROVIDER (self)), (gchar*) urn_uuid);
}


static void
handle_next_item (BijiMemoProvider *self)
{
  BijiMemoProviderPrivate *priv = self->priv;
  BijiMemoItem *item;
  GList        *list;

  item = g_queue_pop_head (self->priv->queue);

  if (item != NULL)
  {
    g_hash_table_remove (priv->tracker, item->set.url);

    create_note_from_item (item);
    /* debug pour tracker. Il faut en plus datasource->urn */
    g_debug ("created=%li", item->set.created);
    g_debug ("title=%s", item->set.title);
    g_debug ("url=%s", item->set.url);
    g_debug ("content=%s\n================\n\n\n", item->set.content);

    biji_tracker_ensure_ressource_from_info (
    biji_provider_get_manager (BIJI_PROVIDER (self)), &item->set);

    //memo_item_free (item);
    handle_next_item (self);
  }


  /* Loading and notes creation over. Provide notes to manager */
  else
  {
    /* Post load tracker db clean-up */
    list = g_hash_table_get_values (self->priv->tracker);
    g_list_foreach (list, trash, self);
    g_list_free (list);

    /* Now simply provide data to controller */
    list = g_hash_table_get_values (self->priv->items);
    BIJI_PROVIDER_GET_CLASS (self)->notify_loaded (BIJI_PROVIDER (self), list, BIJI_LIVING_ITEMS);
    g_list_free (list);
  }
}



static void
on_object_list_got (GObject      *obj,
                    GAsyncResult *res,
                    gpointer      user_data)
{
  GSList *l, *desc, *ll;
  GError *error;
  BijiMemoProvider *self = BIJI_MEMO_PROVIDER (user_data);

  error = NULL;
  e_cal_client_get_object_list_finish (self->priv->client,
                                       res,
                                       &self->priv->memos,
                                       &error);

  if (error)
  {
    g_warning ("e cal get obj list fin %s", error->message);
    g_error_free (error);
    return;
  }

  for (l=self->priv->memos; l!=NULL; l=l->next)
  {
    ECalComponent      *co; /* Memo */
    ECalComponentText   text;
    const gchar        *uid;
    BijiMemoItem       *item;
    icaltimetype       *t;
    glong               time, dtstart;
    ECalComponentDateTime tz;


    item = memo_item_new (self);
    item->set.datasource_urn = g_strdup (self->priv->info.datasource);
    co = item->ecal = e_cal_component_new_from_icalcomponent (l->data);

    /* Summary, url */
    e_cal_component_get_summary (co, &text);
    item->set.title = g_strdup (text.value);
    e_cal_component_get_uid (co, &uid);
    item->set.url = g_strdup (uid);


    /* Last modified, created */
    e_cal_component_get_dtstart (co, &tz);
    if (time_val_from_icaltime (tz.value, &time))
      dtstart = time;
    else
      dtstart = 0;
    // e_cal_component_free_datetime()

    e_cal_component_get_last_modified (co, &t); // or dtstart
    if (time_val_from_icaltime (t, &time))
      item->set.mtime = time;
    else
      item->set.mtime = dtstart;
    e_cal_component_free_icaltimetype (t);

    e_cal_component_get_created (co, &t); // or dtstart
    if (time_val_from_icaltime (t, &time))
      item->set.created = time;
    else
      item->set.created = dtstart;
    e_cal_component_free_icaltimetype (t);

    /* Description */
    e_cal_component_get_description_list (co, &desc);
    for (ll=desc; ll!=NULL; ll=ll->next)
    {
      ECalComponentText *txt = ll->data;

      if (txt->value != NULL)
      {
        item->set.content = g_strdup (txt->value);
	break;
      }
    }

    if (item->set.content == NULL)
      g_warning ("note %s has no content", item->set.title);


    e_cal_component_free_text_list (desc);
    g_queue_push_head (self->priv->queue, item);
  }

  handle_next_item (self);
}




static gchar *
i_want_all_memos (void)
{
  return "occur-in-time-range? (make-time \"19820421T204153Z\") (make-time \"20820423T232416Z\")";
}



/* Stock all existing urn-uuid. Then parse files */
static void
on_notes_mined (GObject       *source_object,
                GAsyncResult  *res,
                gpointer       user_data)
{
  BijiMemoProvider        *self;
  TrackerSparqlConnection *connect;
  TrackerSparqlCursor     *cursor;
  GError                  *error;

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

  e_cal_client_get_object_list (self->priv->client,
                                i_want_all_memos(), /* sexp, not null */
                                NULL, /* Cancellable */
                                on_object_list_got,
                                self);
}



/* From gnome-calendar -> ported to GdkRGBA */
GdkPixbuf*
get_pixbuf_from_color    (GdkRGBA              *color, gint size)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  gint width, height;
  GdkPixbuf *pix;


  width = height = size;
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr,
                        color->red,
                        color->green,
                        color->blue);
  cairo_rectangle (cr, 0, 0, width, height);
  cairo_fill (cr);
  cairo_destroy (cr);
  pix = gdk_pixbuf_get_from_surface (surface,
                                     0, 0,
                                     width, height);
  cairo_surface_destroy (surface);
  return pix;
}


static gboolean
_get_icon (BijiMemoProvider *self,
           GtkWidget        **result)
{
  ESourceExtension *ext;
  const gchar      *color;
  GdkRGBA           rgba;
  GdkPixbuf        *pix, *embed;
  GtkBorder         frame_slice = { 4, 3, 3, 6 };

  ext = e_source_get_extension (self->priv->source, E_SOURCE_EXTENSION_MEMO_LIST);
  color = e_source_selectable_get_color (E_SOURCE_SELECTABLE (ext));

  if (color == NULL || !gdk_rgba_parse (&rgba, color))
    return FALSE;

  pix = get_pixbuf_from_color (&rgba, 48);
  embed = gd_embed_image_in_frame (pix, "resource:///org/gnome/bijiben/thumbnail-frame.png",
                                    &frame_slice, &frame_slice);
  *result = gtk_image_new_from_pixbuf (embed);

  g_clear_object (&pix);
  g_clear_object (&embed);

  return TRUE;
}



/*
 * Once the client is connected,
 * mine notes for this.
 */
static void
on_client_connected (GObject      *obj,
                     GAsyncResult *res,
                     gpointer      user_data)
{
  GError *error;
  error = NULL;
  gchar *query;
  BijiMemoProvider *self = BIJI_MEMO_PROVIDER (user_data);

  e_cal_client_connect_finish (res, &error);

  if (error)
  {
      g_warning ("On Client Connected : %s", error->message);
      return;
  }

  self->priv->client = E_CAL_CLIENT (obj);
  query = g_strdup_printf ("SELECT ?url ?urn WHERE {?urn a nfo:Note; "
                           " nie:dataSource '%s' ; nie:url ?url}",
                           self->priv->info.datasource);

  tracker_sparql_connection_query_async (
      biji_manager_get_tracker_connection (
        biji_provider_get_manager (BIJI_PROVIDER (self))),
      query,
      NULL,
      on_notes_mined,
      self);

  g_free (query);
}


/* GObject */



static void
biji_memo_provider_constructed (GObject *obj)
{
  BijiMemoProvider        *self;
  BijiMemoProviderPrivate *priv;

  G_OBJECT_CLASS (biji_memo_provider_parent_class)->constructed (obj);

  self = BIJI_MEMO_PROVIDER (obj);
  priv = self->priv;

  /* Info */
  priv->info.unique_id = e_source_get_uid (priv->source);
  priv->info.datasource = g_strdup_printf ("memo:%s",
                                           priv->info.unique_id);
  priv->info.name = g_strdup (e_source_get_display_name (priv->source));
  if (!_get_icon (self, &priv->info.icon))
     priv->info.icon = gtk_image_new_from_icon_name ("user-home", GTK_ICON_SIZE_INVALID);

  gtk_image_set_pixel_size (GTK_IMAGE (priv->info.icon), 48);
  g_object_ref (priv->info.icon);

  e_cal_client_connect (self->priv->source,
                        E_CAL_CLIENT_SOURCE_TYPE_MEMOS,
			10, /* wait up to 10 seconds until the memo list is connected */
                        NULL, /* cancel */
                        on_client_connected,
                        self);
}



static void
on_object_created (GObject      *client,
                   GAsyncResult *res,
		   gpointer      user_data)
{
  GError *error = NULL;
  gchar  *out_uid;

  e_cal_client_create_object_finish (E_CAL_CLIENT (client),
                                     res,
                                     &out_uid,
				     &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    return;
  }

  g_free (out_uid);
}



BijiNoteObj *
memo_create_note (BijiProvider *provider,
                  gchar        *str)
{
  BijiMemoProvider *self = BIJI_MEMO_PROVIDER (provider);
  BijiInfoSet    info;
  BijiNoteObj   *note;
  icalcomponent *icalcomp;
  ECalComponent *comp;
  gchar         *title, *html;
  time_t         dttime;
  ECalComponentDateTime dt;
  icaltimezone  *zone;
  struct icaltimetype itt;


  title = NULL;

  if (str)
  {
    title = biji_manager_get_unique_title (
      biji_provider_get_manager (provider), str);
  }

  if (!title)
    title = g_strdup ("1");


  info.url = NULL;
  info.title = title;
  info.mtime = g_get_real_time ();
  info.content = title;
  info.created = g_get_real_time ();

  e_cal_client_get_default_object_sync (
    self->priv->client, &icalcomp, NULL, NULL);

  if (icalcomp == NULL)
    icalcomp = icalcomponent_new (ICAL_VJOURNAL_COMPONENT);

  comp = e_cal_component_new ();
  if (!e_cal_component_set_icalcomponent (comp, icalcomp))
  {
    icalcomponent_free (icalcomp);
    e_cal_component_set_new_vtype (comp, E_CAL_COMPONENT_JOURNAL);
  }


  /* Dates and commit sequence */
  dttime = time (NULL);
  zone = e_cal_client_get_default_timezone (self->priv->client);

  if (dttime)
  {
    itt = icaltime_from_timet_with_zone (
      dttime, FALSE, zone);
    dt.value = &itt;
    dt.tzid = icaltimezone_get_tzid (zone);

    e_cal_component_set_dtstart (comp, &dt);
    e_cal_component_set_dtend (comp, &dt);
    e_cal_component_set_last_modified (comp, &itt);
    e_cal_component_set_created (comp, &itt);
  }

  if (dttime)
    e_cal_component_commit_sequence (comp);


  /* make sure the component has an UID and info get it */
  if (! (info.url = icalcomponent_get_uid (icalcomp)))
  {
    gchar *uid;

    uid = e_cal_component_gen_uid ();
    icalcomponent_set_uid (icalcomp, uid);
    info.url = uid;
  }


  /* Create the note, push the new vjournal */
  note = biji_memo_note_new_from_info (
    self,
    biji_provider_get_manager (provider),
    &info,
    comp,
    str,
    self->priv->client);


  biji_note_obj_set_title (note, title);
  biji_note_obj_set_raw_text (note, title);
  html = html_from_plain_text (title);
  biji_note_obj_set_html (note, html);
  g_free (html);


  e_cal_client_create_object (self->priv->client,
                              icalcomp,
                              NULL, /* GCancellable */
                              on_object_created,
                              self);

  g_free (title);

  return note;
}




static void
biji_memo_provider_init (BijiMemoProvider *self)
{
  BijiMemoProviderPrivate *priv;

  priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_MEMO_PROVIDER, BijiMemoProviderPrivate);

  priv->queue = g_queue_new ();
  priv->items = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  priv->tracker = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  priv->memos = NULL;

}

static void
biji_memo_provider_finalize (GObject *object)
{
  e_cal_client_free_icalcomp_slist (BIJI_MEMO_PROVIDER (object)->priv->memos);

  G_OBJECT_CLASS (biji_memo_provider_parent_class)->finalize (object);
}



static void
biji_memo_provider_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BijiMemoProvider *self = BIJI_MEMO_PROVIDER (object);


  switch (property_id)
    {
    case PROP_SOURCE:
      self->priv->source = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
biji_memo_provider_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BijiMemoProvider *self = BIJI_MEMO_PROVIDER (object);

  switch (property_id)
    {
    case PROP_SOURCE:
      g_value_set_object (value, self->priv->source);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}



const BijiProviderInfo *
memo_provider_get_info (BijiProvider *provider)
{
  return &(BIJI_MEMO_PROVIDER (provider)->priv->info);
}


static void
biji_memo_provider_class_init (BijiMemoProviderClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  BijiProviderClass *provider_class = BIJI_PROVIDER_CLASS (klass);

  object_class->finalize = biji_memo_provider_finalize;
  object_class->constructed = biji_memo_provider_constructed;
  object_class->get_property = biji_memo_provider_get_property;
  object_class->set_property = biji_memo_provider_set_property;

  provider_class->get_info = memo_provider_get_info;
  provider_class->create_new_note = memo_create_note;
  provider_class->create_note_full = NULL;

  properties[PROP_SOURCE] =
    g_param_spec_object ("source",
                         "Provider ESource",
                         "ESource Memo associated to Provider",
                         E_TYPE_SOURCE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, BIJI_MEMO_PROP, properties);

  g_type_class_add_private (klass, sizeof (BijiMemoProviderPrivate));
}



BijiProvider *
biji_memo_provider_new (BijiManager *manager,
                        ESource     *source)
{
    return g_object_new (BIJI_TYPE_MEMO_PROVIDER,
                         "manager", manager,
                         "source",  source,
                         NULL);
}
