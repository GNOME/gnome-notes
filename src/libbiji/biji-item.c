/*
 * biji-item.c
 *
 * Copyright 2013 Pierre-Yves Luyten <py@luyten.fr>
 *
 * Bijiben is free software: you can redistribute it and/or modify it
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

#include "biji-item.h"
#include "biji-manager.h"

/* Properties */
enum {
  PROP_0,
  PROP_BOOK,
  BIJI_ITEM_PROP
};


static GParamSpec *properties[BIJI_ITEM_PROP] = { NULL, };


/* Signals */
enum {
  ITEM_TRASHED,
  ITEM_DELETED,
  ITEM_RESTORED,
  BIJI_ITEM_SIGNALS
};

static guint biji_item_signals [BIJI_ITEM_SIGNALS] = { 0 };


struct BijiItemPrivate_
{
  BijiManager *manager;
};

static void biji_item_finalize (GObject *object);

G_DEFINE_TYPE (BijiItem, biji_item, G_TYPE_OBJECT)


static void
biji_item_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
  BijiItem *self = BIJI_ITEM (object);


  switch (property_id)
    {
    case PROP_BOOK:
      self->priv->manager = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
biji_item_get_property (GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
  BijiItem *self = BIJI_ITEM (object);

  switch (property_id)
    {
    case PROP_BOOK:
      g_value_set_object (value, self->priv->manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_item_class_init (BijiItemClass *klass)
{
  GObjectClass *g_object_class;

  g_object_class = G_OBJECT_CLASS (klass);

  g_object_class->get_property = biji_item_get_property;
  g_object_class->set_property = biji_item_set_property;
  g_object_class->finalize = biji_item_finalize;

  properties[PROP_BOOK] =
    g_param_spec_object("manager",
                        "Note Manager",
                        "The Note Manager controlling this item",
                        BIJI_TYPE_MANAGER,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (g_object_class, BIJI_ITEM_PROP, properties);


  biji_item_signals[ITEM_DELETED] =
    g_signal_new ("deleted",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  biji_item_signals[ITEM_TRASHED] =
    g_signal_new ("trashed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  biji_item_signals[ITEM_RESTORED] =
    g_signal_new ("restored",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__STRING,
                  G_TYPE_NONE,
                  1,
                  G_TYPE_STRING);

  g_type_class_add_private ((gpointer)klass, sizeof (BijiItemPrivate));
}


static void
biji_item_finalize (GObject *object)
{
  g_return_if_fail (BIJI_IS_ITEM (object));

  G_OBJECT_CLASS (biji_item_parent_class)->finalize (object);
}


static void
biji_item_init (BijiItem *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_ITEM, BijiItemPrivate);
  self->priv->manager = NULL;
}

const gchar *
biji_item_get_title         (BijiItem *item)
{
  return BIJI_ITEM_GET_CLASS (item)->get_title (item);
}

const gchar *
biji_item_get_uuid          (BijiItem *item)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), NULL);

  return BIJI_ITEM_GET_CLASS (item)->get_uuid (item);
}


gpointer
biji_item_get_manager     (BijiItem *item)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), NULL);

  return item->priv->manager;
}


cairo_surface_t *
biji_item_get_icon          (BijiItem *item,
                             gint scale)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), NULL);

  return BIJI_ITEM_GET_CLASS (item)->get_icon (item, scale);
}

cairo_surface_t *
biji_item_get_emblem        (BijiItem *item,
                             gint scale)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), NULL);

  return BIJI_ITEM_GET_CLASS (item)->get_emblem (item, scale);
}

cairo_surface_t *
biji_item_get_pristine        (BijiItem *item,
                               gint scale)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), NULL);

  return BIJI_ITEM_GET_CLASS (item)->get_pristine (item, scale);
}


const gchar *
biji_item_get_place           (BijiItem *item)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), NULL);

  return BIJI_ITEM_GET_CLASS (item)->get_place (item);
}


gint64
biji_item_get_mtime           (BijiItem *item)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), 0);

  return BIJI_ITEM_GET_CLASS (item)->get_mtime (item);
}


gboolean
biji_item_has_color           (BijiItem *item)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), FALSE);

  return BIJI_ITEM_GET_CLASS (item)->has_color (item);
}

gboolean
biji_item_trash               (BijiItem *item)
{
  gboolean retval;


  g_return_val_if_fail (BIJI_IS_ITEM (item), FALSE);

  retval = BIJI_ITEM_GET_CLASS (item)->trash (item);
  if (retval == TRUE)
    g_signal_emit_by_name (item, "trashed", NULL);

  return retval;
}


gboolean
biji_item_restore             (BijiItem *item)
{
  gboolean retval;
  gchar   *old_uuid;

  g_return_val_if_fail (BIJI_IS_ITEM (item), FALSE);
  old_uuid = NULL;

  retval = BIJI_ITEM_GET_CLASS (item)->restore (item, &old_uuid);
  if (retval == TRUE)
  {
    g_signal_emit_by_name (item, "restored", old_uuid, NULL);
    if (old_uuid != NULL)
      g_free (old_uuid);
  }

  return retval;
}



gboolean
biji_item_delete               (BijiItem *item)
{
  gboolean retval;

  g_return_val_if_fail (BIJI_IS_ITEM (item), FALSE);

  retval = BIJI_ITEM_GET_CLASS (item)->delete (item);
  if (retval == TRUE)
    g_signal_emit_by_name (item, "deleted", NULL);

  return retval;
}



gboolean
biji_item_is_collectable       (BijiItem *item)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), FALSE);

  return BIJI_ITEM_GET_CLASS (item)->is_collectable (item);
}

gboolean
biji_item_has_notebook      (BijiItem *item, gchar *coll)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), FALSE);

  return BIJI_ITEM_GET_CLASS (item)->has_notebook (item, coll);
}

gboolean         biji_item_add_notebook      (BijiItem *item, BijiItem *coll, gchar *title)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), FALSE);
  g_return_val_if_fail (BIJI_IS_ITEM (coll), FALSE);

  return BIJI_ITEM_GET_CLASS (item)->add_notebook (item, coll, title);
}

gboolean         biji_item_remove_notebook   (BijiItem *item, BijiItem *collection)
{
  g_return_val_if_fail (BIJI_IS_ITEM (item), FALSE);

  return BIJI_ITEM_GET_CLASS (item)->remove_notebook (item, collection);
}
