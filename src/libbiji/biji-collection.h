/*
 * biji-collection.h
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


#ifndef BIJI_COLLECTION_H_
#define BIJI_COLLECTION_H_ 1

#include "biji-item.h"

G_BEGIN_DECLS


#define BIJI_TYPE_COLLECTION             (biji_collection_get_type ())
#define BIJI_COLLECTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_COLLECTION, BijiCollection))
#define BIJI_COLLECTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_COLLECTION, BijiCollectionClass))
#define BIJI_IS_COLLECTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_COLLECTION))
#define BIJI_IS_COLLECTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_COLLECTION))
#define BIJI_COLLECTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_COLLECTION, BijiCollectionClass))

typedef struct BijiCollection_         BijiCollection;
typedef struct BijiCollectionClass_    BijiCollectionClass;
typedef struct BijiCollectionPrivate_  BijiCollectionPrivate;

struct BijiCollection_
{
  BijiItem parent;

  BijiCollectionPrivate *priv;
};

struct BijiCollectionClass_
{
  BijiItemClass parent_class;
};


GType biji_collection_get_type (void);

BijiCollection * biji_collection_new (GObject *book, gchar *urn, gchar *name);

G_END_DECLS

#endif /* BIJI_COLLECTION_H_ */
