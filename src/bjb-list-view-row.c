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
#include "bjb-list-view-row.h"
#include "bjb-utils.h"

struct _BjbListViewRow
{
  GtkListBoxRow   parent_instance;

  GtkCssProvider *css_provider;
  GtkLabel       *title;
  GtkLabel       *content;
  GtkLabel       *updated_time;

  BjbNote        *note;
};

G_DEFINE_TYPE (BjbListViewRow, bjb_list_view_row, GTK_TYPE_LIST_BOX_ROW);

static void
note_mtime_changed_cb (BjbListViewRow *self)
{
  g_autofree char *time_label = NULL;

  g_assert (BJB_IS_LIST_VIEW_ROW (self));

  time_label = bjb_utils_get_human_time (bjb_item_get_mtime (BJB_ITEM (self->note)));
  gtk_label_set_text (self->updated_time, time_label);
}

static void
note_color_changed_cb (BjbListViewRow *self)
{
  g_autofree char *css_style = NULL;
  g_autofree char *css_name = NULL;
  g_autofree char *color = NULL;
  GQuark color_id;
  GdkRGBA rgba;

  g_assert (BJB_IS_LIST_VIEW_ROW (self));

  if (!bjb_item_get_rgba (BJB_ITEM (self->note), &rgba))
    return;

  color = gdk_rgba_to_string (&rgba);
  color_id = g_quark_from_string (color);
  css_name = g_strdup_printf ("color-%u", color_id);
  css_style = g_strdup_printf (".%s {color: %s; background-color: %s;} .%s:hover {background-color: darker(%s);}",
                               css_name,
                               BJB_UTILS_COLOR_INTENSITY ((&rgba)) < 0.5 ? "white" : "black",
                               color, css_name, color);
  gtk_css_provider_load_from_string (self->css_provider, css_style);
  gtk_style_context_add_provider_for_display (gdk_display_get_default (),
                                              GTK_STYLE_PROVIDER (self->css_provider),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  gtk_widget_add_css_class (GTK_WIDGET (self), css_name);
}

static void
note_content_changed_cb (BjbListViewRow *self)
{
  g_autofree char *one_line = NULL;
  g_autofree char *preview = NULL;
  g_autofree char *content = NULL;
  g_auto(GStrv) lines = NULL;

  g_assert (BJB_IS_LIST_VIEW_ROW (self));

  content = bjb_note_get_raw_content (BJB_NOTE (self->note));

  if (!content)
    content = bjb_note_get_text_content (BJB_NOTE (self->note));

  if (!content)
    return;

  lines = g_strsplit (content, "\n", -1);
  one_line = g_strjoinv (" ", lines);
  preview = biji_str_clean (one_line);
  gtk_label_set_text (self->content, preview);
}

static void
bjb_list_view_row_init (BjbListViewRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->css_provider = gtk_css_provider_new ();
}

static void
bjb_list_view_row_finalize (GObject *object)
{
  BjbListViewRow *self = BJB_LIST_VIEW_ROW (object);

  g_clear_object (&self->note);
  g_clear_object (&self->css_provider);

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

GtkWidget *
bjb_list_view_row_new_with_note (BjbNote *note)
{
  BjbListViewRow *self;

  g_return_val_if_fail (BJB_IS_NOTE (note), NULL);

  self = g_object_new (BJB_TYPE_LIST_VIEW_ROW, NULL);
  self->note = g_object_ref (note);

  g_object_bind_property (note, "title",
                          self->title, "label",
                          G_BINDING_SYNC_CREATE);
  g_signal_connect_object (note, "notify::mtime",
                           G_CALLBACK (note_mtime_changed_cb),
                           self, G_CONNECT_SWAPPED);

  g_signal_connect_object (note, "notify::color",
                           G_CALLBACK (note_color_changed_cb),
                           self, G_CONNECT_SWAPPED);
  g_signal_connect_object (note, "notify::title",
                           G_CALLBACK (note_content_changed_cb),
                           self, G_CONNECT_SWAPPED);

  note_color_changed_cb (self);
  note_mtime_changed_cb (self);
  note_content_changed_cb (self);

  return GTK_WIDGET (self);
}

BjbItem *
bjb_list_view_row_get_note (BjbListViewRow *self)
{
  g_return_val_if_fail (BJB_IS_LIST_VIEW_ROW (self), NULL);

  return BJB_ITEM (self->note);
}
