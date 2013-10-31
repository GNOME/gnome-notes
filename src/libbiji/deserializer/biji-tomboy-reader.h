/*
 * biji-tomboy-reader.h
 * 
 * Copyright 2013 Pierre-Yves Luyten <py@luyten.fr>
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


#ifndef BIJI_TOMBOY_READER_H_
#define BIJI_TOMBOY_READER_H_ 1

#include <glib-object.h>

G_BEGIN_DECLS


#define BIJI_TYPE_TOMBOY_READER             (biji_tomboy_reader_get_type ())
#define BIJI_TOMBOY_READER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_TOMBOY_READER, BijiTomboyReader))
#define BIJI_TOMBOY_READER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_TOMBOY_READER, BijiTomboyReaderClass))
#define BIJI_IS_TOMBOY_READER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_TOMBOY_READER))
#define BIJI_IS_TOMBOY_READER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_TOMBOY_READER))
#define BIJI_TOMBOY_READER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_TOMBOY_READER, BijiTomboyReaderClass))

typedef struct BijiTomboyReader_         BijiTomboyReader;
typedef struct BijiTomboyReaderClass_    BijiTomboyReaderClass;
typedef struct BijiTomboyReaderPrivate_  BijiTomboyReaderPrivate;

struct BijiTomboyReader_
{
  GObject parent;
  BijiTomboyReaderPrivate *priv;
};

struct BijiTomboyReaderClass_
{
  GObjectClass parent_class;
};


GType                biji_tomboy_reader_get_type            (void);


gboolean             biji_tomboy_reader_read              (gchar *source,
                                                           GError **result,
                                                           BijiInfoSet **set,
                                                           gchar       **html,
                                                           GList **notebooks);


G_END_DECLS

#endif /* BIJI_TOMBOY_READER_H_ */
