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

#include "bjb-application.h"
#include "bjb-empty-results-box.h"
#include "bjb-list-view.h"
#include "bjb-list-view-row.h"
#include "bjb-note-view.h"
#include "bjb-notebooks-dialog.h"
#include "bjb-search-toolbar.h"
#include "bjb-share.h"
#include "bjb-window.h"

#define BIJIBEN_MAIN_WIN_TITLE N_("Notes")

struct _BjbWindow
{
  HdyApplicationWindow  parent_instance;

  BjbSettings          *settings;
  BjbController        *controller;

  gulong                display_notebooks_changed;
  gulong                note_deleted;
  gulong                note_trashed;

  GtkStack             *stack;
  BjbWindowView         current_view;
  BjbListView          *note_list;
  BjbNoteView          *note_view;
  GtkWidget            *spinner;
  GtkWidget            *no_note;

  /* when a note is opened */
  BijiNoteObj          *note;

  /* window geometry */
  GdkRectangle          window_geometry;
  gboolean              is_maximized;

  HdyLeaflet           *main_leaflet;
  HdyHeaderGroup       *header_group;
  GtkStack             *main_stack;
  GtkWidget            *back_button;
  GtkWidget            *filter_label;
  GtkWidget            *filter_menu_button;
  GtkWidget            *headerbar;
  GtkWidget            *note_box;
  GtkWidget            *note_headerbar;
  GtkWidget            *notebooks_box;
  GtkWidget            *sidebar_box;
  GtkWidget            *search_bar;
  GtkWidget            *title_entry;
  GtkWidget            *new_window_item;
  GtkWidget            *last_update_item;

  gboolean              is_main;
};

G_DEFINE_TYPE (BjbWindow, bjb_window, HDY_TYPE_APPLICATION_WINDOW)

