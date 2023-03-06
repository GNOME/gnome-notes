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

#include <libbiji/libbiji.h>

#include "items/bjb-plain-note.h"
#include "providers/bjb-provider.h"
#include "bjb-application.h"
#include "bjb-manager.h"
#include "bjb-note-list.h"
#include "bjb-list-view-row.h"
#include "bjb-note-view.h"
#include "bjb-notebooks-dialog.h"
#include "bjb-share.h"
#include "bjb-utils.h"
#include "bjb-window.h"

#define BIJIBEN_MAIN_WIN_TITLE N_("Notes")

struct _BjbWindow
{
  HdyApplicationWindow  parent_instance;

  BjbSettings          *settings;

  BjbWindowView         current_view;
  BjbNoteList          *note_list;

  /* when a note is opened */
  BjbNote              *note;

  /* window geometry */
  GdkRectangle          window_geometry;
  gboolean              is_maximized;

  HdyLeaflet           *main_leaflet;
  HdyHeaderGroup       *header_group;
  GtkWidget            *back_button;
  GtkWidget            *filter_label;
  GtkWidget            *filter_menu_button;
  GtkWidget            *headerbar;
  GtkWidget            *note_box;
  GtkWidget            *note_headerbar;
  GtkWidget            *note_view;
  GtkWidget            *notebooks_box;
  GtkWidget            *sidebar_box;
  GtkWidget            *title_entry;
  GtkWidget            *new_window_item;
  GtkWidget            *last_update_item;
  GtkWidget            *providers_popover;
  GtkWidget            *providers_list_box;

  gboolean              is_main;
  gulong                remove_item_id;
};

G_DEFINE_TYPE (BjbWindow, bjb_window, HDY_TYPE_APPLICATION_WINDOW)

static void
destroy_note_if_needed (BjbWindow *self)
{
  BjbProvider *provider = NULL;

  if (self->note)
    provider = g_object_get_data (G_OBJECT (self->note), "provider");

  g_clear_signal_handler (&self->remove_item_id, provider);

  g_clear_object (&self->note);
  bjb_note_view_set_note (BJB_NOTE_VIEW (self->note_view), NULL);

  gtk_widget_hide (self->title_entry);
}

static void
on_note_renamed (BjbWindow *self)
{
  const char *str;

  g_assert (BJB_IS_WINDOW (self));

  str = bjb_item_get_title (BJB_ITEM (self->note));
  if (!str || !*str)
    str = _("Untitled");
  gtk_entry_set_text (GTK_ENTRY (self->title_entry), str);
  hdy_header_bar_set_custom_title (HDY_HEADER_BAR (self->note_headerbar),
                                   self->title_entry);
  gtk_widget_show (self->title_entry);
}

static void
on_back_button_clicked (BjbWindow *self)
{
  hdy_leaflet_navigate (self->main_leaflet, HDY_NAVIGATION_DIRECTION_BACK);
}

static void
on_title_changed (BjbWindow *self,
                  GtkEntry      *title)
{
  const char *str = gtk_entry_get_text (title);

  bjb_item_set_title (BJB_ITEM (self->note), str);
}

static void
window_selected_note_changed_cb (BjbWindow *self)
{
  gpointer to_open;
  GList *windows;

  g_assert (BJB_IS_WINDOW (self));

  windows = gtk_application_get_windows (gtk_window_get_application (GTK_WINDOW (self)));
  to_open = bjb_note_list_get_selected_note (self->note_list);

  for (GList *node = windows; BJB_IS_NOTE (to_open) && node; node = node->next)
    {
      if (BJB_IS_WINDOW (node->data) &&
          self != node->data &&
          bjb_window_get_note (node->data) == BJB_NOTE (to_open))
        {
          gtk_window_present (node->data);
          return;
        }
    }

  if (to_open && BJB_IS_NOTE (to_open))
    {
      hdy_leaflet_navigate (self->main_leaflet, HDY_NAVIGATION_DIRECTION_FORWARD);
      bjb_window_set_note (self, to_open);
    }
}

