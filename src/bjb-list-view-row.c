/*
 * bjb-list-view-row.c
 * Copyright 2020 Isaque Galdino <igaldino@gmail.org>
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

#include <biji-string.h>
#include "bjb-list-view-row.h"

struct _BjbListViewRow
{
  GtkBox    parent_instance;

  GtkLabel *title;
  GtkLabel *content;
  GtkLabel *updated_time;
};

G_DEFINE_TYPE (BjbListViewRow, bjb_list_view_row, GTK_TYPE_BOX);

void
bjb_list_view_row_setup (BjbListViewRow *self,
                         const char     *title,
                         const char     *content,
                         const char     *updated_time)
{
  g_auto (GStrv)   lines   = NULL;
  g_autofree char *preview = NULL;

  if (title)
    {
      gtk_label_set_text (self->title, title);
    }
  if (content)
    {
      lines = g_strsplit(content, "\n", 4);
      preview = bjb_strjoinv ("\n", lines, 3);
      gtk_label_set_text (self->content, preview);
    }
  if (updated_time)
    {
      gtk_label_set_text (self->updated_time, updated_time);
    }
}

static void
bjb_list_view_row_init (BjbListViewRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bjb_list_view_row_class_init (BjbListViewRowClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/list-view-row.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbListViewRow, title);
  gtk_widget_class_bind_template_child (widget_class, BjbListViewRow, content);
  gtk_widget_class_bind_template_child (widget_class, BjbListViewRow, updated_time);
}


BjbListViewRow *
bjb_list_view_row_new (void)
{
  return g_object_new (BJB_TYPE_LIST_VIEW_ROW, NULL);
}

