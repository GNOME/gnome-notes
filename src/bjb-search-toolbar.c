/* bjb-search-toolbar.c
 * Copyright © 2012, 2013 Pierre-Yves LUYTEN <py@luyten.fr>
 * Copyright 2017 Mohammed Sadiq <sadiq@sadiqpk.org>
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
 * The SearchToolbar displays an entry when text is inserted.
 * 
 * BjbController is updated accordingly and makes note to be
 * displayed or not.
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "bjb-controller.h"
#include "bjb-search-toolbar.h"
#include "bjb-window.h"

struct _BjbSearchToolbar
{
  HdySearchBar    parent_instance;

  GtkEntry        *entry;
  gchar           *needle;
  BjbController   *controller;
  GtkWidget       *window;
};

G_DEFINE_TYPE (BjbSearchToolbar, bjb_search_toolbar, HDY_TYPE_SEARCH_BAR)

GtkEntry *
bjb_search_toolbar_get_entry_widget (BjbSearchToolbar *self)
{
  g_return_val_if_fail (BJB_IS_SEARCH_TOOLBAR (self), NULL);

  return self->entry;
}

static void
on_search_changed_cb (GtkEntry         *entry,
                      BjbSearchToolbar *self)
{
  bjb_controller_set_needle (BJB_CONTROLLER (self->controller),
                             gtk_entry_get_text (entry));
}

void
bjb_search_toolbar_setup (BjbSearchToolbar *self,
                          GtkWidget        *window,
                          BjbController    *controller)
{
  self->controller = controller;
  self->window = window;

  self->needle = bjb_controller_get_needle (self->controller);
  if (self->needle && g_strcmp0 (self->needle, "") != 0)
  {
    gtk_entry_set_text (self->entry, self->needle);
    gtk_editable_set_position (GTK_EDITABLE (self->entry), -1);
  }
}

static void
bjb_search_toolbar_init (BjbSearchToolbar *self)
{
  self->entry = GTK_ENTRY (gtk_search_entry_new ());
  hdy_search_bar_connect_entry (HDY_SEARCH_BAR (self), self->entry);
  g_signal_connect (self->entry, "search-changed",
                    G_CALLBACK (on_search_changed_cb), self);

  hdy_search_bar_connect_entry (HDY_SEARCH_BAR (self), GTK_ENTRY (self->entry));
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->entry));
  gtk_widget_show (GTK_WIDGET (self->entry));
}

static void
bjb_search_toolbar_class_init (BjbSearchToolbarClass *class)
{
}

BjbSearchToolbar *
bjb_search_toolbar_new (void)
{
  return g_object_new (BJB_TYPE_SEARCH_TOOLBAR, NULL);
}