static void
providers_list_row_activated_cb (BjbWindow     *self,
                                 GtkListBoxRow *row)
{
  BjbProvider *provider;
  GtkWidget *child;
  BjbItem *note;

  g_assert (BJB_IS_WINDOW (self));
  g_assert (GTK_IS_LIST_BOX_ROW (row));

  child = gtk_bin_get_child (GTK_BIN (row));
  provider = g_object_get_data (G_OBJECT (child), "provider");
  g_assert (BJB_IS_PROVIDER (provider));

  note = bjb_plain_note_new_from_data (NULL, NULL, NULL);
  g_object_set_data (G_OBJECT (note), "provider", provider);

  hdy_leaflet_navigate (self->main_leaflet, HDY_NAVIGATION_DIRECTION_FORWARD);
  bjb_window_set_note (self, BJB_NOTE (note));

  gtk_popover_popdown (GTK_POPOVER (self->providers_popover));
}

static void
bjb_window_finalize (GObject *object)
{
  BjbWindow *self = BJB_WINDOW (object);
  BjbNoteView *note_view;

  note_view = g_object_get_data (object, "note-view");
  if (note_view)
    bjb_note_view_set_detached (note_view, FALSE);

  g_clear_object (&self->note);

  G_OBJECT_CLASS (bjb_window_parent_class)->finalize (object);
}

static gboolean
on_key_pressed_cb (BjbWindow *self, GdkEvent *event)
{
  GApplication *app = g_application_get_default ();
  GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask ();

  switch (event->key.keyval)
  {
    case GDK_KEY_F1:
      if ((event->key.state & modifiers) != GDK_CONTROL_MASK)
        {
          bjb_app_help (BJB_APPLICATION (app));
          return TRUE;
        }
      break;

    case GDK_KEY_F10:
      if ((event->key.state & modifiers) != GDK_CONTROL_MASK)
        {
          /* FIXME: Port this to BjbWindow. */
          /* bjb_main_toolbar_open_menu (self->main_toolbar); */
          return TRUE;
        }
      break;

    case GDK_KEY_F2:
    case GDK_KEY_F3:
    case GDK_KEY_F4:
    case GDK_KEY_F5:
    case GDK_KEY_F6:
    case GDK_KEY_F7:
    case GDK_KEY_F8:
    case GDK_KEY_F9:
    case GDK_KEY_F11:
      return TRUE;

    case GDK_KEY_q:
      if ((event->key.state & modifiers) == GDK_CONTROL_MASK)
        {
          g_application_quit (app);
          return TRUE;
        }
      break;

    default:
      ;
    }

  return FALSE;
}

static void
on_paste_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  BjbWindow *self = user_data;

  if (self->note)
    biji_webkit_editor_paste (bjb_note_view_get_editor (BJB_NOTE_VIEW (self->note_view)));
}

static void
on_undo_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  BjbWindow *self = user_data;

  if (self->note)
    biji_webkit_editor_undo (bjb_note_view_get_editor (BJB_NOTE_VIEW (self->note_view)));
}

static void
on_redo_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  BjbWindow *self = user_data;

  if (self->note)
    biji_webkit_editor_redo (bjb_note_view_get_editor (BJB_NOTE_VIEW (self->note_view)));
}

