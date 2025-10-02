/* bjb-window.c
 *
 * Copyright 2012, 2013 Pierre-Yves Luyten <py@luyten.fr>
 * Copyright 2020 Jonathan Kang <jonathankang@gnome.org>
 * Copyright 2021 Mohammed Sadiq <sadiq@sadiqpk.org>
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
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


#define G_LOG_DOMAIN "bjb-window"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <stdlib.h>

#include "providers/bjb-provider.h"
#include "bjb-application.h"
#include "bjb-manager.h"
#include "bjb-note-list.h"
#include "bjb-note-view.h"
#include "bjb-side-view.h"
#include "bjb-notebooks-dialog.h"
#include "bjb-share.h"
#include "bjb-utils.h"
#include "bjb-window.h"

#define BIJIBEN_MAIN_WIN_TITLE N_("Notes")

struct _BjbWindow
{
  AdwApplicationWindow  parent_instance;

  BjbSettings          *settings;

  /* when a note is opened */
  BjbNote              *note;

  /* window geometry */
  GdkRectangle          window_geometry;
  gboolean              is_maximized;

  AdwLeaflet           *main_leaflet;
  GtkWidget            *side_view;
  GtkWidget            *back_button;
  GtkWidget            *note_box;
  GtkWidget            *note_headerbar;
  GtkWidget            *note_view;
  GtkWidget            *title_entry;
  GtkWidget            *last_update_item;

  gboolean              is_self_change;
  gulong                remove_item_id;
};

G_DEFINE_TYPE (BjbWindow, bjb_window, ADW_TYPE_APPLICATION_WINDOW)

static void
window_item_removed_cb (BjbWindow   *self,
                        BjbProvider *provider,
                        BjbNote     *item)
{
  g_assert (BJB_IS_WINDOW (self));
  g_assert (!item || BJB_IS_ITEM (item));

  if (item != self->note)
    return;

  g_clear_object (&self->note);
  bjb_note_view_set_note (BJB_NOTE_VIEW (self->note_view), NULL);
  gtk_widget_set_visible (self->title_entry, FALSE);
}

static void
on_note_renamed (BjbWindow *self)
{
  const char *str;

  g_assert (BJB_IS_WINDOW (self));

  str = bjb_item_get_title (BJB_ITEM (self->note));
  if (!str || !*str)
    str = _("Untitled");

  self->is_self_change = TRUE;
  gtk_editable_set_text (GTK_EDITABLE (self->title_entry), str);
  self->is_self_change = FALSE;

  adw_header_bar_set_title_widget (ADW_HEADER_BAR (self->note_headerbar),
                                   self->title_entry);
  gtk_widget_set_visible (self->title_entry, TRUE);
}

static void
on_back_button_clicked (BjbWindow *self)
{
  adw_leaflet_navigate (self->main_leaflet, ADW_NAVIGATION_DIRECTION_BACK);
}

static void
on_title_changed (BjbWindow   *self,
                  GtkEditable *editable)
{
  const char *str;

  if (self->is_self_change)
    return;

  str = gtk_editable_get_text (editable);
  bjb_item_set_title (BJB_ITEM (self->note), str);
}

static void
window_selected_note_changed_cb (BjbWindow *self)
{
  gpointer to_open;

  g_assert (BJB_IS_WINDOW (self));

  to_open = bjb_side_view_get_selected_note(BJB_SIDE_VIEW (self->side_view));

  if (to_open != self->note && BJB_IS_NOTE (to_open))
    bjb_window_set_note (self, to_open);

  adw_leaflet_navigate (self->main_leaflet, ADW_NAVIGATION_DIRECTION_FORWARD);
}

static void
bjb_window_finalize (GObject *object)
{
  BjbWindow *self = BJB_WINDOW (object);

  g_clear_object (&self->note);

  G_OBJECT_CLASS (bjb_window_parent_class)->finalize (object);
}

static void
on_view_notebooks_cb (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  BjbWindow *self = BJB_WINDOW (user_data);
  GtkWidget *notebooks_dialog;

  if (!BJB_IS_NOTE (self->note))
    return;

  notebooks_dialog = bjb_notebooks_dialog_new (GTK_WINDOW (self));
  bjb_notebooks_dialog_set_item (BJB_NOTEBOOKS_DIALOG (notebooks_dialog),
                                 BJB_NOTE (self->note));

  g_signal_connect_swapped (notebooks_dialog, "response",
                            G_CALLBACK (gtk_window_destroy), notebooks_dialog);
  gtk_window_present (GTK_WINDOW (notebooks_dialog));
}

static void
on_email_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  BjbWindow *self = user_data;

  if (self->note)
    on_email_note_callback (self->note);
}

static void
on_close (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
  GtkApplicationWindow *window;

  window = GTK_APPLICATION_WINDOW (user_data);

  gtk_window_close (GTK_WINDOW (window));
}

