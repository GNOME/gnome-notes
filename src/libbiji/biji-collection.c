/*
 * biji-collection.c
 *
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
 * in current implementation, one cannot add a collection
 * to a collection
 * but tracker would be fine with this
 * given we prevent self-containment.
 */

/*
 * biji_create_collection_icon
 * is adapted from Photos (photos_utils_create_collection_icon),
 * which is ported from Documents
 */

#include <math.h>

#include "biji-collection.h"
#include "biji-tracker.h"


static void biji_collection_update_collected (GList *result, gpointer user_data);


struct BijiCollectionPrivate_
{

  gchar           *urn;
  gchar           *name;
  gint64           mtime;

  GdkPixbuf       *icon;
  GdkPixbuf       *emblem;

  GList           *collected_items;
};

static void biji_collection_finalize (GObject *object);

G_DEFINE_TYPE (BijiCollection, biji_collection, BIJI_TYPE_ITEM)

/* Properties */
enum {
  PROP_0,
  PROP_URN,
  PROP_NAME,
  PROP_MTIME,
  BIJI_COLL_PROPERTIES
};


/* Signals */
enum {
  COLLECTION_DELETED,
  COLLECTION_ICON_UPDATED,
  BIJI_COLLECTIONS_SIGNALS
};

static GParamSpec *properties[BIJI_COLL_PROPERTIES] = { NULL, };

static guint biji_collections_signals [BIJI_COLLECTIONS_SIGNALS] = { 0 };


static const gchar *
biji_collection_get_title (BijiItem *coll)
{
  BijiCollection *collection;

  g_return_val_if_fail (BIJI_IS_COLLECTION (coll), NULL);
  collection = BIJI_COLLECTION (coll);

  return collection->priv->name;
}


static const gchar *
biji_collection_get_uuid (BijiItem *coll)
{
  BijiCollection *collection;

  g_return_val_if_fail (BIJI_IS_COLLECTION (coll), NULL);
  collection = BIJI_COLLECTION (coll);

  return collection->priv->urn;
}


static GdkPixbuf *
biji_create_collection_icon (gint base_size, GList *pixbufs)
{
  cairo_surface_t *surface;
  cairo_t *cr;
  GdkPixbuf *pix;
  GdkPixbuf *ret_val;
  GList *l;
  GtkStyleContext *context;
  GtkWidgetPath *path;
  gint cur_x;
  gint cur_y;
  gint idx;
  gint padding;
  gint pix_height;
  gint pix_width;
  gint scale_size;
  gint tile_size;

  /* TODO: use notes thumbs */

  padding = MAX (floor (base_size / 10), 4);
  tile_size = (base_size - (3 * padding)) / 2;

  context = gtk_style_context_new ();
  gtk_style_context_add_class (context, "documents-collection-icon");

  path = gtk_widget_path_new ();
  gtk_widget_path_append_type (path, GTK_TYPE_ICON_VIEW);
  gtk_style_context_set_path (context, path);
  gtk_widget_path_unref (path);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, base_size, base_size);
  cr = cairo_create (surface);

  gtk_render_background (context, cr, 0, 0, base_size, base_size);

  l = pixbufs;
  idx = 0;
  cur_x = padding;
  cur_y = padding;


  while (l != NULL && idx < 4)
    {
      pix = l->data;
      pix_width = gdk_pixbuf_get_width (pix);
      pix_height = gdk_pixbuf_get_height (pix);

      scale_size = MIN (pix_width, pix_height);

      cairo_save (cr);

      cairo_translate (cr, cur_x, cur_y);

      cairo_rectangle (cr, 0, 0,
                       tile_size, tile_size);
      cairo_clip (cr);

      cairo_scale (cr, (gdouble) tile_size / (gdouble) scale_size, (gdouble) tile_size / (gdouble) scale_size);
      gdk_cairo_set_source_pixbuf (cr, pix, 0, 0);

      cairo_paint (cr);
      cairo_restore (cr);

      if ((idx % 2) == 0)
        {
          cur_x += tile_size + padding;
        }
      else
        {
          cur_x = padding;
          cur_y += tile_size + padding;
        }

      idx++;
      l = l->next;
    }

  ret_val = gdk_pixbuf_get_from_surface (surface, 0, 0, base_size, base_size);

  cairo_surface_destroy (surface);
  cairo_destroy (cr);
  g_object_unref (context);

  return ret_val;
}