static void
on_view_notebooks_cb (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  BjbWindow *self = BJB_WINDOW (user_data);
  GtkWidget *notebooks_dialog;

  if (BIJI_IS_NOTE_OBJ (self->note))
    return;

  notebooks_dialog = bjb_notebooks_dialog_new (GTK_WINDOW (self));
  bjb_notebooks_dialog_set_item (BJB_NOTEBOOKS_DIALOG (notebooks_dialog),
                                 BIJI_NOTE_OBJ (self->note));

  gtk_dialog_run (GTK_DIALOG (notebooks_dialog));
  gtk_widget_destroy (notebooks_dialog);
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
show_all_notes (BjbWindow *self)
{
  destroy_note_if_needed (self);

  /* Going back from a notebook. */
  bjb_note_list_set_notebook (self->note_list, NULL);

  bjb_note_list_show_trash (self->note_list, FALSE);
  /* bjb_controller_set_group (self->controller, BIJI_LIVING_ITEMS); */
  gtk_label_set_text (GTK_LABEL (self->filter_label), _("All Notes"));
}

static void
show_trash (BjbWindow *self)
{
  destroy_note_if_needed (self);

  g_warning ("Trash");
  bjb_note_list_show_trash (self->note_list, TRUE);
  /* bjb_controller_set_group (self->controller, BIJI_ARCHIVED_ITEMS); */
  gtk_label_set_text (GTK_LABEL (self->filter_label), _("Trash"));
}

static void
on_show_notebook_cb (GSimpleAction *action,
                     GVariant      *variant,
                     gpointer       user_data)
{
  const char *note_uuid;
  const char *title;
  BjbWindow *self = BJB_WINDOW (user_data);
  BjbItem *notebook;
  BijiManager *manager;

  destroy_note_if_needed (self);

  note_uuid = g_variant_get_string (variant, NULL);
  if (g_strcmp0 (note_uuid, "ALL NOTES") == 0)
    show_all_notes (self);
  else if (g_strcmp0 (note_uuid, "TRASH") == 0)
    show_trash (self);
  else
  {
    /* manager = bjb_window_get_manager (GTK_WIDGET (self)); */
    /* notebook = biji_manager_find_notebook (manager, note_uuid); */
    /* bjb_note_list_set_notebook (self->note_list, notebook); */

    /* /\* Update headerbar title. *\/ */
    /* title = bjb_item_get_title (notebook); */
    /* gtk_label_set_text (GTK_LABEL (self->filter_label), title); */
  }

  g_simple_action_set_state (action, variant);
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
on_action_radio (GSimpleAction *action,
                 GVariant      *variant,
                 gpointer       user_data)
{
  g_action_change_state (G_ACTION (action), variant);
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

static gboolean
bjb_window_configure_event (GtkWidget         *widget,
                            GdkEventConfigure *event)
{
  BjbWindow *self;

  self = BJB_WINDOW (widget);

  self->is_maximized = gtk_window_is_maximized (GTK_WINDOW (self));
  if (!self->is_maximized)
    {
      GdkRectangle *geometry = &self->window_geometry;
      gtk_window_get_size (GTK_WINDOW (self), &geometry->width, &geometry->height);
      gtk_window_get_position (GTK_WINDOW (self), &geometry->x, &geometry->y);
    }

  return GTK_WIDGET_CLASS (bjb_window_parent_class)->configure_event (widget,
                                                                           event);
}

static GActionEntry win_entries[] = {
  { "paste", on_paste_cb, NULL, NULL, NULL },
  { "undo", on_undo_cb, NULL, NULL, NULL },
  { "redo", on_redo_cb, NULL, NULL, NULL },
  { "view-notebooks", on_view_notebooks_cb, NULL, NULL, NULL },
  { "email", on_email_cb, NULL, NULL, NULL },
  { "show-notebook", on_action_radio, "s", "'ALL NOTES'", on_show_notebook_cb },
  { "close", on_close },
};

static GtkWidget *
provider_row_new (gpointer item,
                  gpointer user_data)
{
  GtkWidget *grid, *child;
  GtkStyleContext *context;
  GIcon *icon;

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
      child = gtk_image_new_from_gicon (icon, GTK_ICON_SIZE_DND);
      gtk_grid_attach (GTK_GRID (grid), child, 0, 0, 1, 2);
    }

  child = gtk_label_new (bjb_provider_get_name (item));
  gtk_widget_set_halign (child, GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), child, 1, 0, 1, 1);
  gtk_widget_show_all (grid);

  child = gtk_label_new (bjb_provider_get_location_name (item));
  gtk_widget_set_halign (child, GTK_ALIGN_START);
  context = gtk_widget_get_style_context (child);
  gtk_style_context_add_class (context, "dim-label");

  gtk_grid_attach (GTK_GRID (grid), child, 1, 1, 1, 1);
  gtk_widget_show_all (grid);

  return grid;
}

static void
bjb_window_constructed (GObject *obj)
{
  BjbWindow *self = BJB_WINDOW (obj);
  GListModel *providers;
  GdkRectangle geometry;

  G_OBJECT_CLASS (bjb_window_parent_class)->constructed (obj);

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   win_entries,
                                   G_N_ELEMENTS (win_entries),
                                   self);

  self->settings = bjb_app_get_settings ((gpointer) g_application_get_default ());

  providers = bjb_manager_get_providers (bjb_manager_get_default ());
  gtk_list_box_bind_model (GTK_LIST_BOX (self->providers_list_box), providers,
                           provider_row_new, NULL, NULL);
  gtk_window_set_position (GTK_WINDOW (self), GTK_WIN_POS_CENTER);
  gtk_window_set_title (GTK_WINDOW (self), _(BIJIBEN_MAIN_WIN_TITLE));

  bjb_window_load_geometry (self);
  geometry = self->window_geometry;
  gtk_window_set_default_size (GTK_WINDOW (self), geometry.width, geometry.height);

  if (self->is_maximized)
    gtk_window_maximize (GTK_WINDOW (self));
  else if (geometry.x >= 0)
    gtk_window_move (GTK_WINDOW (self), geometry.x, geometry.y);

  gtk_widget_set_sensitive (self->notebooks_box, FALSE);

  /* Connection to window signals */
  g_signal_connect (GTK_WIDGET (self),
                    "destroy",
                    G_CALLBACK (bjb_window_destroy),
                    self);

  /* Keys */

  g_signal_connect (GTK_WIDGET (self),
                    "key-press-event",
                    G_CALLBACK(on_key_pressed_cb),
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
}

static void
bjb_window_class_init (BjbWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = bjb_window_constructed;
  object_class->finalize = bjb_window_finalize;

  widget_class->configure_event = bjb_window_configure_event;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/bjb-window.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbWindow, main_leaflet);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, header_group);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, back_button);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, headerbar);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, filter_label);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, filter_menu_button);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, note_box);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, note_list);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, note_headerbar);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, note_view);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, notebooks_box);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, sidebar_box);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, title_entry);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, new_window_item);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, last_update_item);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, providers_popover);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, providers_list_box);

  gtk_widget_class_bind_template_callback (widget_class, on_back_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_title_changed);
  gtk_widget_class_bind_template_callback (widget_class, window_selected_note_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, providers_list_row_activated_cb);
}