enum {
  VIEW_CHANGED,
  ACTIVATED,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

static void
destroy_note_if_needed (BjbWindow *self)
{
  g_clear_object (&self->note);

  if (self->note_view && GTK_IS_WIDGET (self->note_view))
    gtk_widget_destroy (GTK_WIDGET (self->note_view));

  gtk_widget_hide (self->title_entry);

  self->note_view = NULL;
}

static void
on_note_renamed (BijiItem      *note,
                 BjbWindow *self)
{
  const char *str;

  str = biji_item_get_title (note);
  if (str == NULL || strlen(str) == 0)
    str = _("Untitled");
  gtk_entry_set_text (GTK_ENTRY (self->title_entry), str);
  hdy_header_bar_set_custom_title (HDY_HEADER_BAR (self->note_headerbar),
                                   self->title_entry);
  gtk_widget_show (self->title_entry);
}

static void
on_note_list_row_activated (GtkListBox    *box,
                            GtkListBoxRow *row,
                            gpointer       user_data)
{
  const char *note_uuid;
  BijiItem *to_open;
  BijiManager *manager;
  BjbWindow *self = BJB_WINDOW (user_data);
  GList *windows;

  windows = gtk_application_get_windows (gtk_window_get_application (GTK_WINDOW (self)));
  manager = bjb_window_get_manager (GTK_WIDGET (self));
  note_uuid = bjb_list_view_row_get_uuid (BJB_LIST_VIEW_ROW (row));
  to_open = biji_manager_get_item_at_path (manager, note_uuid);

  for (GList *node = windows; BIJI_IS_NOTE_OBJ (to_open) && node; node = node->next)
    {
      if (BJB_IS_WINDOW (node->data) &&
          self != node->data &&
          bjb_window_get_note (node->data) == BIJI_NOTE_OBJ (to_open))
        {
          gtk_window_present (node->data);
          return;
        }
    }

  if (to_open && BIJI_IS_NOTE_OBJ (to_open))
    {
      hdy_leaflet_navigate (self->main_leaflet, HDY_NAVIGATION_DIRECTION_FORWARD);

      /* Only open the note if it's not already opened. */
      if (!biji_note_obj_is_opened (BIJI_NOTE_OBJ (to_open)))
        {
          bjb_window_set_note (self, BIJI_NOTE_OBJ (to_open));
        }
    }
  else if (to_open && BIJI_IS_NOTEBOOK (to_open))
    {
      bjb_controller_set_notebook (self->controller, BIJI_NOTEBOOK (to_open));
    }
}

static void
on_back_button_clicked (BjbWindow *self)
{
  hdy_leaflet_navigate (self->main_leaflet, HDY_NAVIGATION_DIRECTION_BACK);
}

static void
on_new_note_clicked (BjbWindow *self)
{
  BijiNoteObj *result;
  BijiManager *manager;

  g_assert (BJB_IS_WINDOW (self));

  /* append note to notebook */
  manager = bjb_window_get_manager (GTK_WIDGET (self));
  result = biji_manager_note_new (manager,
                                  NULL,
                                  bjb_settings_get_default_location (self->settings));

  /* Go to that note */
  hdy_leaflet_navigate (self->main_leaflet, HDY_NAVIGATION_DIRECTION_FORWARD);
  bjb_window_set_note (self, result);
}

static void
on_title_changed (BjbWindow *self,
                  GtkEntry      *title)
{
  const char *str = gtk_entry_get_text (title);

  if (strlen (str) > 0)
    biji_note_obj_set_title (self->note, str);
}

static void
bjb_window_finalize (GObject *object)
{
  BjbWindow *self = BJB_WINDOW (object);
  BjbNoteView *note_view;

  if (self->note != NULL)
    {
      bjb_settings_set_last_opened_item (self->settings, biji_note_obj_get_path (self->note));

      g_clear_signal_handler (&self->note_deleted, self->note);
      g_clear_signal_handler (&self->note_trashed, self->note);
    }

  note_view = g_object_get_data (object, "note-view");
  if (note_view)
    bjb_note_view_set_detached (note_view, FALSE);

  g_clear_signal_handler (&self->display_notebooks_changed, self->controller);
  g_clear_object (&self->controller);
  g_clear_object (&self->note);

  G_OBJECT_CLASS (bjb_window_parent_class)->finalize (object);
}

static void
clear_text_in_search_entry (BjbWindow *self)
{
  GtkEntry *search_entry;

  /* Clear the content in search bar if there is any in it. */
  search_entry = bjb_search_toolbar_get_entry_widget (BJB_SEARCH_TOOLBAR (self->search_bar));
  gtk_entry_set_text (search_entry, "");
}

static gboolean
on_key_pressed_cb (BjbWindow *self, GdkEvent *event)
{
  GApplication *app = g_application_get_default ();
  GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask ();

  if ((event->key.state & modifiers) == GDK_MOD1_MASK &&
      event->key.keyval == GDK_KEY_Left &&
      (self->current_view == BJB_WINDOW_MAIN_VIEW ||
       self->current_view == BJB_WINDOW_ARCHIVE_VIEW))
  {
    BijiItemsGroup items;

    items = bjb_controller_get_group (self->controller);

    /* Back to main view from trash bin */
    if (items == BIJI_ARCHIVED_ITEMS)
      bjb_controller_set_group (self->controller, BIJI_LIVING_ITEMS);
    /* Back to main view */
    else
      bjb_controller_set_notebook (self->controller, NULL);

    return TRUE;
  }

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
note_view_destroyed (BjbWindow *self)
{
  g_assert (BJB_IS_WINDOW (self));

  g_object_set_data (G_OBJECT (self), "note-view", NULL);
}

static void
on_detach_window_cb (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  int width, height;
  BjbWindow *detached_window;
  BjbWindow *self = BJB_WINDOW (user_data);
  BijiNoteObj *note = self->note;

  if (!note)
    return;

  /* Width and height of the detached window. */
  width = gtk_widget_get_allocated_width (GTK_WIDGET (self->note_view));
  gtk_window_get_size (GTK_WINDOW (self), NULL, &height);

  if (biji_note_obj_is_trashed (note))
    bjb_window_set_view (self, BJB_WINDOW_ARCHIVE_VIEW);
  else
    bjb_window_set_view (self, BJB_WINDOW_MAIN_VIEW);

  bjb_note_view_set_detached (self->note_view, TRUE);

  detached_window = BJB_WINDOW (bjb_window_new ());
  gtk_window_set_default_size (GTK_WINDOW (detached_window), width, height);
  g_object_set_data_full (G_OBJECT (detached_window), "note-view",
                          g_object_ref (self->note_view), g_object_unref);
  g_signal_connect_object (self->note_view, "destroy",
                           G_CALLBACK (note_view_destroyed), detached_window,
                           G_CONNECT_SWAPPED);

  detached_window->controller = g_object_ref (self->controller);

  bjb_window_set_view (detached_window, self->current_view);
  bjb_window_set_is_main (detached_window, FALSE);
  bjb_window_set_note (detached_window, note);
  gtk_window_present (GTK_WINDOW (detached_window));
}

static void
on_paste_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  BjbWindow *self = user_data;

  if (self->note)
    biji_webkit_editor_paste (BIJI_WEBKIT_EDITOR (biji_note_obj_get_editor (self->note)));
}

static void
on_undo_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  BjbWindow *self = user_data;

