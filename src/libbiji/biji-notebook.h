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


#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define BIJI_TYPE_NOTEBOOK (biji_notebook_get_type ())

G_DECLARE_FINAL_TYPE (BijiNotebook, biji_notebook, BIJI, NOTEBOOK, GObject)

/* Exiting coll in tracker : provide urn & iso8601 date
 * To create a brand new notebook in tracker rather gobjectize existing one,
 * see biji_create_new_notebook_async */
BijiNotebook * biji_notebook_new (GObject *manager, gchar *urn, gchar *name, gint64 mtime);
gint64         biji_notebook_get_mtime  (BijiNotebook *self);
gpointer       biji_notebook_manager    (BijiNotebook *self);
const char    *biji_notebook_get_title  (BijiNotebook *self);
const char    *biji_notebook_get_uuid   (BijiNotebook *self);
gboolean       biji_notebook_trash      (BijiNotebook *self);


/* Watching for tracker would be best. Right now manually called. */
void             biji_notebook_refresh (BijiNotebook *collection);

G_END_DECLS
