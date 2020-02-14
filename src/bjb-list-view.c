/*
 * bjb-list-view.c
 * Copyright 2018 Isaque Galdino <igaldino@gmail.com>
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

#include "bjb-list-view.h"
#include "bjb-list-view-row.h"
#include "bjb-utils.h"

struct _BjbListView
{
  GtkScrolledWindow  parent_instance;

  GtkListBox        *list_box;
  BjbController     *controller;

  gulong             display_items_changed;
};

G_DEFINE_TYPE (BjbListView, bjb_list_view, GTK_TYPE_SCROLLED_WINDOW);

static void
bjb_list_view_destroy_row_cb (GtkWidget *widget,
                              gpointer   data)
{
  gtk_widget_destroy (widget);
}

static gboolean
bjb_list_view_create_row_cb (GtkTreeModel *model,
                             GtkTreePath  *path,
                             GtkTreeIter  *iter,
                             gpointer      data)
{
  BjbListView     *self         = NULL;
  char            *uuid;
  char            *title;
  char            *text;
  gint64           mtime;
  BjbListViewRow  *row;
  g_autofree char *updated_time = NULL;

  self = BJB_LIST_VIEW (data);

  gtk_tree_model_get (model,
                      iter,
                      BJB_MODEL_COLUMN_UUID,  &uuid,
                      BJB_MODEL_COLUMN_TITLE, &title,
                      BJB_MODEL_COLUMN_TEXT,  &text,
                      BJB_MODEL_COLUMN_MTIME, &mtime,
                      -1);

  updated_time = bjb_utils_get_human_time (mtime);

  row = bjb_list_view_row_new ();
  bjb_list_view_row_setup (row, uuid, title, text, updated_time);
  gtk_widget_show (GTK_WIDGET (row));
  gtk_container_add (GTK_CONTAINER (self->list_box), GTK_WIDGET (row));

  return FALSE;
}

static void
bjb_list_view_show_select_button_cb (GtkWidget *widget,
                                     gpointer   data)
{
  BjbListViewRow *row  = BJB_LIST_VIEW_ROW (widget);
  gboolean        mode = GPOINTER_TO_INT (data);

  bjb_list_view_row_show_select_button (row, mode);
}

static void
bjb_list_view_on_display_items_changed (BjbController *controller,
                                        gboolean       items_to_show,
                                        gboolean       remaining_items,
                                        BjbListView   *self)
{
  g_return_if_fail (self);

  bjb_list_view_update (self);
}

static void
bjb_list_view_finalize (GObject *object)
{
  BjbListView *self = BJB_LIST_VIEW (object);

  if (self->display_items_changed != 0)
    {
      g_signal_handler_disconnect (self->controller, self->display_items_changed);
    }

  G_OBJECT_CLASS (bjb_list_view_parent_class)->finalize (object);
}

void
bjb_list_view_setup (BjbListView   *self,
                     BjbController *controller)
{
  g_return_if_fail (controller);

  if (self->display_items_changed != 0)
    g_signal_handler_disconnect (self->controller, self->display_items_changed);

  self->controller = controller;

  self->display_items_changed = g_signal_connect (self->controller,
                                                  "display-items-changed",
                                                  G_CALLBACK (bjb_list_view_on_display_items_changed),
                                                  self);

  bjb_list_view_update (self);
}

void
bjb_list_view_update (BjbListView *self)
{
  gtk_container_foreach (GTK_CONTAINER (self->list_box),
                         bjb_list_view_destroy_row_cb,
                         NULL);

  gtk_tree_model_foreach (bjb_controller_get_model (self->controller),
                          bjb_list_view_create_row_cb,
                          self);
}

GtkListBox *
bjb_list_view_get_list_box (BjbListView *self)
{
  return self->list_box;
}

static void
bjb_list_view_init (BjbListView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bjb_list_view_class_init (BjbListViewClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bjb_list_view_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/list-view.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbListView, list_box);
}

BjbListView *
bjb_list_view_new (void)
{
  return g_object_new (BJB_TYPE_LIST_VIEW, NULL);
}

void
bjb_list_view_set_selection_mode (BjbListView *self,
                                  gboolean     mode)
{
  gtk_list_box_unselect_all (self->list_box);

  gtk_container_foreach (GTK_CONTAINER (self->list_box),
                         bjb_list_view_show_select_button_cb,
                         GINT_TO_POINTER (mode));
}
