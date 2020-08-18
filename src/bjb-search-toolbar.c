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
#include "bjb-window-base.h"

struct _BjbSearchToolbar
{
  HdySearchBar    parent_instance;

  GtkEntry        *entry;
  gchar           *needle;
  BjbController   *controller;
  GtkWidget       *window;
};

G_DEFINE_TYPE (BjbSearchToolbar, bjb_search_toolbar, HDY_TYPE_SEARCH_BAR)

static gboolean
on_key_press_event_cb (BjbSearchToolbar *self,
                       GdkEvent         *event)
{
  if (bjb_window_base_get_view_type (self->window) == BJB_WINDOW_BASE_NOTE_VIEW)
    return FALSE;

  return hdy_search_bar_handle_event (HDY_SEARCH_BAR (self), event);
}

static void
on_search_changed_cb (BjbSearchToolbar *self)
{
  bjb_controller_set_needle (BJB_CONTROLLER (self->controller),
                             gtk_entry_get_text (self->entry));
}

static void
action_entry_text_change_callback (GtkEntry         *entry,
                                   BjbSearchToolbar *self)
{
  bjb_controller_set_needle (BJB_CONTROLLER (self->controller),
                             gtk_entry_get_text (entry));
}

void
bjb_search_toolbar_disconnect (BjbSearchToolbar *self)
{
  if (self->key_pressed)
    g_signal_handler_disconnect (self->window, self->key_pressed);
  if (self->text_id)
    g_signal_handler_disconnect (self->entry, self->text_id);

  self->key_pressed = 0;
  self->text_id = 0;
}

void
bjb_search_toolbar_connect (BjbSearchToolbar *self)
{
  /* Connect to set the text */
  if (self->key_pressed == 0)
    self->key_pressed = g_signal_connect(self->window,"key-press-event",
                                         G_CALLBACK(on_key_pressed), self);


  if (self->text_id == 0)
    self->text_id = g_signal_connect (self->entry, "search-changed",
                        G_CALLBACK (action_entry_text_change_callback), self);
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

  g_signal_connect_object (self->window,
                           "key-press-event",
                            G_CALLBACK (on_key_press_event_cb),
                           self,
                           G_CONNECT_SWAPPED);

  g_signal_connect_swapped (self->entry,
                            "search-changed",
                            G_CALLBACK (on_search_changed_cb),
                            self);
}

static void
bjb_search_toolbar_init (BjbSearchToolbar *self)
{
  self->entry = GTK_ENTRY (gtk_search_entry_new ());
  hdy_search_bar_connect_entry (HDY_SEARCH_BAR (self), self->entry);
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

