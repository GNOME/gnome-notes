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


#ifndef BIJI_ITEM_H_
#define BIJI_ITEM_H_ 1

#include <glib-object.h>

G_BEGIN_DECLS

#define BIJI_TYPE_ITEM             (biji_item_get_type ())
#define BIJI_ITEM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_ITEM, BijiItem))
#define BIJI_ITEM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_ITEM, BijiItemClass))
#define BIJI_IS_ITEM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_ITEM))
#define BIJI_IS_ITEM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_ITEM))
#define BIJI_ITEM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_ITEM, BijiItemClass))

typedef struct BijiItem_         BijiItem;
typedef struct BijiItemClass_    BijiItemClass;
typedef struct BijiItemPrivate_  BijiItemPrivate;

struct BijiItem_
{
  GObject parent;
  /* add your public declarations here */
  BijiItemPrivate *priv;
};

struct BijiItemClass_
{
  GObjectClass parent_class;
};


GType biji_item_get_type (void);

GObject *biji_item_new (void);


G_END_DECLS

#endif /* BIJI_ITEM_H_ */
