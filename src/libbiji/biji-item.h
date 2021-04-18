/*
 * biji-item.h
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


#pragma once

#include <cairo-gobject.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BIJI_TYPE_ITEM (biji_item_get_type ())

G_DECLARE_DERIVABLE_TYPE (BijiItem, biji_item, BIJI, ITEM, GObject)

/* Icon */
#define BIJI_ICON_WIDTH 200
#define BIJI_ICON_HEIGHT 200
#define BIJI_ICON_FONT "Purusa 10"

/* a cute baby icon without txt. squared. */
#define BIJI_EMBLEM_WIDTH BIJI_ICON_WIDTH / 6
#define BIJI_EMBLEM_HEIGHT BIJI_EMBLEM_WIDTH

struct _BijiItemClass
{
  GObjectClass parent_class;

  const gchar * (*get_title)            (BijiItem *item);
  const gchar * (*get_uuid)             (BijiItem *item);
  cairo_surface_t * (*get_icon)         (BijiItem *item,
                                         gint scale);
  cairo_surface_t * (*get_emblem)       (BijiItem *item,
                                         gint scale);
  cairo_surface_t * (*get_pristine)     (BijiItem *item,
                                         gint scale);

  /* Just return some provider information */
  const gchar * (*get_place)            (BijiItem *item);

  gint64        (*get_mtime)            (BijiItem *item);
  gboolean      (*has_color)            (BijiItem *item);

  gboolean      (*trash)                (BijiItem *item);
  gboolean      (*restore)              (BijiItem *item, gchar **old_uuid);
  gboolean      (*delete)               (BijiItem *item);

  gboolean      (*is_collectable)       (BijiItem *item);
  gboolean      (*has_notebook)         (BijiItem *item, gchar *coll);
  gboolean      (*add_notebook)         (BijiItem *item, BijiItem *coll, gchar *title);
  gboolean      (*remove_notebook)      (BijiItem *item, BijiItem *coll);
};

/* Do not create a generic items, it's rather an iface
 * but i just need common stuff */


/*  - note uuid is a location (as in GFile)
 *  - notebook uuid is urn                */

const gchar *    biji_item_get_title           (BijiItem *item);


const gchar *    biji_item_get_uuid            (BijiItem *item);


gpointer         biji_item_get_manager         (BijiItem *item);


gboolean         biji_item_has_color           (BijiItem *item);


cairo_surface_t * biji_item_get_icon           (BijiItem *item,
                                                gint scale);


cairo_surface_t * biji_item_get_emblem         (BijiItem *item,
                                                gint scale);


cairo_surface_t * biji_item_get_pristine       (BijiItem *item,
                                                gint scale);


const gchar *    biji_item_get_place           (BijiItem *item);


gint64           biji_item_get_mtime           (BijiItem *item);


gboolean         biji_item_trash               (BijiItem *item);


gboolean         biji_item_restore             (BijiItem *item);


gboolean         biji_item_delete              (BijiItem *item);


gboolean         biji_item_is_collectable      (BijiItem *item);


gboolean         biji_item_has_notebook        (BijiItem *item,
                                                gchar *collection);


/* Add Collection:
 * either provide an existing notebook object
 * or a title, in which case it's considered not on user action
 * and no notify happens */

gboolean         biji_item_add_notebook  (BijiItem *item, BijiItem *collection, gchar *title);


/* Remove Collection:
 * always on user action. */

gboolean         biji_item_remove_notebook   (BijiItem *item, BijiItem *collection);

G_END_DECLS
