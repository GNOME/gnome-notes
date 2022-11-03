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
};

G_DEFINE_TYPE (BjbListView, bjb_list_view, GTK_TYPE_SCROLLED_WINDOW);

void
bjb_list_view_setup (BjbListView   *self,
                     BjbController *controller)
{
  GListModel *notes;

  g_return_if_fail (controller);

  if (self->controller)
    return;

  self->controller = controller;

  notes = bjb_controller_get_notes (controller);
  gtk_list_box_bind_model (self->list_box, notes,
                           (GtkListBoxCreateWidgetFunc)bjb_list_view_row_new_with_note,
                           self, NULL);
}

GtkListBox *
bjb_list_view_get_list_box (BjbListView *self)
{
  return self->list_box;
}

BjbController *
bjb_list_view_get_controller (BjbListView *self)
{
  return self->controller;
}

static void
bjb_list_view_init (BjbListView *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bjb_list_view_class_init (BjbListViewClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/list-view.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbListView, list_box);
}

BjbListView *
bjb_list_view_new (void)
{
  return g_object_new (BJB_TYPE_LIST_VIEW, NULL);
}
