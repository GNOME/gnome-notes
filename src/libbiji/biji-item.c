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

struct BijiItemPrivate_
{
  gpointer dummy;
};

static void biji_item_finalize (GObject *object);

G_DEFINE_TYPE (BijiItem, biji_item, G_TYPE_OBJECT)


static void
biji_item_class_init (BijiItemClass *klass)
{
  GObjectClass *g_object_class;

  g_object_class = G_OBJECT_CLASS (klass);

  g_object_class->finalize = biji_item_finalize;

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
}

const gchar *
biji_item_get_title         (BijiItem *item)
{
  return BIJI_ITEM_GET_CLASS (item)->get_title (item);
}

const gchar *
biji_item_get_uuid          (BijiItem *item)
{
  return BIJI_ITEM_GET_CLASS (item)->get_uuid (item);
}

GdkPixbuf *
biji_item_get_icon          (BijiItem *item)
{
  return BIJI_ITEM_GET_CLASS (item)->get_icon (item);
}

GdkPixbuf *
biji_item_get_emblem        (BijiItem *item)
{
  return BIJI_ITEM_GET_CLASS (item)->get_emblem (item);
}

GdkPixbuf *
biji_item_get_pristine        (BijiItem *item)
{
  return BIJI_ITEM_GET_CLASS (item)->get_pristine (item);
}



gint64
biji_item_get_mtime           (BijiItem *item)
{
  return BIJI_ITEM_GET_CLASS (item)->get_mtime (item);
}

gboolean
biji_item_trash               (BijiItem *item)
{
  return BIJI_ITEM_GET_CLASS (item)->trash (item);
}

gboolean
biji_item_has_collection      (BijiItem *item, gchar *coll)
{
  return BIJI_ITEM_GET_CLASS (item)->has_collection (item, coll);
}

gboolean         biji_item_add_collection      (BijiItem *item, gchar *coll, gboolean on_user_action)
{
  return BIJI_ITEM_GET_CLASS (item)->add_collection (item, coll, on_user_action);
}

gboolean         biji_item_remove_collection   (BijiItem *item, gchar *coll, gchar *urn)
{
  return BIJI_ITEM_GET_CLASS (item)->remove_collection (item, coll, urn);
}
