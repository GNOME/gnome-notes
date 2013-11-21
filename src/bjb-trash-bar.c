/* bjb-trash-bar.c
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

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgd/gd.h>

#include "bjb-main-view.h"
#include "bjb-selection-toolbar.h"
#include "bjb-trash-bar.h"
#include "bjb-window-base.h"


enum
{
  PROP_0,
  PROP_BJB_SELECTION,
  PROP_BJB_CONTROLLER,
  NUM_PROPERTIES
};



static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };


struct BjbTrashBarPrivate_
{
  GtkWidget     *bar;
  BjbController *controller;
  GdMainView    *selection;

  GtkWidget     *restore;
  GtkWidget     *delete;
  GtkWidget     *empty_bin;
};




G_DEFINE_TYPE (BjbTrashBar, bjb_trash_bar, GTK_TYPE_BOX)


void
bjb_trash_bar_set_visibility      (BjbTrashBar *self)
{
/*
  BjbTrashBarPrivate *priv;
  GList *items;

  priv = self->priv;
  items = gd_main_view_get_selection (priv->selection);

  if (items != NULL)
    gtk_widget_hide (priv->empty_bin);

  else
    gtk_widget_show (priv->empty_bin);

  g_list_free (items);
*/
}

void
bjb_trash_bar_fade_in             (BjbTrashBar *self)
{
}


void
bjb_trash_bar_fade_out            (BjbTrashBar *self)
{
}

static void
bjb_trash_bar_dispose (GObject *obj)
{
  G_OBJECT_CLASS (bjb_trash_bar_parent_class)->dispose (obj);
}


static void
bjb_trash_bar_get_property (GObject  *object,
                            guint     property_id,
                            GValue   *value,
                            GParamSpec *pspec)
{
  BjbTrashBar *self = BJB_TRASH_BAR (object);

  switch (property_id)
  {
    case PROP_BJB_SELECTION:
      g_value_set_object(value, self->priv->selection);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
bjb_trash_bar_set_property (GObject  *object,
                            guint     property_id,
                            const GValue *value,
                            GParamSpec *pspec)
{
  BjbTrashBar *self = BJB_TRASH_BAR (object);

  switch (property_id)
  {
    case PROP_BJB_SELECTION:
      self->priv->selection = g_value_get_object (value);
      break;

    case PROP_BJB_CONTROLLER:
      self->priv->controller = g_value_get_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
bjb_trash_bar_constructed (GObject *obj)
{
  BjbTrashBar *self;
  BjbTrashBarPrivate *priv;

  G_OBJECT_CLASS (bjb_trash_bar_parent_class)->constructed (obj);

  self = BJB_TRASH_BAR (obj);
  priv = self->priv;

  priv->empty_bin = gtk_label_new (_("Empty"));
  //gtk_container_add (GTK_CONTAINER (self), priv->empty_bin);
  gtk_widget_show_all (GTK_WIDGET (self));
}



static void
bjb_trash_bar_class_init (BjbTrashBarClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bjb_trash_bar_dispose;
  object_class->get_property = bjb_trash_bar_get_property;
  object_class->set_property = bjb_trash_bar_set_property;
  object_class->constructed = bjb_trash_bar_constructed;

  properties[PROP_BJB_SELECTION] = g_param_spec_object ("selection",
                                                        "Selection",
                                                        "SelectionController",
                                                        GD_TYPE_MAIN_VIEW,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS);


  properties[PROP_BJB_CONTROLLER] = g_param_spec_object ("controller",
                                                         "Controller",
                                                         "Window Controller",
                                                         BJB_TYPE_CONTROLLER,
                                                         G_PARAM_READWRITE  |
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  g_type_class_add_private ((gpointer)klass, sizeof (BjbTrashBarPrivate));
}



static void
bjb_trash_bar_init (BjbTrashBar *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BJB_TYPE_TRASH_BAR, BjbTrashBarPrivate);
}


BjbTrashBar *
bjb_trash_bar_new (BjbController *controller,
                   GdMainView *view)
{
  return g_object_new (BJB_TYPE_TRASH_BAR,
                       "orientation", GTK_ORIENTATION_HORIZONTAL,
                       "spacing", 2,
                       "controller", controller,
                       "selection", view,
                        NULL);
}
