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
  GtkListBoxRow   parent_instance;

  GtkCheckButton *select_button;
  GtkLabel       *uuid;
  GtkLabel       *title;
  GtkLabel       *content;
  GtkLabel       *updated_time;
};

G_DEFINE_TYPE (BjbListViewRow, bjb_list_view_row, GTK_TYPE_LIST_BOX_ROW);

static void
on_toggled_cb (BjbListViewRow *self,
               gpointer        data)
{
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->select_button)))
    gtk_list_box_select_row (GTK_LIST_BOX (gtk_widget_get_parent (GTK_WIDGET (self))), GTK_LIST_BOX_ROW (self));
  else
    gtk_list_box_unselect_row (GTK_LIST_BOX (gtk_widget_get_parent (GTK_WIDGET (self))), GTK_LIST_BOX_ROW (self));

  g_signal_emit_by_name (G_OBJECT (gtk_widget_get_parent (GTK_WIDGET (self))), "selected-rows-changed", 0);

}

void
bjb_list_view_row_setup (BjbListViewRow *self,
                         const char     *uuid,
                         const char     *title,
                         const char     *content,
                         const char     *updated_time)
{
  g_auto (GStrv)   lines    = NULL;
  g_autofree char *one_line = NULL;
  g_autofree char *preview  = NULL;

  if (uuid)
    gtk_label_set_text (self->uuid, uuid);
  if (title)
    gtk_label_set_text (self->title, title);
  if (content)
    {
      lines = g_strsplit (content, "\n", -1);
      one_line = g_strjoinv (" ", lines);
      preview = biji_str_clean (one_line);
      gtk_label_set_text (self->content, preview);
    }
  if (updated_time)
    gtk_label_set_text (self->updated_time, updated_time);
}

void
bjb_list_view_row_show_select_button (BjbListViewRow *self,
                                      gboolean        show)
{
  gtk_widget_set_visible (GTK_WIDGET (self->select_button), show);
}

const char *
bjb_list_view_row_get_uuid (BjbListViewRow *self)
{
  return gtk_label_get_text (self->uuid);
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

  gtk_widget_class_bind_template_child (widget_class, BjbListViewRow, select_button);
  gtk_widget_class_bind_template_child (widget_class, BjbListViewRow, uuid);
  gtk_widget_class_bind_template_child (widget_class, BjbListViewRow, title);
  gtk_widget_class_bind_template_child (widget_class, BjbListViewRow, content);
  gtk_widget_class_bind_template_child (widget_class, BjbListViewRow, updated_time);
  gtk_widget_class_bind_template_callback (widget_class, on_toggled_cb);
}

BjbListViewRow *
bjb_list_view_row_new (void)
{
  return g_object_new (BJB_TYPE_LIST_VIEW_ROW, NULL);
}