static GList *
get_collected_pix (BijiCollection *self)
{
  GList *result = NULL, *l;

  for (l = self->priv->collected_items ; l != NULL; l = l->next)
  {
    if (BIJI_IS_ITEM (l->data))
      result = g_list_prepend (
                        result,
                        biji_item_get_pristine (BIJI_ITEM (l->data)));
  }

  return result;
}

static GdkPixbuf *
biji_collection_get_icon (BijiItem *coll)
{
  BijiCollection *self = BIJI_COLLECTION (coll);
  GList *pix;

  if (!self->priv->icon)
  {
    pix = get_collected_pix (self);
    self->priv->icon = biji_create_collection_icon (BIJI_ICON_WIDTH, pix);
    g_list_free (pix);
  }

  return self->priv->icon;
}


static GdkPixbuf *
biji_collection_get_emblem (BijiItem *coll)
{
  BijiCollection *self = BIJI_COLLECTION (coll);
  GList *pix;

  if (!self->priv->emblem)
  {
    pix = get_collected_pix (self);
    self->priv->emblem = biji_create_collection_icon (BIJI_EMBLEM_WIDTH,
                                                      get_collected_pix (self));
    g_list_free (pix);
  }

  return self->priv->emblem;
}



static gint64
biji_collection_get_mtime (BijiItem *coll)
{
  BijiCollection *self;

  g_return_val_if_fail (BIJI_IS_COLLECTION (coll), 0);
  self = BIJI_COLLECTION (coll);

  return self->priv->mtime;
}


static gboolean
biji_collection_trash (BijiItem *item)
{
  BijiCollection *self;
  g_return_val_if_fail (BIJI_IS_COLLECTION (item), FALSE);

  self = BIJI_COLLECTION (item);

  g_signal_emit (G_OBJECT (item), biji_collections_signals[COLLECTION_DELETED], 0);
  biji_remove_collection_from_tracker (biji_item_get_book (item), self->priv->urn);
  g_object_unref (self);

  return TRUE;
}


static gboolean
biji_collection_has_collection (BijiItem *item, gchar *collection)
{
  //todo
  return FALSE;
}


static gboolean
biji_collection_add_collection (BijiItem *item, BijiItem *coll, gchar *title)
{
  g_warning ("biji collection add collection is not implemented.");
  return FALSE;
}


static gboolean
biji_collection_remove_collection (BijiItem *item, BijiItem *collection)
{
  g_warning ("biji collection remove collection is not implemented.");
  return FALSE;
}


