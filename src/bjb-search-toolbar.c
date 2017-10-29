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
#include <libgd/gd-entry-focus-hack.h>
#include <libgd/gd-tagged-entry.h>

#include "bjb-controller.h"
#include "bjb-main-toolbar.h"
#include "bjb-main-view.h"
#include "bjb-search-toolbar.h"
#include "bjb-window-base.h"

enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_CONTROLLER,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

struct _BjbSearchToolbar
{
  GtkSearchBar       parent_instance;

  GtkWidget         *entry;
  gchar             *needle;
  GtkEntryBuffer    *entry_buf;
  BjbController     *controller;

  /* Signals */
  gulong            key_pressed;
  gulong            deleted;
  gulong            inserted;


  GtkWidget         *window;
};

G_DEFINE_TYPE (BjbSearchToolbar, bjb_search_toolbar, GTK_TYPE_SEARCH_BAR)

static gboolean
on_key_pressed (GtkWidget *widget,GdkEvent  *event,gpointer user_data)
{
  GtkSearchBar *search_bar = GTK_SEARCH_BAR (user_data);

  return gtk_search_bar_handle_event (search_bar, event);
}


static void
bjb_search_toolbar_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BjbSearchToolbar *self = BJB_SEARCH_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_WINDOW:
      g_value_set_object (value, self->window);
      break;
    case PROP_CONTROLLER:
      g_value_set_object(value, self->controller);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
bjb_search_toolbar_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BjbSearchToolbar *self = BJB_SEARCH_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_WINDOW:
      self->window = g_value_get_object (value);
      break;
    case PROP_CONTROLLER:
      self->controller = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
action_search_entry (GtkEntry *entry, BjbController *controller)
{
  bjb_controller_set_needle (controller, gtk_entry_get_text (entry));
}



static void
action_entry_insert_callback (GtkEntryBuffer *buffer,
                              guint position,
                              gchar *chars,
                              guint n_chars,
                              BjbSearchToolbar *self)
{
  action_search_entry (GTK_ENTRY (self->entry),
                       self->controller);
}




static void
action_entry_delete_callback (GtkEntryBuffer *buffer,
                              guint position,
                              guint n_chars,
                              BjbSearchToolbar *self)
{
  action_search_entry (GTK_ENTRY (self->entry),
                       self->controller);
}


void
bjb_search_toolbar_disconnect (BjbSearchToolbar *self)
{
  if (self->key_pressed)
    g_signal_handler_disconnect (self->window, self->key_pressed);
  if (self->inserted)
    g_signal_handler_disconnect (self->entry_buf, self->inserted);
  if (self->deleted)
    g_signal_handler_disconnect (self->entry_buf, self->deleted);

  self->key_pressed = 0;
  self->inserted = 0;
  self->deleted = 0;
}

void
bjb_search_toolbar_connect (BjbSearchToolbar *self)
{
  /* Connect to set the text */
  if (self->key_pressed == 0)
    self->key_pressed = g_signal_connect(self->window,"key-press-event",
                                         G_CALLBACK(on_key_pressed), self);


  if (self->inserted == 0)
    self->inserted = g_signal_connect (self->entry_buf, "inserted-text",
                        G_CALLBACK (action_entry_insert_callback), self);

  if (self->deleted == 0)
    self->deleted = g_signal_connect (self->entry_buf, "deleted-text",
                        G_CALLBACK (action_entry_delete_callback), self);
}

static void
bjb_search_toolbar_constructed (GObject *obj)
{
  BjbSearchToolbar        *self = BJB_SEARCH_TOOLBAR(obj);

  G_OBJECT_CLASS (bjb_search_toolbar_parent_class)->constructed (obj);

  /* Get the needle from controller */
  self->needle = bjb_controller_get_needle (self->controller);
  self->entry_buf = gtk_entry_get_buffer (GTK_ENTRY (self->entry));

  if (self->needle && g_strcmp0 (self->needle, "") != 0)
  {
    gtk_entry_set_text (GTK_ENTRY (self->entry), self->needle);
    gtk_editable_set_position (GTK_EDITABLE (self->entry), -1);
  }
}


static void
bjb_search_toolbar_init (BjbSearchToolbar *self)
{
  self->entry = gtk_search_entry_new ();
  gtk_search_bar_connect_entry (GTK_SEARCH_BAR (self), GTK_ENTRY (self->entry));
  g_object_set (self->entry, "width_request", 500, NULL);
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (self->entry));
  gtk_widget_show (GTK_WIDGET (self->entry));
}


static void
bjb_search_toolbar_class_init (BjbSearchToolbarClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = bjb_search_toolbar_get_property ;
  object_class->set_property = bjb_search_toolbar_set_property ;
  object_class->constructed = bjb_search_toolbar_constructed ;

  properties[PROP_WINDOW] = g_param_spec_object ("window",
                                                 "Window",
                                                 "Window",
                                                 GTK_TYPE_WIDGET,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT |
                                                 G_PARAM_STATIC_STRINGS);

  properties[PROP_CONTROLLER] = g_param_spec_object ("controller",
                                                     "Controller",
                                                     "Controller",
                                                     BJB_TYPE_CONTROLLER,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}


BjbSearchToolbar *
bjb_search_toolbar_new (GtkWidget     *window,
                        BjbController *controller)
{
  return g_object_new (BJB_TYPE_SEARCH_TOOLBAR,
                       "window",      window,
                       "controller",  controller,
                       NULL);
}

