/*
 * biji-notebook.c
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
 * in current implementation, one cannot add a notebook
 * to a notebook
 * but tracker would be fine with this
 * given we prevent self-containment.
 */

/*
 * biji_create_notebook_icon
 * is adapted from Photos (photos_utils_create_notebook_icon),
 * which is ported from Documents
 */

#include <math.h>

#include "biji-notebook.h"
#include "biji-tracker.h"


static void biji_notebook_update_collected (GList *result, gpointer user_data);


struct BijiNotebookPrivate_
{

  gchar           *urn;
  gchar           *name;
  gint64           mtime;

  cairo_surface_t *icon;
  cairo_surface_t *emblem;

  GList           *collected_items;
};

static void biji_notebook_finalize (GObject *object);

G_DEFINE_TYPE (BijiNotebook, biji_notebook, BIJI_TYPE_ITEM)

/* Properties */
enum {
  PROP_0,
  PROP_URN,
  PROP_NAME,
  PROP_MTIME,
  BIJI_NOTEBOOK_PROPERTIES
};


/* Signals */
enum {
  NOTEBOOK_ICON_UPDATED,
  BIJI_NOTEBOOKS_SIGNALS
};

static GParamSpec *properties[BIJI_NOTEBOOK_PROPERTIES] = { NULL, };

static guint biji_notebooks_signals [BIJI_NOTEBOOKS_SIGNALS] = { 0 };


static const gchar *
biji_notebook_get_title (BijiItem *coll)
{
  BijiNotebook *notebook;

  g_return_val_if_fail (BIJI_IS_NOTEBOOK (coll), NULL);
  notebook = BIJI_NOTEBOOK (coll);

  return notebook->priv->name;
}


static const gchar *
biji_notebook_get_uuid (BijiItem *coll)
{
  BijiNotebook *notebook;

  g_return_val_if_fail (BIJI_IS_NOTEBOOK (coll), NULL);
  notebook = BIJI_NOTEBOOK (coll);

  return notebook->priv->urn;
}


static cairo_surface_t *
biji_create_notebook_icon (gint base_size, gint scale, GList *surfaces)
{
  cairo_surface_t *surface, *pix;
  cairo_t *cr;
  GList *l;
  GtkStyleContext *context;
  GtkWidgetPath *path;
  gint cur_x;
  gint cur_y;
  gint idx;
  gint padding;
  gint pix_height;
  gint pix_width;
  gdouble pix_scale_x;
  gdouble pix_scale_y;
  gint scale_size;
  gint tile_size;

  /* TODO: use notes thumbs */

  padding = MAX (floor (base_size / 10), 4);
  tile_size = (base_size - (3 * padding)) / 2;

  context = gtk_style_context_new ();
  gtk_style_context_add_class (context, "biji-notebook-icon");

  path = gtk_widget_path_new ();
  gtk_widget_path_append_type (path, GTK_TYPE_ICON_VIEW);
  gtk_style_context_set_path (context, path);
  gtk_widget_path_unref (path);

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, base_size * scale, base_size * scale);
  cairo_surface_set_device_scale (surface, scale, scale);
  cr = cairo_create (surface);

  gtk_render_frame (context, cr, 0, 0, base_size, base_size);
  gtk_render_background (context, cr, 0, 0, base_size, base_size);

  l = surfaces;
  idx = 0;
  cur_x = padding;
  cur_y = padding;


  while (l != NULL && idx < 4)
    {
      pix = l->data;
      cairo_surface_get_device_scale (pix, &pix_scale_x, &pix_scale_y);
      pix_width = cairo_image_surface_get_width (pix) / (gint) pix_scale_x;
      pix_height = cairo_image_surface_get_height (pix) / (gint) pix_scale_y;

      scale_size = MIN (pix_width, pix_height);

      cairo_save (cr);

      cairo_translate (cr, cur_x, cur_y);

      cairo_rectangle (cr, 0, 0,
                       tile_size, tile_size);
      cairo_clip (cr);

      cairo_scale (cr, (gdouble) tile_size / (gdouble) scale_size, (gdouble) tile_size / (gdouble) scale_size);
      cairo_set_source_surface (cr, pix, 0, 0);

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

  cairo_destroy (cr);
  g_object_unref (context);

  return surface;
}