static void
biji_collection_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BijiCollection *self = BIJI_COLLECTION (object);


  switch (property_id)
    {
      case PROP_URN:
        self->priv->urn = g_strdup (g_value_get_string (value));
        break;
      case PROP_NAME:
        self->priv->name = g_strdup (g_value_get_string (value));
        break;
      case PROP_MTIME:
        self->priv->mtime = g_value_get_int64 (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static void
biji_collection_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BijiCollection *self = BIJI_COLLECTION (object);

  switch (property_id)
    {
      case PROP_URN:
        g_value_set_string (value, self->priv->urn);
        break;
      case PROP_NAME:
        g_value_set_string (value, self->priv->name);
        break;
      case PROP_MTIME:
        g_value_set_int64 (value, self->priv->mtime);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static void
on_collected_item_change (BijiCollection *self)
{
  BijiCollectionPrivate *priv = self->priv;
  BijiNoteBook *book;
  GList *l;

  book = biji_item_get_book (BIJI_ITEM (self));

  /* Diconnected any handler */
  for (l= priv->collected_items; l!= NULL; l=l->next)
  {
    g_signal_handlers_disconnect_by_func (l->data, on_collected_item_change, self);
  }

  /* Then re-process the whole stuff */
  biji_get_items_with_collection_async (book,
                                        self->priv->name,
                                        biji_collection_update_collected,
                                        self);
}

/* For convenience, items are retrieved async.
 * Thus use a signal once icon & emblem updated.*/
static void
biji_collection_update_collected (GList *result,
                                  gpointer user_data)
{
  BijiCollection *self = user_data;
  BijiCollectionPrivate *priv = self->priv;
  GList *l;

  g_clear_pointer (&priv->collected_items, g_list_free);
  g_clear_pointer (&priv->icon, g_object_unref);
  g_clear_pointer (&priv->emblem, g_object_unref);

  priv->collected_items = result;

  /* Connect */
  for (l = priv->collected_items; l!= NULL; l=l->next)
  {
    g_signal_connect_swapped (l->data, "color-changed",
                              G_CALLBACK (on_collected_item_change), self);

    g_signal_connect_swapped (l->data, "deleted",
                              G_CALLBACK (on_collected_item_change), self);
  }

  g_signal_emit (self, biji_collections_signals[COLLECTION_ICON_UPDATED], 0);
}

void
biji_collection_refresh (BijiCollection *collection)
{
  on_collected_item_change (collection);
}

static void
biji_collection_constructed (GObject *obj)
{
  BijiCollection *self = BIJI_COLLECTION (obj);
  BijiNoteBook *book;


  book = biji_item_get_book (BIJI_ITEM (obj));

  biji_get_items_with_collection_async (book,
                                        self->priv->name,
                                        biji_collection_update_collected,
                                        self);
}

static gboolean
say_no (BijiItem *item)
{
  return FALSE;
}


static void
biji_collection_class_init (BijiCollectionClass *klass)
{
  GObjectClass *g_object_class;
  BijiItemClass*  item_class;

  g_object_class = G_OBJECT_CLASS (klass);
  item_class = BIJI_ITEM_CLASS (klass);

  g_object_class->constructed = biji_collection_constructed;
  g_object_class->finalize = biji_collection_finalize;
  g_object_class->set_property = biji_collection_set_property;
  g_object_class->get_property = biji_collection_get_property;

  g_type_class_add_private ((gpointer)klass, sizeof (BijiCollectionPrivate));


  properties[PROP_URN] =
    g_param_spec_string ("urn",
                         "Collection URN",
                         "Collection URN as in Tracker",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  properties[PROP_NAME] =
    g_param_spec_string ("name",
                         "Collection label",
                         "The tag as a string",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  properties[PROP_MTIME] =
    g_param_spec_int64  ("mtime",
                         "Modification time",
                         "Last modified time",
                         G_MININT64, G_MAXINT64, 0,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (g_object_class, BIJI_COLL_PROPERTIES, properties);

  biji_collections_signals[COLLECTION_ICON_UPDATED] =
    g_signal_new ("icon-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  biji_collections_signals[COLLECTION_DELETED] =
    g_signal_new ("deleted" ,
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  /* Interface */
  item_class->get_title = biji_collection_get_title;
  item_class->get_uuid = biji_collection_get_uuid;
  item_class->get_icon = biji_collection_get_icon;
  item_class->get_emblem = biji_collection_get_emblem;
  item_class->get_pristine = biji_collection_get_emblem;
  item_class->get_mtime = biji_collection_get_mtime;
  item_class->has_color = say_no;
  item_class->trash = biji_collection_trash;
  item_class->is_collectable = say_no;
  item_class->has_collection = biji_collection_has_collection;
  item_class->add_collection = biji_collection_add_collection;
  item_class->remove_collection = biji_collection_remove_collection;
}


static void
biji_collection_finalize (GObject *object)
{
  BijiCollection *self;
  g_return_if_fail (BIJI_IS_COLLECTION (object));

  self = BIJI_COLLECTION (object);
  g_free (self->priv->name);
  g_free (self->priv->urn);

  G_OBJECT_CLASS (biji_collection_parent_class)->finalize (object);
}


static void
biji_collection_init (BijiCollection *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_COLLECTION, BijiCollectionPrivate);

  self->priv->mtime = 0;

  self->priv->icon = NULL;
  self->priv->emblem = NULL;

  self->priv->collected_items = NULL;
}


BijiCollection *
biji_collection_new (GObject *book, gchar *urn, gchar *name, gint64 mtime)
{
  return g_object_new (BIJI_TYPE_COLLECTION,
                       "note-book", book,
                       "name",      name,
                       "urn",       urn,
                       "mtime",     mtime,
                       NULL);
}
