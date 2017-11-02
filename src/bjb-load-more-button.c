/*
 * Bjb - access, organize and share your bjb on GNOME
 * Copyright © 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/* Chained based on Photos, Documents */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gprintf.h>


#include "bjb-load-more-button.h"

struct _BjbLoadMoreButton
{
  GtkButton parent_instance;

  GtkWidget *revealer;
  GtkWidget *label;
  GtkWidget *spinner;
  gboolean   block;

  BjbController *controller;
};


enum
{
  PROP_0,
  PROP_BJB_CONTROLLER,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };


G_DEFINE_TYPE (BjbLoadMoreButton, bjb_load_more_button, GTK_TYPE_BUTTON)


static void
on_displayed_items_changed (BjbController     *controller,
                            gboolean           some_is_shown,
                            gboolean           remaining,
                            BjbLoadMoreButton *self)
{
  gtk_spinner_stop (GTK_SPINNER (self->spinner));

  if (some_is_shown)
  {
    gtk_widget_hide (self->spinner);
    gtk_label_set_label (GTK_LABEL (self->label), _("Load More"));

    if (remaining && (self->block == FALSE))
      gtk_revealer_set_reveal_child (GTK_REVEALER (self->revealer), TRUE);
    else
      gtk_revealer_set_reveal_child (GTK_REVEALER (self->revealer), FALSE);
  }
  else
  {
    gtk_revealer_set_reveal_child (GTK_REVEALER (self->revealer), FALSE);
  }
}


static void
bjb_load_more_button_clicked (GtkButton *button)
{
  BjbLoadMoreButton *self;

  self = BJB_LOAD_MORE_BUTTON (button);

  gtk_label_set_label (GTK_LABEL (self->label), _("Loading…"));
  gtk_widget_show (self->spinner);
  gtk_spinner_start (GTK_SPINNER (self->spinner));

  bjb_controller_show_more (self->controller);
}


static void
bjb_load_more_button_constructed (GObject *object)
{
  BjbLoadMoreButton *self;

  self = BJB_LOAD_MORE_BUTTON (object);

  g_signal_connect (self->controller, "display-items-changed",
                    G_CALLBACK (on_displayed_items_changed), self);

  G_OBJECT_CLASS (bjb_load_more_button_parent_class)->constructed (object);
  gtk_widget_show_all (GTK_WIDGET (self));
}


static void
bjb_load_more_button_finalize (GObject *object)
{
  BjbLoadMoreButton *self;

  self = BJB_LOAD_MORE_BUTTON (object);

  if (self->controller && BJB_IS_CONTROLLER (self->controller))
    g_signal_handlers_disconnect_by_func
      (self->controller, on_displayed_items_changed, self);
}


static void
bjb_load_more_button_dispose (GObject *object)
{
  G_OBJECT_CLASS (bjb_load_more_button_parent_class)->dispose (object);
}


static void
bjb_load_more_button_init (BjbLoadMoreButton *self)
{
  GtkStyleContext *context;
  GtkWidget *child;

  context = gtk_widget_get_style_context (GTK_WIDGET (self));
  gtk_style_context_add_class (context, "documents-load-more");

  child = gtk_grid_new ();
  gtk_widget_set_halign (child, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand (child, TRUE);
  gtk_widget_set_visible (child, TRUE);
  gtk_grid_set_column_spacing (GTK_GRID (child), 10);
  gtk_container_add (GTK_CONTAINER (self), child);

  self->spinner = gtk_spinner_new ();
  gtk_widget_set_halign (self->spinner, GTK_ALIGN_CENTER);
  gtk_widget_set_no_show_all (self->spinner, TRUE);
  gtk_widget_set_size_request (self->spinner, 16, 16);
  gtk_container_add (GTK_CONTAINER (child), self->spinner);

  self->label = gtk_label_new (_("Load More"));
  gtk_widget_set_visible (self->label, TRUE);
  gtk_container_add (GTK_CONTAINER (child), self->label);

  self->revealer = gtk_revealer_new ();
  gtk_container_add (GTK_CONTAINER (self->revealer), GTK_WIDGET (self));
}


static void
bjb_load_more_button_get_property (GObject      *object,
                                   guint        prop_id,
                                   GValue       *value,
                                   GParamSpec   *pspec)
{
  BjbLoadMoreButton *self = BJB_LOAD_MORE_BUTTON (object);

  switch (prop_id)
  {
    case PROP_BJB_CONTROLLER:
      g_value_set_object (value, self->controller);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bjb_load_more_button_set_property (GObject        *object,
                                   guint          prop_id,
                                   const GValue   *value,
                                   GParamSpec     *pspec)
{
  BjbLoadMoreButton *self = BJB_LOAD_MORE_BUTTON (object);

  switch (prop_id)
  {
    case PROP_BJB_CONTROLLER:
      self->controller = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
bjb_load_more_button_class_init (BjbLoadMoreButtonClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (class);

  object_class->constructed = bjb_load_more_button_constructed;
  object_class->dispose = bjb_load_more_button_dispose;
  object_class->finalize = bjb_load_more_button_finalize;
  object_class->get_property = bjb_load_more_button_get_property;
  object_class->set_property = bjb_load_more_button_set_property;
  button_class->clicked = bjb_load_more_button_clicked;

  properties[PROP_BJB_CONTROLLER] = g_param_spec_object ("controller",
                                                         "Controller",
                                                         "BjbController",
                                                         BJB_TYPE_CONTROLLER,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}


GtkWidget *
bjb_load_more_button_new (BjbController *controller)
{
  BjbLoadMoreButton *button;

  button = g_object_new (BJB_TYPE_LOAD_MORE_BUTTON, "controller", controller, NULL);
  return GTK_WIDGET (button);
}


void
bjb_load_more_button_set_block (BjbLoadMoreButton *self, gboolean block)
{
  BjbController *controller;

  if (self->block == block)
    return;

  self->block = block;
  controller = self->controller;
  on_displayed_items_changed (controller,
                              bjb_controller_shows_item (controller),
                              bjb_controller_get_remaining_items (controller),
                              self);
}


GtkWidget *
bjb_load_more_button_get_revealer (BjbLoadMoreButton *self)
{
  return self->revealer;
}