  if (self->note)
    biji_webkit_editor_undo (BIJI_WEBKIT_EDITOR (biji_note_obj_get_editor (self->note)));
}

static void
on_redo_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  BjbWindow *self = user_data;

  if (self->note)
    biji_webkit_editor_redo (BIJI_WEBKIT_EDITOR (biji_note_obj_get_editor (self->note)));
}

static void
on_view_notebooks_cb (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  BjbWindow *self = BJB_WINDOW (user_data);
  GtkWidget *notebooks_dialog;

  if (!self->note)
    return;

  notebooks_dialog = bjb_notebooks_dialog_new (GTK_WINDOW (self));
  bjb_notebooks_dialog_set_item (BJB_NOTEBOOKS_DIALOG (notebooks_dialog), self->note);

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
  clear_text_in_search_entry (self);

  /* Going back from a notebook. */
  if (bjb_controller_get_notebook (self->controller) != NULL)
    bjb_controller_set_notebook (self->controller, NULL);

  bjb_controller_set_group (self->controller, BIJI_LIVING_ITEMS);
  gtk_label_set_text (GTK_LABEL (self->filter_label), _("All Notes"));
}

static void
show_trash (BjbWindow *self)
{
  clear_text_in_search_entry (self);

  bjb_controller_set_group (self->controller, BIJI_ARCHIVED_ITEMS);
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
  BijiItem *notebook;
  BijiManager *manager;

  clear_text_in_search_entry (self);

  note_uuid = g_variant_get_string (variant, NULL);
  if (g_strcmp0 (note_uuid, "ALL NOTES") == 0)
    show_all_notes (self);
  else if (g_strcmp0 (note_uuid, "TRASH") == 0)
    show_trash (self);
  else
  {
    manager = bjb_window_get_manager (GTK_WIDGET (self));
    notebook = biji_manager_find_notebook (manager, note_uuid);
    bjb_controller_set_notebook (self->controller, BIJI_NOTEBOOK (notebook));

    /* Update headerbar title. */
    title = biji_item_get_title (notebook);
    gtk_label_set_text (GTK_LABEL (self->filter_label), title);
  }

  g_simple_action_set_state (action, variant);
}

static void
on_trash_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  BjbWindow *self = BJB_WINDOW (user_data);
  BijiNoteObj *note = self->note;

  if (!note)
    return;

  /* Delete the note from notebook
   * The deleted note will emit a signal. */
  biji_item_trash (BIJI_ITEM (note));

  destroy_note_if_needed (self);
  bjb_window_set_view (self, BJB_WINDOW_MAIN_VIEW);

  if (!bjb_window_get_is_main (self))
    gtk_window_close (GTK_WINDOW (self));
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
  bjb_controller_disconnect (self->controller);
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

static void
append_notebook (BijiItem      *notebook,
                 BjbWindow *self)
{
  const char *note_uuid;
  const char *title;
  GVariant *variant;
  GtkStyleContext *context;
  GtkWidget *button;

  note_uuid = biji_item_get_uuid (notebook);
  variant = g_variant_new_string (note_uuid);

  title = biji_item_get_title (notebook);
  button = gtk_model_button_new ();
  g_object_set (button,
                "action-name", "win.show-notebook",
                "action-target", variant,
                "text", title,
                NULL);

  context = gtk_widget_get_style_context (button);
  gtk_style_context_add_class (context, "modelbutton");

  gtk_widget_show (button);
  gtk_box_pack_start (GTK_BOX (self->notebooks_box), button, TRUE, TRUE, 0);
}

