/*
 * biji-notebook.h
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


#ifndef BIJI_NOTEBOOK_H_
#define BIJI_NOTEBOOK_H_ 1

#include "biji-item.h"

G_BEGIN_DECLS


#define BIJI_TYPE_NOTEBOOK             (biji_notebook_get_type ())
#define BIJI_NOTEBOOK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_NOTEBOOK, BijiNotebook))
#define BIJI_NOTEBOOK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_NOTEBOOK, BijiNotebookClass))
#define BIJI_IS_NOTEBOOK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_NOTEBOOK))
#define BIJI_IS_NOTEBOOK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_NOTEBOOK))
#define BIJI_NOTEBOOK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_NOTEBOOK, BijiNotebookClass))

typedef struct BijiNotebook_         BijiNotebook;
typedef struct BijiNotebookClass_    BijiNotebookClass;
typedef struct BijiNotebookPrivate_  BijiNotebookPrivate;

struct BijiNotebook_
{
  BijiItem parent;

  BijiNotebookPrivate *priv;
};

struct BijiNotebookClass_
{
  BijiItemClass parent_class;
};


GType biji_notebook_get_type (void);

/* Exiting coll in tracker : provide urn & iso8601 date
 * To create a brand new notebook in tracker rather gobjectize existing one,
 * see biji_create_new_notebook_async */
BijiNotebook * biji_notebook_new (GObject *manager, gchar *urn, gchar *name, gint64 mtime);


/* Watching for tracker would be best. Right now manually called. */
void             biji_notebook_refresh (BijiNotebook *collection);

G_END_DECLS

#endif /* BIJI_NOTEBOOK_H_ */
