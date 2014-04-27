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

/*
 * ECALCOMPONENT
 * uid, categories, created (or dtstart), last modified,
 * summary,
 *
 * but the tricky part is description,
 * since ical says as-many-descriptions-as-you-want
 * while we need a single content.
 * Let's start with something simple: take the last
 * description we find.
 *
 * e_cal_component_free_******
 *
 */


/*

void                e_cal_client_get_timezone           (ECalClient *client,
                                                         const gchar *tzid,
                                                         GCancellable *cancellable,
                                                         GAsyncReadyCallback callback,
                                                         gpointer user_data);
*/

  for (l=self->priv->memos; l!=NULL; l=l->next)
  {
    ECalComponent      *co; /* Memo */
    ECalComponentText   text;
    const gchar        *uid;
    BijiMemoItem       *item;
    icaltimetype       *t;
    gchar              *iso;
    //GTimeVal            time = {0;0};
    //gint64              sec;

    item = memo_item_new (self);
    item->set.datasource_urn = g_strdup (self->priv->info.datasource);
    co = item->ecal = e_cal_component_new_from_icalcomponent (l->data);

#ifdef FALSE
//iso8601 => g_time_val.tv_sec
struct icaltimetype
{
int year;	/**< Actual year, e.g. 2001. */
int month;	/**< 1 (Jan) to 12 (Dec). */
int day;
int hour;
int minute;
int second;
int is_utc; /**< 1-> time is in UTC timezone */
int is_date; /**< 1 -> interpret this as date. */
int is_daylight; /**< 1 -> time is in daylight savings time. */
const icaltimezone *zone;	/**< timezone */
};
#endif

    e_cal_component_get_summary (co, &text);
    item->set.title = g_strdup (text.value);
    e_cal_component_get_uid (co, &uid);
    item->set.url = g_strdup (uid);

    /* Set url contains timezone. Not the-right-thing-to-do however */

    // time: we expect something like time.tv_sec (sec since)
    e_cal_component_get_last_modified (co, &t); // or dtstart
//gchar *             isodate_from_time_t                 (time_t t);

    iso = g_strdup_printf  ("%i-%i-%iT%i:%i:%i",
                            t->year,
                            t->month,
			    t->day,
			    t->hour,
			    t->minute,
			    t->second);
    g_warning ("iso=%s", iso);

    item->set.mtime = 0;
    e_cal_component_free_icaltimetype (t);
    e_cal_component_get_created (co, &t); // or dtstart
    item->set.created = 0;
    e_cal_component_free_icaltimetype (t);

    e_cal_component_get_description_list (co, &desc);
    for (ll=desc; ll!=NULL; ll=ll->next)
    {
      ECalComponentText *txt = ll->data;

      if (txt->value != NULL)
      {
        //g_warning ("txt value got: memo %s has\n %s\n====", item->set.title, txt->value);
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
  g_warning ("FIXME: memo provider query is wrong");
/*
  return "occur-in-time-range? "
         "(make-time \"19000101T204153Z\") "
         "(make-time \"22220202T204153Z\")";*/
  return "occur-in-time-range? (make-time \"20110421T204153Z\") (make-time \"20140423T232416Z\")";
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



/* From gnome-calendar */
GdkPixbuf*
get_pixbuf_from_color    (GdkColor              *color, gint size)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  gint width, height;
  GdkPixbuf *pix;


  width = height = size;
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, width, height);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr,
                        color->red / 65535.0,
                        color->green / 65535.0,
                        color->blue / 65535.0);
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
  GdkColor          color;
  GdkPixbuf        *pix, *embed;
  GtkBorder         frame_slice = { 4, 3, 3, 6 };

  ext = e_source_get_extension (self->priv->source, E_SOURCE_EXTENSION_MEMO_LIST);
  if (!gdk_color_parse (e_source_selectable_get_color (E_SOURCE_SELECTABLE (ext)), &color))
    return FALSE;

  pix = get_pixbuf_from_color (&color, 48);
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
                        NULL, /* cancel */
                        on_client_connected,
                        self);
}



/*


void                e_cal_client_create_object          (ECalClient *client,
                                                         icalcomponent *icalcomp,
                                                         GCancellable *cancellable,
                                                         GAsyncReadyCallback callback,
                                                         gpointer user_data);
gboolean            e_cal_client_create_object_finish   (ECalClient *client,
                                                         GAsyncResult *result,
                                                         gchar **out_uid,
                                                         GError **error);

*/



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
  provider_class->create_new_note = NULL;
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