static void
on_display_notebooks_changed (BjbWindow *self)
{
  BijiManager *manager;
  GListModel *notebooks;
  guint n_items;

  g_assert (BJB_IS_WINDOW (self));

  manager = bjb_window_get_manager (GTK_WIDGET (self));
  notebooks = biji_manager_get_notebooks (manager);

  gtk_container_foreach (GTK_CONTAINER (self->notebooks_box),
                         (GtkCallback) gtk_widget_destroy, NULL);

  manager = bjb_window_get_manager (GTK_WIDGET (self));

  n_items = g_list_model_get_n_items (notebooks);

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(BijiItem) item = NULL;

      item = g_list_model_get_item (notebooks, i);
      append_notebook (item, self);
    }
}

static GActionEntry win_entries[] = {
  { "detach-window", on_detach_window_cb, NULL, NULL, NULL },
  { "paste", on_paste_cb, NULL, NULL, NULL },
  { "undo", on_undo_cb, NULL, NULL, NULL },
  { "redo", on_redo_cb, NULL, NULL, NULL },
  { "view-notebooks", on_view_notebooks_cb, NULL, NULL, NULL },
  { "email", on_email_cb, NULL, NULL, NULL },
  { "show-notebook", on_action_radio, "s", "'ALL NOTES'", on_show_notebook_cb },
  { "trash", on_trash_cb, NULL, NULL, NULL },
  { "close", on_close },
};

static void
bjb_window_constructed (GObject *obj)
{
  BijiManager *manager;
  BjbWindow *self = BJB_WINDOW (obj);
  GtkListBox *list_box;
  GListModel *notebooks;
  GdkRectangle geometry;

  G_OBJECT_CLASS (bjb_window_parent_class)->constructed (obj);

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   win_entries,
                                   G_N_ELEMENTS (win_entries),
                                   self);

  self->settings = bjb_app_get_settings ((gpointer) g_application_get_default ());

  gtk_window_set_position (GTK_WINDOW (self), GTK_WIN_POS_CENTER);
  gtk_window_set_title (GTK_WINDOW (self), _(BIJIBEN_MAIN_WIN_TITLE));

  bjb_window_load_geometry (self);
  geometry = self->window_geometry;
  gtk_window_set_default_size (GTK_WINDOW (self), geometry.width, geometry.height);

  if (self->is_maximized)
    gtk_window_maximize (GTK_WINDOW (self));
  else if (geometry.x >= 0)
    gtk_window_move (GTK_WINDOW (self), geometry.x, geometry.y);

  self->note_list = bjb_list_view_new ();

  list_box = bjb_list_view_get_list_box (self->note_list);
  g_signal_connect (list_box, "row-activated",
                    G_CALLBACK (on_note_list_row_activated), self);

  self->spinner = gtk_spinner_new ();
  gtk_stack_add_named (self->main_stack, self->spinner, "spinner");
  gtk_stack_set_visible_child_name (self->main_stack, "spinner");
  gtk_widget_show (self->spinner);
  gtk_spinner_start (GTK_SPINNER (self->spinner));
  gtk_widget_set_sensitive (self->notebooks_box, FALSE);

  self->no_note = bjb_empty_results_box_new ();
  gtk_stack_add_named (self->main_stack, self->no_note, "empty");

  gtk_stack_add_named (self->main_stack, GTK_WIDGET (self->note_list), "main-view");
  gtk_widget_show (GTK_WIDGET (self->main_stack));

  /* Populate the filter menu model. */
  manager = bjb_window_get_manager (GTK_WIDGET (self));
  notebooks = biji_manager_get_notebooks (manager);

  g_signal_connect_object (notebooks, "items-changed",
                           G_CALLBACK (on_display_notebooks_changed), self,
                           G_CONNECT_SWAPPED | G_CONNECT_AFTER);
  on_display_notebooks_changed (self);

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
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bjb_window_class_init (BjbWindowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = bjb_window_constructed;
  object_class->finalize = bjb_window_finalize;

  widget_class->configure_event = bjb_window_configure_event;

  signals[VIEW_CHANGED] = g_signal_new ("view-changed",
                                        G_OBJECT_CLASS_TYPE (klass),
                                        G_SIGNAL_RUN_LAST,
                                        0,
                                        NULL,
                                        NULL,
                                        g_cclosure_marshal_VOID__VOID,
                                        G_TYPE_NONE,
                                        0);

  signals[ACTIVATED] = g_signal_new ("activated",
                                     G_OBJECT_CLASS_TYPE (klass),
                                     G_SIGNAL_RUN_LAST,
                                     0,
                                     NULL,
                                     NULL,
                                     g_cclosure_marshal_VOID__BOOLEAN,
                                     G_TYPE_NONE,
                                     1,
                                     G_TYPE_BOOLEAN);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/bjb-window.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbWindow, main_leaflet);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, header_group);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, main_stack);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, back_button);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, headerbar);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, filter_label);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, filter_menu_button);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, note_box);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, note_headerbar);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, notebooks_box);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, sidebar_box);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, search_bar);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, title_entry);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, new_window_item);
  gtk_widget_class_bind_template_child (widget_class, BjbWindow, last_update_item);

  gtk_widget_class_bind_template_callback (widget_class, on_back_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_new_note_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_title_changed);
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
      if (!self->controller)
        self->controller = bjb_controller_new (bijiben_get_manager (BJB_APPLICATION (g_application_get_default())),
                                               GTK_WINDOW (self), NULL);
      if (!self->display_notebooks_changed)
        {
          BjbSearchToolbar *search_bar;

          search_bar = BJB_SEARCH_TOOLBAR (self->search_bar);
          bjb_list_view_setup (self->note_list, self->controller);
          bjb_search_toolbar_setup (search_bar, GTK_WIDGET (self), self->controller);
        }

      g_clear_signal_handler (&self->display_notebooks_changed, self->controller);
      self->display_notebooks_changed = g_signal_connect_swapped (self->controller,
                                                                  "display-notebooks-changed",
                                                                  G_CALLBACK (on_display_notebooks_changed),
                                                                  self);
    }

  if (!is_main)
    hdy_leaflet_navigate (self->main_leaflet, HDY_NAVIGATION_DIRECTION_FORWARD);
  gtk_widget_set_visible (self->sidebar_box, is_main);
  gtk_widget_set_visible (self->new_window_item, is_main);
  gtk_widget_set_visible (self->back_button, is_main);
  self->is_main = !!is_main;
}