static GList *
get_collected_pix (BijiNotebook *self,
                   gint scale)
{
  GList *result = NULL, *l;

  for (l = self->priv->collected_items ; l != NULL; l = l->next)
  {
    if (BIJI_IS_ITEM (l->data))
      result = g_list_prepend (
                        result,
                        biji_item_get_pristine (BIJI_ITEM (l->data), scale));
  }

  return result;
}

static cairo_surface_t *
biji_notebook_get_icon (BijiItem *coll,
                        gint scale)
{
  BijiNotebook *self = BIJI_NOTEBOOK (coll);
  GList *pix;

  if (!self->priv->icon)
  {
    pix = get_collected_pix (self, scale);
    self->priv->icon = biji_create_notebook_icon (BIJI_ICON_WIDTH, scale, pix);
    g_list_free (pix);
  }

  return self->priv->icon;
}


static cairo_surface_t *
biji_notebook_get_emblem (BijiItem *coll,
                          gint scale)
{
  BijiNotebook *self = BIJI_NOTEBOOK (coll);
  GList *pix;

  if (!self->priv->emblem)
  {
    pix = get_collected_pix (self, scale);
    self->priv->emblem = biji_create_notebook_icon (BIJI_EMBLEM_WIDTH, scale, pix);
    g_list_free (pix);
  }

  return self->priv->emblem;
}



static gint64
biji_notebook_get_mtime (BijiItem *coll)
{
  BijiNotebook *self;

  g_return_val_if_fail (BIJI_IS_NOTEBOOK (coll), 0);
  self = BIJI_NOTEBOOK (coll);

  return self->priv->mtime;
}


/* As of today, notebooks are only local
 * We'll need to override this */

static const gchar *
biji_notebook_get_place (BijiItem *coll)
{
  return _("Local");
}


static gboolean
biji_notebook_trash (BijiItem *item)
{
  BijiNotebook *self;
  g_return_val_if_fail (BIJI_IS_NOTEBOOK (item), FALSE);

  self = BIJI_NOTEBOOK (item);
  biji_remove_notebook_from_tracker (biji_item_get_manager (item), self->priv->urn);

  return TRUE;
}


static gboolean
biji_notebook_restore (BijiItem  *item,
                       gchar    **old_uuid)
{
  g_warning ("Notebooks restore is not yet implemented");
  return FALSE;
}


static gboolean
biji_notebook_has_notebook (BijiItem *item, gchar *notebook)
{
  //todo
  return FALSE;
}


static gboolean
biji_notebook_add_notebook (BijiItem *item, BijiItem *coll, gchar *title)
{
  g_warning ("biji notebook add notebook is not implemented.");
  return FALSE;
}


static gboolean
biji_notebook_remove_notebook (BijiItem *item, BijiItem *notebook)
{
  g_warning ("biji notebook remove notebook is not implemented.");
  return FALSE;
}


static void
biji_notebook_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BijiNotebook *self = BIJI_NOTEBOOK (object);


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
biji_notebook_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BijiNotebook *self = BIJI_NOTEBOOK (object);

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
on_collected_item_change (BijiNotebook *self)
{
  BijiNotebookPrivate *priv = self->priv;
  BijiManager *manager;
  GList *l;

  manager = biji_item_get_manager (BIJI_ITEM (self));

  /* Diconnected any handler */
  for (l= priv->collected_items; l!= NULL; l=l->next)
  {
    g_signal_handlers_disconnect_by_func (l->data, on_collected_item_change, self);
  }

  /* Then re-process the whole stuff */
  biji_get_items_with_notebook_async (manager,
                                        self->priv->name,
                                        biji_notebook_update_collected,
                                        self);
}

/* For convenience, items are retrieved async.
 * Thus use a signal once icon & emblem updated.*/
