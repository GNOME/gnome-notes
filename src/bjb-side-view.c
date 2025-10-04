/* bjb-side-view.c
 *
 * Copyright 2012, 2013 Pierre-Yves Luyten <py@luyten.fr>
 * Copyright 2020 Jonathan Kang <jonathankang@gnome.org>
 * Copyright 2021, 2025 Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s):
 *   Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "bjb-side-view"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "items/bjb-plain-note.h"
#include "providers/bjb-provider.h"
#include "bjb-note-list.h"
#include "bjb-manager.h"
#include "bjb-side-view.h"

struct _BjbSideView
{
  AdwNavigationPage    parent_instance;

  GtkWidget  *note_list;
  GtkWidget  *providers_popover;
  GtkWidget  *providers_list_box;

  BjbItem    *selected_note;
};


enum {
  SELECTED_NOTE_CHANGED,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

G_DEFINE_TYPE (BjbSideView, bjb_side_view, ADW_TYPE_NAVIGATION_PAGE)

static GtkWidget *
provider_row_new (gpointer item,
                  gpointer user_data)
{
  GtkWidget *grid, *child;
  g_autoptr(GIcon) icon = NULL;

  grid = gtk_grid_new ();
  g_object_set_data (G_OBJECT (grid), "provider", item);
  gtk_widget_set_margin_start (grid, 6);
  gtk_widget_set_margin_end (grid, 6);
  gtk_widget_set_margin_top (grid, 6);
  gtk_widget_set_margin_bottom (grid, 6);
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);

  icon = bjb_provider_get_icon (item, NULL);
  if (icon)
    {
      child = gtk_image_new_from_gicon (icon);
      gtk_grid_attach (GTK_GRID (grid), child, 0, 0, 1, 2);
    }

  child = gtk_label_new (bjb_provider_get_name (item));
  gtk_widget_set_halign (child, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), child, 1, 0, 1, 1);

  child = gtk_label_new (bjb_provider_get_location_name (item));
  gtk_widget_set_halign (child, GTK_ALIGN_START);
  gtk_widget_add_css_class (child, "dim-label");

  gtk_grid_attach (GTK_GRID (grid), child, 1, 1, 1, 1);

  return grid;
}

static void
bjb_side_view_note_changed_cb (BjbSideView *self,
                               BjbNoteList *note_list)
{
  BjbItem *note;

  g_assert (BJB_IS_SIDE_VIEW (self));
  g_assert (BJB_IS_NOTE_LIST (note_list));

  note = bjb_note_list_get_selected_note (note_list);

  g_set_object (&self->selected_note, note);
  g_signal_emit (self, signals[SELECTED_NOTE_CHANGED], 0);
}

static void
bjb_side_view_provider_row_activated_cb (BjbSideView   *self,
                                         GtkListBoxRow *row)
{
  BjbProvider *provider;
  GtkWidget *child;
  BjbItem *note;

  g_assert (BJB_IS_SIDE_VIEW (self));
  g_assert (GTK_IS_LIST_BOX_ROW (row));

  child = gtk_list_box_row_get_child (row);
  provider = g_object_get_data (G_OBJECT (child), "provider");
  g_assert (BJB_IS_PROVIDER (provider));

  note = bjb_plain_note_new_from_data (NULL, NULL, NULL);
  g_object_set_data (G_OBJECT (note), "provider", provider);

  g_set_object (&self->selected_note, note);
  g_signal_emit (self, signals[SELECTED_NOTE_CHANGED], 0);

  gtk_popover_popdown (GTK_POPOVER (self->providers_popover));
}

static void
bjb_side_view_finalize (GObject *object)
{
  BjbSideView *self = (BjbSideView *)object;

  g_clear_object (&self->selected_note);

  G_OBJECT_CLASS (bjb_side_view_parent_class)->finalize (object);
}

static void
bjb_side_view_class_init (BjbSideViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bjb_side_view_finalize;

  signals [SELECTED_NOTE_CHANGED] =
    g_signal_new ("selected-note-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Notes/"
                                               "ui/bjb-side-view.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbSideView, note_list);
  gtk_widget_class_bind_template_child (widget_class, BjbSideView, providers_popover);
  gtk_widget_class_bind_template_child (widget_class, BjbSideView, providers_list_box);

  gtk_widget_class_bind_template_callback (widget_class, bjb_side_view_note_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, bjb_side_view_provider_row_activated_cb);
}

static void
bjb_side_view_init (BjbSideView *self)
{
  GListModel *providers;
  BjbManager *manager;
  GListModel *notes;

  gtk_widget_init_template (GTK_WIDGET (self));

  manager = bjb_manager_get_default ();
  notes = bjb_manager_get_notes (manager);

  providers = bjb_manager_get_providers (manager);
  gtk_list_box_bind_model (GTK_LIST_BOX (self->providers_list_box), providers,
                           provider_row_new, NULL, NULL);

  bjb_note_list_set_model (BJB_NOTE_LIST (self->note_list), notes);
}

GtkWidget *
bjb_side_view_new (void)
{
  return g_object_new (BJB_TYPE_SIDE_VIEW, NULL);
}

BjbItem *
bjb_side_view_get_selected_note (BjbSideView *self)
{
  g_return_val_if_fail (BJB_IS_SIDE_VIEW (self), NULL);

  return self->selected_note;
}