BjbWindowView
bjb_window_get_view (BjbWindow *self)
{
  g_return_val_if_fail (BJB_IS_WINDOW (self), BJB_WINDOW_MAIN_VIEW);

  return self->current_view;
}

void
bjb_window_set_view (BjbWindow     *self,
                     BjbWindowView  view)
{
  g_return_if_fail (BJB_IS_WINDOW (self));

  gtk_widget_set_sensitive (self->notebooks_box, FALSE);

  switch (view)
  {

    /* Precise the window does not display any specific note
     * Refresh the model
     * Ensure the main view receives the proper signals
     *
     * main view & archive view are the same widget
     */

    case BJB_WINDOW_MAIN_VIEW:
      hdy_leaflet_navigate (self->main_leaflet, HDY_NAVIGATION_DIRECTION_BACK);
      gtk_widget_show (GTK_WIDGET (self->search_bar));
      gtk_stack_set_visible_child_name (self->main_stack, "main-view");
      gtk_widget_set_sensitive (self->notebooks_box, TRUE);
      break;

   case BJB_WINDOW_ARCHIVE_VIEW:
      gtk_widget_show (GTK_WIDGET (self->search_bar));
      gtk_stack_set_visible_child_name (self->main_stack, "main-view");
      gtk_widget_set_sensitive (self->notebooks_box, TRUE);
      break;

    case BJB_WINDOW_SPINNER_VIEW:
      gtk_widget_hide (GTK_WIDGET (self->search_bar));
      gtk_stack_set_visible_child_name (self->main_stack, "spinner");
      break;


    case BJB_WINDOW_NO_NOTE:
      bjb_empty_results_box_set_type (BJB_EMPTY_RESULTS_BOX (self->no_note),
                                      BJB_EMPTY_RESULTS_NO_NOTE);
      gtk_widget_show (self->no_note);
      gtk_widget_hide (GTK_WIDGET (self->search_bar));
      gtk_stack_set_visible_child_name (self->main_stack, "empty");
      gtk_widget_set_sensitive (self->notebooks_box, TRUE);
      break;


    case BJB_WINDOW_NO_RESULT:
      bjb_empty_results_box_set_type (BJB_EMPTY_RESULTS_BOX (self->no_note),
                                      BJB_EMPTY_RESULTS_NO_RESULTS);
      gtk_widget_show (self->no_note);
      gtk_stack_set_visible_child_name (self->main_stack, "empty");
      gtk_widget_set_sensitive (self->notebooks_box, TRUE);
      break;


    case BJB_WINDOW_ERROR_TRACKER:
      bjb_empty_results_box_set_type (BJB_EMPTY_RESULTS_BOX (self->no_note),
                                      BJB_EMPTY_RESULTS_TRACKER);
      gtk_widget_show_all (self->no_note);
      gtk_widget_hide (GTK_WIDGET (self->search_bar));
      gtk_stack_set_visible_child_name (self->stack, "empty");
      break;


    case BJB_WINDOW_NOTE_VIEW:
      break;


    case BJB_WINDOW_NO_VIEW:
    default:
      return;
  }

  self->current_view = view;

  g_signal_emit (G_OBJECT (self), signals[VIEW_CHANGED], 0);
}

