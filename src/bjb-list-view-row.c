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
#include "bjb-list-view.h"
#include "bjb-list-view-row.h"
#include "bjb-utils.h"

struct _BjbListViewRow
{
  GtkListBoxRow   parent_instance;

  GtkCssProvider *css_provider;
  BjbListView    *view;
  GtkCheckButton *select_button;
  GtkLabel       *title;
  GtkLabel       *content;
  GtkLabel       *updated_time;

  char           *uuid;
  char           *model_iter;
};

G_DEFINE_TYPE (BjbListViewRow, bjb_list_view_row, GTK_TYPE_LIST_BOX_ROW);

static void
on_toggled_cb (BjbListViewRow *self,
               gpointer        data)
{
  GtkListBox *list_box = bjb_list_view_get_list_box (self->view);
  BjbController *controller = bjb_list_view_get_controller (self->view);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (self->select_button)))
    {
      gtk_list_box_select_row (list_box, GTK_LIST_BOX_ROW (self));
      bjb_controller_select_item (controller, self->model_iter);
    }
  else
    {
      gtk_list_box_unselect_row (list_box, GTK_LIST_BOX_ROW (self));
      bjb_controller_unselect_item (controller, self->model_iter);
    }

  g_signal_emit_by_name (G_OBJECT (gtk_widget_get_parent (GTK_WIDGET (self))), "selected-rows-changed", 0);
}

void
bjb_list_view_row_setup (BjbListViewRow *self,
                         BjbListView    *view,
                         const char     *model_iter)
{
  GtkTreeModel    *model;
  GtkTreeIter      iter;
  char            *uuid;
  char            *title;
  char            *text;
  char            *color;
  gint64           mtime;
  GdkRGBA          rgba;
  gboolean         selected;
  BjbController   *controller;
  GtkListBox      *list_box;
  g_auto (GStrv)   lines        = NULL;
  g_autofree char *one_line     = NULL;
  g_autofree char *preview      = NULL;
  g_autofree char *updated_time = NULL;
  g_autofree char *css_style    = NULL;

  self->view = view;
  list_box = bjb_list_view_get_list_box (view);
  controller = bjb_list_view_get_controller (view);

  model = bjb_controller_get_model (controller);
  if (!gtk_tree_model_get_iter_from_string (model, &iter, model_iter))
    return;
  self->model_iter = g_strdup (model_iter);

  gtk_tree_model_get (model,
                      &iter,
                      BJB_MODEL_COLUMN_UUID,     &uuid,
                      BJB_MODEL_COLUMN_TITLE,    &title,
                      BJB_MODEL_COLUMN_TEXT,     &text,
                      BJB_MODEL_COLUMN_MTIME,    &mtime,
                      BJB_MODEL_COLUMN_COLOR,    &color,
                      BJB_MODEL_COLUMN_SELECTED, &selected,
                      -1);

  updated_time = bjb_utils_get_human_time (mtime);

  if (uuid)
    self->uuid = g_strdup (uuid);
  if (title)
    gtk_label_set_text (self->title, title);
  if (text)
    {
      lines = g_strsplit (text, "\n", -1);
      one_line = g_strjoinv (" ", lines);
      preview = biji_str_clean (one_line);
      gtk_label_set_text (self->content, preview);
    }
  if (updated_time)
    gtk_label_set_text (self->updated_time, updated_time);
  if (color && gdk_rgba_parse (&rgba, color))
    {
      css_style = g_strdup_printf ("row {color: %s; background-color: %s} row:hover {background-color: darker(%s)}",
                                   BJB_UTILS_COLOR_INTENSITY ((&rgba)) < 0.5 ? "white" : "black",
                                   color, color);
      self->css_provider = gtk_css_provider_new ();
      gtk_css_provider_load_from_data (self->css_provider, css_style, -1, 0);
      gtk_style_context_add_provider (gtk_widget_get_style_context (GTK_WIDGET (self)),
                                      GTK_STYLE_PROVIDER (self->css_provider),
                                      GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }

  gtk_widget_set_visible (GTK_WIDGET (self->select_button),
                          bjb_controller_get_selection_mode (controller));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->select_button),
                                selected);
  if (selected)
    gtk_list_box_select_row (list_box, GTK_LIST_BOX_ROW (self));
  else
    gtk_list_box_unselect_row (list_box, GTK_LIST_BOX_ROW (self));

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
  return self->uuid;
}

const char *
bjb_list_view_row_get_model_iter (BjbListViewRow *self)
{
  return self->model_iter;
}

static void
bjb_list_view_row_init (BjbListViewRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bjb_list_view_row_finalize (GObject *object)
{
  BjbListViewRow *self = BJB_LIST_VIEW_ROW (object);

  g_clear_object (&self->css_provider);
  g_free (self->uuid);
  g_free (self->model_iter);

  G_OBJECT_CLASS (bjb_list_view_row_parent_class)->finalize (object);
}

static void
bjb_list_view_row_class_init (BjbListViewRowClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bjb_list_view_row_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/list-view-row.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbListViewRow, select_button);
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