static void
bjb_window_save_geometry (BjbWindow *self)
{
  bjb_settings_set_window_maximized (self->settings, self->is_maximized);
  bjb_settings_set_window_geometry (self->settings, &self->window_geometry);
}

static void
bjb_window_load_geometry (BjbWindow *self)
{
  self->is_maximized = bjb_settings_get_window_maximized (self->settings);
  bjb_settings_get_window_geometry (self->settings, &self->window_geometry);
}

/* Just disconnect to avoid crash, the finalize does the real
 * job */
static void
bjb_window_destroy (gpointer a, BjbWindow * self)
{
  bjb_window_save_geometry (self);
}

static GActionEntry win_entries[] = {
  { "view-notebooks", on_view_notebooks_cb, NULL, NULL, NULL },
  { "email", on_email_cb, NULL, NULL, NULL },
  { "close", on_close },
};

static void
bjb_window_constructed (GObject *obj)
{
  BjbWindow *self = BJB_WINDOW (obj);
  GdkRectangle geometry;

  G_OBJECT_CLASS (bjb_window_parent_class)->constructed (obj);

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   win_entries,
                                   G_N_ELEMENTS (win_entries),
                                   self);

  self->settings = bjb_app_get_settings ((gpointer) g_application_get_default ());

  gtk_window_set_title (GTK_WINDOW (self), _(BIJIBEN_MAIN_WIN_TITLE));

  bjb_window_load_geometry (self);
  geometry = self->window_geometry;
  gtk_window_set_default_size (GTK_WINDOW (self), geometry.width, geometry.height);

  if (self->is_maximized)
    gtk_window_maximize (GTK_WINDOW (self));

  /* Connection to window signals */
  g_signal_connect (GTK_WIDGET (self),
                    "destroy",
                    G_CALLBACK (bjb_window_destroy),
                    self);

  /* If a note is requested at creation, show it
   * This is a specific type of window not associated with any view */
  if (self->note != NULL)
    {
      bjb_window_set_note (self, self->note);
    }
}


static void
bjb_window_init (BjbWindow *self)
{
  BjbManager *manager;

  gtk_widget_init_template (GTK_WIDGET (self));

  manager = bjb_manager_get_default ();
  bjb_manager_load (manager);

  g_signal_connect_object (manager, "item-removed",
                           G_CALLBACK (window_item_removed_cb),
                           self, G_CONNECT_SWAPPED);
}

static void
bjb_window_class_init (BjbWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = bjb_window_constructed;
  object_class->finalize = bjb_window_finalize;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/bjb-window.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbWindow, main_leaflet);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, side_view);

  gtk_widget_class_bind_template_child (widget_class, BjbWindow, back_button);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, note_box);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, note_headerbar);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, note_view);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, title_entry);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, last_update_item);

  gtk_widget_class_bind_template_callback (widget_class, on_back_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_title_changed);
  gtk_widget_class_bind_template_callback (widget_class, window_selected_note_changed_cb);

  g_type_ensure (BJB_TYPE_SIDE_VIEW);
  g_type_ensure (BJB_TYPE_NOTE_LIST);
  g_type_ensure (BJB_TYPE_NOTE_VIEW);
}

GtkWidget *
bjb_window_new (void)
{
  return g_object_new (BJB_TYPE_WINDOW,
                       "application", g_application_get_default(),
                       NULL);
}

static void
on_last_updated_cb (BjbWindow *self)
{
  g_autofree char *label = NULL;
  g_autofree char *time_str = NULL;

  g_assert (BJB_IS_WINDOW (self));

  time_str = bjb_utils_get_human_time (bjb_item_get_mtime (BJB_ITEM (self->note)));
  /* Translators: %s is the note last recency description.
   * Last updated is placed as in left to right language
   * right to left languages might move %s
   *         '%s Last Updated'
   */
  label = g_strdup_printf (_("Last updated: %s"), time_str);
  gtk_label_set_text (GTK_LABEL (self->last_update_item), label);
}

static void
populate_headerbar_for_note_view (BjbWindow *self)
{
  g_signal_connect_object (self->note, "notify::renamed",
                           G_CALLBACK (on_note_renamed),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->note, "notify::mtime",
                           G_CALLBACK (on_last_updated_cb),
                           self,
                           G_CONNECT_SWAPPED);


  on_note_renamed (self);
  on_last_updated_cb (self);
}

void
bjb_window_set_note (BjbWindow *self,
                     BjbNote   *note)
{
  g_return_if_fail (BJB_IS_WINDOW (self));
  g_return_if_fail (!note || BJB_IS_NOTE (note));

  if (self->note == note)
    return;

  if (!note)
    {
      window_item_removed_cb (self, NULL, NULL);
      return;
    }

  g_set_object (&self->note, note);
  bjb_note_view_set_note (BJB_NOTE_VIEW (self->note_view), self->note);
  populate_headerbar_for_note_view (self);
}