static void
biji_notebook_update_collected (GList *result,
                                  gpointer user_data)
{
  BijiNotebook *self = user_data;
  BijiNotebookPrivate *priv = self->priv;
  GList *l;

  g_clear_pointer (&priv->collected_items, g_list_free);
  g_clear_pointer (&priv->icon, cairo_surface_destroy);
  g_clear_pointer (&priv->emblem, cairo_surface_destroy);

  priv->collected_items = result;

  /* Connect */
  for (l = priv->collected_items; l!= NULL; l=l->next)
  {
    g_signal_connect_swapped (l->data, "color-changed",
                              G_CALLBACK (on_collected_item_change), self);

    g_signal_connect_swapped (l->data, "trashed",
                              G_CALLBACK (on_collected_item_change), self);
  }

  g_signal_emit (self, biji_notebooks_signals[NOTEBOOK_ICON_UPDATED], 0);
}

void
biji_notebook_refresh (BijiNotebook *notebook)
{
  on_collected_item_change (notebook);
}

static void
biji_notebook_constructed (GObject *obj)
{
  BijiNotebook *self = BIJI_NOTEBOOK (obj);
  BijiManager *manager;


  manager = biji_item_get_manager (BIJI_ITEM (obj));

  biji_get_items_with_notebook_async (manager,
                                        self->priv->name,
                                        biji_notebook_update_collected,
                                        self);
}

static gboolean
say_no (BijiItem *item)
{
  return FALSE;
}


static void
biji_notebook_class_init (BijiNotebookClass *klass)
{
  GObjectClass *g_object_class;
  BijiItemClass*  item_class;

  g_object_class = G_OBJECT_CLASS (klass);
  item_class = BIJI_ITEM_CLASS (klass);

  g_object_class->constructed = biji_notebook_constructed;
  g_object_class->finalize = biji_notebook_finalize;
  g_object_class->set_property = biji_notebook_set_property;
  g_object_class->get_property = biji_notebook_get_property;

  g_type_class_add_private ((gpointer)klass, sizeof (BijiNotebookPrivate));


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

  g_object_class_install_properties (g_object_class, BIJI_NOTEBOOK_PROPERTIES, properties);

  biji_notebooks_signals[NOTEBOOK_ICON_UPDATED] =
    g_signal_new ("icon-changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  /* Interface */
  item_class->get_title = biji_notebook_get_title;
  item_class->get_uuid = biji_notebook_get_uuid;
  item_class->get_icon = biji_notebook_get_icon;
  item_class->get_emblem = biji_notebook_get_emblem;
  item_class->get_pristine = biji_notebook_get_emblem;
  item_class->get_mtime = biji_notebook_get_mtime;
  item_class->get_place = biji_notebook_get_place;
  item_class->has_color = say_no;
  item_class->trash = biji_notebook_trash;
  item_class->restore = biji_notebook_restore;
  item_class->is_collectable = say_no;
  item_class->has_notebook = biji_notebook_has_notebook;
  item_class->add_notebook = biji_notebook_add_notebook;
  item_class->remove_notebook = biji_notebook_remove_notebook;
}


static void
biji_notebook_finalize (GObject *object)
{
  BijiNotebook *self;
  g_return_if_fail (BIJI_IS_NOTEBOOK (object));

  self = BIJI_NOTEBOOK (object);
  g_free (self->priv->name);
  g_free (self->priv->urn);

  G_OBJECT_CLASS (biji_notebook_parent_class)->finalize (object);
}


static void
biji_notebook_init (BijiNotebook *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_NOTEBOOK, BijiNotebookPrivate);

  self->priv->mtime = 0;

  self->priv->icon = NULL;
  self->priv->emblem = NULL;

  self->priv->collected_items = NULL;
}


BijiNotebook *
biji_notebook_new (GObject *manager, gchar *urn, gchar *name, gint64 mtime)
{
  return g_object_new (BIJI_TYPE_NOTEBOOK,
                       "manager", manager,
                       "name",      name,
                       "urn",       urn,
                       "mtime",     mtime,
                       NULL);
}