GtkWidget *
bjb_window_new (void)
{
  return g_object_new (BJB_TYPE_WINDOW,
                       "application", g_application_get_default(),
                       NULL);
}

gboolean
bjb_window_get_is_main (BjbWindow *self)
{
  g_return_val_if_fail (BJB_IS_WINDOW (self), FALSE);

  return self->is_main;
}

void
bjb_window_set_is_main (BjbWindow *self,
                        gboolean   is_main)
{
  g_return_if_fail (BJB_IS_WINDOW (self));

  if (is_main)
    {
      BjbManager *manager;
      GListModel *notes;

      /* if (!self->controller) */
      /*   self->controller = bjb_controller_new (bijiben_get_manager (BJB_APPLICATION (g_application_get_default()))); */

      manager = bjb_manager_get_default ();
      notes = bjb_manager_get_notes (manager);
      /* notes = bjb_controller_get_notes (self->controller); */
      bjb_note_list_set_model (self->note_list, notes);
    }

  if (!is_main)
    hdy_leaflet_navigate (self->main_leaflet, HDY_NAVIGATION_DIRECTION_FORWARD);
  gtk_widget_set_visible (self->sidebar_box, is_main);
  gtk_widget_set_visible (self->new_window_item, is_main);
  gtk_widget_set_visible (self->back_button, is_main);
  self->is_main = !!is_main;
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

BjbNote *
bjb_window_get_note (BjbWindow *self)
{
  g_return_val_if_fail (BJB_IS_WINDOW (self), NULL);

  return self->note;
}

static void
window_provider_item_removed_cb (BjbWindow   *self,
                                 BjbItem     *item,
                                 BjbProvider *provider)
{
  g_assert (BJB_IS_WINDOW (self));
  g_assert (BJB_IS_PROVIDER (provider));
  g_assert (BJB_IS_ITEM (item));

  if (self->note == (gpointer)item)
    destroy_note_if_needed (self);
}

void
bjb_window_set_note (BjbWindow *self,
                     BjbNote   *note)
{
  BjbProvider *provider;

  g_return_if_fail (BJB_IS_WINDOW (self));
  g_return_if_fail (!note || BJB_IS_NOTE (note));

  if (self->note == note)
    return;

  destroy_note_if_needed (self);

  if (!note)
    return;

  self->note = g_object_ref (note);
  bjb_note_view_set_note (BJB_NOTE_VIEW (self->note_view), self->note);

  provider = g_object_get_data (G_OBJECT (note), "provider");
  g_assert (BJB_IS_PROVIDER (provider));

  self->remove_item_id = g_signal_connect_object (provider, "item-removed",
                                                  G_CALLBACK (window_provider_item_removed_cb),
                                                  self,
                                                  G_CONNECT_SWAPPED);
  populate_headerbar_for_note_view (self);
}

BijiManager *
bjb_window_get_manager (GtkWidget *win)
{
  return bijiben_get_manager (BJB_APPLICATION (g_application_get_default()));
}
