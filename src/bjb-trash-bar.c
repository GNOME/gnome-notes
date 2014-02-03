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
  PROP_BJB_MAIN_VIEW,
  PROP_BJB_CONTROLLER,
  NUM_PROPERTIES
};



static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };


struct BjbTrashBarPrivate_
{
  GtkWidget     *bar;
  BjbController *controller;
  BjbMainView   *view;
  GdMainView    *selection;

  GtkWidget     *normal_box;
  GtkWidget     *empty_bin;

  GtkWidget     *selection_box;
  GtkWidget     *restore;
  GtkWidget     *delete;
};




G_DEFINE_TYPE (BjbTrashBar, bjb_trash_bar, GTK_TYPE_BOX)



static void
on_empty_clicked_callback        (BjbTrashBar *self)
{

  biji_manager_empty_bin (
    bjb_window_base_get_manager (
      gtk_widget_get_toplevel (GTK_WIDGET (self))));
}


static void
on_restore_clicked_callback      (BjbTrashBar *self)
{
  GList *selection, *l;

  selection = bjb_main_view_get_selected_items (self->priv->view);
  for (l=selection; l!=NULL; l=l->next)
    biji_item_restore (BIJI_ITEM (l->data));

  g_list_free (selection);
}


static void
on_delete_clicked_callback        (BjbTrashBar *self)
{
  GList *selection, *l;

  selection = bjb_main_view_get_selected_items (self->priv->view);
  for (l=selection; l!=NULL; l=l->next)
    biji_item_delete (BIJI_ITEM (l->data));

  g_list_free (selection);
}


void
bjb_trash_bar_set_visibility      (BjbTrashBar *self)
{
  BjbTrashBarPrivate *priv;
  GList *items;

  priv = self->priv;


  gtk_widget_hide (priv->normal_box);
  gtk_widget_hide (priv->selection_box);
  items = gd_main_view_get_selection (priv->selection);

  if (items != NULL)
    gtk_widget_show (priv->selection_box);

  else
    gtk_widget_show (priv->normal_box);

  g_list_free (items);
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

    case PROP_BJB_MAIN_VIEW:
      self->priv->view = g_value_get_object (value);
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
  GtkWidget *super_box, *separator;
  GtkStyleContext *context;

  G_OBJECT_CLASS (bjb_trash_bar_parent_class)->constructed (obj);

  self = BJB_TRASH_BAR (obj);
  priv = self->priv;

  super_box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (self), super_box);

  /* No selection : just offer to empty bin */
  priv->normal_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (priv->normal_box, TRUE);
  priv->empty_bin = gtk_button_new_with_label(_("Empty"));
  gtk_widget_set_halign (priv->empty_bin, GTK_ALIGN_CENTER);
  gtk_widget_set_size_request (priv->empty_bin, 90, -1);
  context = gtk_widget_get_style_context (priv->empty_bin);
  gtk_style_context_add_class (context, "destructive-action");
  gtk_box_pack_start (GTK_BOX (priv->normal_box),
                      priv->empty_bin,
                      TRUE,
                      FALSE,
                      0);
  gtk_container_add (GTK_CONTAINER (super_box), priv->normal_box);
  g_signal_connect_swapped (priv->empty_bin,
                            "clicked",
                            G_CALLBACK (on_empty_clicked_callback),
                            self);


  /* Selection : delete or restore selected items */
  priv->selection_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  gtk_widget_set_hexpand (priv->selection_box, TRUE);
  priv->restore = gtk_button_new_with_label (_("Restore"));
  gtk_widget_set_halign (priv->restore, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (priv->selection_box),
                      priv->restore,
                      FALSE,
                      FALSE,
                      4);
  g_signal_connect_swapped (priv->restore,
                            "clicked",
                            G_CALLBACK (on_restore_clicked_callback),
                            self);

  separator = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (separator), GTK_SHADOW_NONE);
  gtk_widget_set_hexpand (priv->selection_box, TRUE);
  gtk_box_pack_start (GTK_BOX (priv->selection_box),
                      separator,
                      TRUE,
                      FALSE,
                      4);

  priv->delete = gtk_button_new_with_label (_("Permanently Delete"));
  gtk_widget_set_halign (priv->delete, GTK_ALIGN_END);
  context = gtk_widget_get_style_context (priv->delete);
  gtk_style_context_add_class (context, "destructive-action");
  gtk_box_pack_start (GTK_BOX (priv->selection_box),
                      priv->delete,
                      FALSE,
                      FALSE,
                      4);
  gtk_container_add (GTK_CONTAINER (super_box), priv->selection_box);
  g_signal_connect_swapped (priv->delete,
                            "clicked",
                            G_CALLBACK (on_delete_clicked_callback),
                            self);


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


  properties[PROP_BJB_MAIN_VIEW] = g_param_spec_object ("view",
                                                        "view",
                                                        "View",
                                                        BJB_TYPE_MAIN_VIEW,
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
                   BjbMainView   *parent,
                   GdMainView *view)
{
  return g_object_new (BJB_TYPE_TRASH_BAR,
                       "orientation", GTK_ORIENTATION_HORIZONTAL,
                       "spacing", 2,
                       "controller", controller,
                       "view", parent,
                       "selection", view,
                        NULL);
}