static void
on_last_updated_cb (BijiItem  *note,
                    BjbWindow *self)
{
  g_autofree char *label = NULL;
  g_autofree char *time_str = NULL;

  time_str = biji_note_obj_get_last_change_date_string (self->note);
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
  on_note_renamed (BIJI_ITEM (self->note), self);
  g_signal_connect (self->note, "renamed", G_CALLBACK (on_note_renamed), self);

  on_last_updated_cb (BIJI_ITEM (self->note), self);
  g_signal_connect (self->note, "changed", G_CALLBACK (on_last_updated_cb), self);
}

static gboolean
on_note_trashed (BijiNoteObj *note,
                 gpointer     user_data)
{
  BjbWindow *self = BJB_WINDOW (user_data);

  destroy_note_if_needed (self);
  bjb_window_set_view (self, BJB_WINDOW_MAIN_VIEW);

  return TRUE;
}

BijiNoteObj *
bjb_window_get_note (BjbWindow *self)
{
  g_return_val_if_fail (BJB_IS_WINDOW (self), NULL);

  return self->note;
}

void
bjb_window_set_note (BjbWindow   *self,
                     BijiNoteObj *note)
{
  g_return_if_fail (BJB_IS_WINDOW (self));
  g_return_if_fail (!note || BIJI_IS_NOTE_OBJ (note));

  /* Disconnect these two callbacks for previously opened note item. */
  if (self->note)
    {
      g_clear_signal_handler (&self->note_deleted, self->note);
      g_clear_signal_handler (&self->note_trashed, self->note);
    }

  destroy_note_if_needed (self);

  if (!note)
    return;

  self->note = g_object_ref (note);
  self->note_view = bjb_note_view_new (note);
  gtk_box_pack_end (GTK_BOX (self->note_box), GTK_WIDGET (self->note_view), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (self->note_view));

  self->note_deleted = g_signal_connect (self->note, "deleted",
                                         G_CALLBACK (on_note_trashed), self);
  self->note_trashed = g_signal_connect (self->note, "trashed",
                                         G_CALLBACK (on_note_trashed), self);

  populate_headerbar_for_note_view (self);
}

BijiManager *
bjb_window_get_manager (GtkWidget *win)
{
  return bijiben_get_manager (BJB_APPLICATION (g_application_get_default()));
}

void
bjb_window_set_active (BjbWindow *self,
                       gboolean   active)
{
  gboolean available;

  g_return_if_fail (BJB_IS_WINDOW (self));

  available = (self->current_view == BJB_WINDOW_MAIN_VIEW);

  if (active == TRUE)
    g_signal_emit (self, signals[ACTIVATED], 0, available);
}
