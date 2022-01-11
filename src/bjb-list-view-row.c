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
#include "bjb-application.h"
#include "bjb-list-view.h"
#include "bjb-list-view-row.h"
#include "bjb-utils.h"

struct _BjbListViewRow
{
  GtkListBoxRow   parent_instance;

  GtkCssProvider *css_provider;
  BjbListView    *view;
  GtkLabel       *title;
  GtkLabel       *content;
  GtkLabel       *updated_time;

  char           *uuid;
  char           *model_iter;

  gulong          display_note_amended;
};

G_DEFINE_TYPE (BjbListViewRow, bjb_list_view_row, GTK_TYPE_LIST_BOX_ROW);

static void
on_manager_changed (BijiManager            *manager,
                    BijiItemsGroup          group,
                    BijiManagerChangeFlag   flag,
                    gpointer               *biji_item,
                    BjbListViewRow         *self)
{
  BijiItem *item = BIJI_ITEM (biji_item);
  BijiNoteObj *note_obj = BIJI_NOTE_OBJ (item);

  /* Note title/content amended. */
  if (flag == BIJI_MANAGER_NOTE_AMENDED)
    {
      if (g_strcmp0 (self->uuid, biji_item_get_uuid (item)) == 0)
        {
          if (biji_note_obj_get_title (note_obj) != NULL &&
              g_strcmp0 (gtk_label_get_text (self->title),
                         biji_note_obj_get_title (note_obj)) != 0)
            {
              gtk_label_set_text (self->title, biji_note_obj_get_title (note_obj));
            }
          if (biji_note_obj_get_raw_text (note_obj) != NULL &&
              g_strcmp0 (gtk_label_get_text (self->content),
                         biji_note_obj_get_raw_text (note_obj)) != 0)
            {
              g_auto(GStrv)   lines         = NULL;
              g_autofree char *one_line     = NULL;
              g_autofree char *preview      = NULL;

              lines = g_strsplit (biji_note_obj_get_raw_text (note_obj), "\n", -1);
              one_line = g_strjoinv (" ", lines);
              preview = biji_str_clean (one_line);
              gtk_label_set_text (self->content, preview);
            }
        }
    }
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
  BijiManager     *manager;
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

  if (selected)
    gtk_list_box_select_row (list_box, GTK_LIST_BOX_ROW (self));
  else
    gtk_list_box_unselect_row (list_box, GTK_LIST_BOX_ROW (self));

  if (self->display_note_amended != 0)
    {
      g_signal_handler_disconnect (controller, self->display_note_amended);
    }

  manager = bijiben_get_manager (BJB_APPLICATION (g_application_get_default ()));
  self->display_note_amended = g_signal_connect (manager, "changed",
                                                 G_CALLBACK (on_manager_changed), self);
}

const char *
bjb_list_view_row_get_uuid (BjbListViewRow *self)
{
  return self->uuid;
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

  if (self->display_note_amended != 0)
    {
      BijiManager *manager;

      manager = bijiben_get_manager (BJB_APPLICATION (g_application_get_default ()));
      g_signal_handler_disconnect (manager, self->display_note_amended);
    }

  G_OBJECT_CLASS (bjb_list_view_row_parent_class)->finalize (object);
}

static void
bjb_list_view_row_class_init (BjbListViewRowClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bjb_list_view_row_finalize;

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

