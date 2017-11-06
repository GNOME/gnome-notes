/* Bijiben
 * Copyright (C) Pierre-Yves Luyten 2012, 2013 <py@luyten.fr>
 *
 * bijiben is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bijiben is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <glib/gi18n.h>
#include <libgd/gd.h>

#include "bjb-application.h"
#include "bjb-color-button.h"
#include "bjb-main-toolbar.h"
#include "bjb-organize-dialog.h"
#include "bjb-share.h"
#include "bjb-window-base.h"

/* All needed toolbars */
typedef enum
{
  BJB_TOOLBAR_0,
  BJB_TOOLBAR_STD_LIST,
  BJB_TOOLBAR_STD_ICON,
  BJB_TOOLBAR_SELECT,
  BJB_TOOLBAR_TRASH_LIST,
  BJB_TOOLBAR_TRASH_ICON,
  BJB_TOOLBAR_TRASH_SELECT,
  BJB_TOOLBAR_NOTE_VIEW
} BjbToolbarType;

/* Color Button */
#define COLOR_SIZE 22

struct _BjbMainToolbar
{
  GtkHeaderBar      parent_instance;

  /* Controllers */
  BjbToolbarType    type;
  BjbMainView      *parent;
  BjbController    *controller;
  GtkWindow        *window;
  GtkWidget        *back;

  /* Main View */
  GtkWidget        *new;
  GtkWidget        *list;
  GtkWidget        *grid;
  GtkWidget        *select;
  GtkWidget        *search;
  GtkWidget        *empty_bin;
  gulong            finish_sig;
  gulong            update_selection;
  gulong            search_handler;
  gulong            display_notes;
  gulong            view_selection_changed;

  /* When note view */
  BijiNoteObj      *note;
  GtkWidget        *color;
  GtkWidget        *share;
  GtkWidget        *menu;
  gulong            note_renamed;
  gulong            note_color_changed;
  GtkAccelGroup    *accel;
};

/* GObject properties */

enum {
  PROP_0,
  PROP_PARENT,
  PROP_CONTROLLER,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (BjbMainToolbar, bjb_main_toolbar, GTK_TYPE_HEADER_BAR)


static void
bjb_main_toolbar_clear (BjbMainToolbar *self)
{
  g_clear_pointer (&self->back      ,gtk_widget_destroy);
  g_clear_pointer (&self->color     ,gtk_widget_destroy);
  g_clear_pointer (&self->grid      ,gtk_widget_destroy);
  g_clear_pointer (&self->list      ,gtk_widget_destroy);
  g_clear_pointer (&self->menu      ,gtk_widget_destroy);
  g_clear_pointer (&self->new       ,gtk_widget_destroy);
  g_clear_pointer (&self->search    ,gtk_widget_destroy);
  g_clear_pointer (&self->select    ,gtk_widget_destroy);
  g_clear_pointer (&self->empty_bin ,gtk_widget_destroy);
}

/* Callbacks */

static void
on_new_note_clicked (GtkWidget *but, BjbMainView *view)
{
  BijiNoteObj *result;
  BijiManager *manager;
  BjbSettings  *settings;

  /* append note to notebook */
  manager = bjb_window_base_get_manager (bjb_main_view_get_window (view));
  settings = bjb_app_get_settings (g_application_get_default ());
  result = biji_manager_note_new (manager,
                                    NULL,
                                    bjb_settings_get_default_location (settings));

  /* Go to that note */
  switch_to_note_view(view,result);
}

static void populate_main_toolbar (BjbMainToolbar *self);

static gboolean
update_selection_label (BjbMainToolbar *self)
{
  GList *selected;
  gint length;
  gchar *label;

  selected = bjb_main_view_get_selected_items (self->parent);
  length = g_list_length (selected);
  g_list_free (selected);

  if (length == 0)
    label = g_strdup(_("Click on items to select them"));
  else
    label = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE, "%d selected", "%d selected", length),length);


  gtk_header_bar_set_title (GTK_HEADER_BAR (self), label);
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (self), NULL);
  g_free (label);

  return TRUE;
}

static void
on_view_selection_changed_cb (BjbMainToolbar *self)
{
  GtkStyleContext *context;
  GtkWidget *widget = GTK_WIDGET (self);
  context = gtk_widget_get_style_context (widget);

  g_return_if_fail (BJB_IS_MAIN_TOOLBAR (self));

  if (!bjb_main_view_get_selection_mode (self->parent))
    gtk_style_context_remove_class (context, "selection-mode");

  else
    gtk_style_context_add_class (context, "selection-mode");

  gtk_widget_reset_style (widget);
  populate_main_toolbar (self);

  /* If we were already on selection mode,
   * the bar is not totaly refreshed. just udpate label */
  if (self->type == BJB_TOOLBAR_SELECT)
    update_selection_label (self);

  return;
}

static void
on_selection_mode_clicked (GtkWidget *button, BjbMainToolbar *self)
{
  if (bjb_main_view_get_selection_mode (self->parent))
  {
    bjb_main_view_set_selection_mode (self->parent, FALSE);
  }

  /* Force refresh. We go to selection mode but nothing yet selected
   * Thus no signal emited */
  else
  {
    bjb_main_view_set_selection_mode (self->parent, TRUE);
    on_view_selection_changed_cb (self);
  }
}

static gboolean
on_view_mode_clicked (GtkWidget *button, BjbMainToolbar *self)
{
  GdMainViewType current = bjb_main_view_get_view_type (self->parent);

  switch ( current )
  {
    case GD_MAIN_VIEW_ICON :
      bjb_main_view_set_view_type (self->parent, GD_MAIN_VIEW_LIST);
      break ;
    case GD_MAIN_VIEW_LIST :
      bjb_main_view_set_view_type (self->parent, GD_MAIN_VIEW_ICON);
      break ;
    default:
      bjb_main_view_set_view_type (self->parent, GD_MAIN_VIEW_ICON);
  }

  bjb_main_view_update_model (self->parent);
  populate_main_toolbar (self);

  return TRUE;
}

/* Just makes toolbar draggable */
static gboolean
on_button_press (GtkWidget* widget,
                 GdkEventButton * event,
                 GdkWindowEdge edge)
{
  if (event->type == GDK_BUTTON_PRESS)
  {
    if (event->button == 1) {
      gtk_window_begin_move_drag (GTK_WINDOW (gtk_widget_get_toplevel (widget)),
                                  event->button,
                                  event->x_root,
                                  event->y_root,
                                  event->time);
    }
  }

  return FALSE;
}

static void
add_search_button (BjbMainToolbar *self)
{
  GtkWidget *search_image;

  self->search = gtk_toggle_button_new ();
  search_image = gtk_image_new_from_icon_name ("edit-find-symbolic", GTK_ICON_SIZE_MENU);
  gtk_button_set_image (GTK_BUTTON (self->search), search_image);
  gtk_widget_set_valign (self->search, GTK_ALIGN_CENTER);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->search),
                               "image-button");
  gtk_header_bar_pack_end (GTK_HEADER_BAR (self), self->search);
  gtk_widget_set_tooltip_text (self->search,
                               _("Search note titles, content and notebooks"));

  g_object_bind_property (self->search,
                          "active",
                          bjb_window_base_get_search_bar (BJB_WINDOW_BASE (self->window)),
                          "search-mode-enabled",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);
}

static void
update_selection_buttons (BjbController *controller,
                          gboolean some_item_is_visible,
                          gboolean remaining,
                          BjbMainToolbar *self)
{
  if (self->grid)
    gtk_widget_set_sensitive (self->grid, some_item_is_visible);

  if (self->list)
    gtk_widget_set_sensitive (self->list, some_item_is_visible);

  if (self->empty_bin)
    gtk_widget_set_sensitive (self->empty_bin, some_item_is_visible);

  gtk_widget_set_sensitive (self->select, some_item_is_visible);
}


static void
populate_bar_for_selection (BjbMainToolbar *self)
{
  GtkSizeGroup *size;

  /* Hide close button */
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self), FALSE);

  size = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);

  /* Select */
  self->select = gtk_button_new_with_mnemonic (_("Cancel"));
  gtk_widget_set_valign (self->select, GTK_ALIGN_CENTER);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->select),
                               "text-button");
  gtk_header_bar_pack_end (GTK_HEADER_BAR (self), self->select);

  /* Search button */
  add_search_button (self);

  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), self->search);
  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), self->select);

  g_object_unref (size);

  gtk_widget_set_tooltip_text (self->select, _("Exit selection mode"));
  gtk_widget_reset_style (self->select);

  g_signal_connect (self->select, "clicked",
                    G_CALLBACK (on_selection_mode_clicked), self);

  if (self->view_selection_changed == 0)
  {
    self->view_selection_changed = g_signal_connect_swapped (
                      self->parent, "view-selection-changed",
                      G_CALLBACK (on_view_selection_changed_cb), self);
  }

  update_selection_label (self);
}

static void
update_label_for_standard (BjbMainToolbar *self)
{
  BijiNotebook *coll;
  gchar *needle, *label;

  coll = bjb_controller_get_notebook (self->controller);
  needle = bjb_controller_get_needle (self->controller);

  if (coll)
    label = g_strdup_printf ("%s", biji_item_get_title (BIJI_ITEM (coll)));

  else if (needle && g_strcmp0 (needle, "") !=0)
    label = g_strdup_printf (_("Results for %s"), needle);

  else
    label = g_strdup (_("New and Recent"));


  gtk_header_bar_set_title (GTK_HEADER_BAR (self), label);
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (self), NULL);
  g_free (label);

  self->display_notes = g_signal_connect (self->controller,
                                                "display-items-changed",
                                                G_CALLBACK (update_selection_buttons),
                                                self);
}

static void
connect_main_view_handlers (BjbMainToolbar *self)
{
  if (self->view_selection_changed == 0)
  {
    self->view_selection_changed = g_signal_connect_swapped (
                      self->parent, "view-selection-changed",
                      G_CALLBACK (on_view_selection_changed_cb), self);
  }
}

static void
on_back_button_clicked (BjbMainToolbar *self)
{
  BijiItemsGroup group;

  group = bjb_controller_get_group (self->controller);

  /* Back to main view from trash bin */
  if (group == BIJI_ARCHIVED_ITEMS)
    bjb_controller_set_group (self->controller, BIJI_LIVING_ITEMS);


  /* Back to main view */
  else
    bjb_controller_set_notebook (self->controller, NULL);
}



static void
on_empty_clicked_callback        (BjbMainToolbar *self)
{
  biji_manager_empty_bin (bjb_window_base_get_manager (GTK_WIDGET (self->window)));
}


static void
populate_bar_for_standard(BjbMainToolbar *self)
{
  BijiNotebook *coll;
  GtkWidget *select_image;
  gboolean rtl;
  GtkSizeGroup *size;

  rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);

  /* Label */
  update_label_for_standard (self);
  self->search_handler = g_signal_connect_swapped (self->controller,
         "search-changed", G_CALLBACK(update_label_for_standard), self);

  /* Go back to all notes */
  coll = bjb_controller_get_notebook (self->controller);

  if (coll != NULL)
  {
    self->back = gtk_button_new_from_icon_name (rtl ? "go-previous-rtl-symbolic" : "go-previous-symbolic",
                                                GTK_ICON_SIZE_MENU);
    gtk_widget_set_valign (self->back, GTK_ALIGN_CENTER);
    gtk_header_bar_pack_start (GTK_HEADER_BAR (self), self->back);

    g_signal_connect_swapped (self->back, "clicked",
                              G_CALLBACK (on_back_button_clicked), self);
  }

  else
  {
    /*
     * Translators : <_New> refers to new note creation.
     * User clicks new, which opens a new blank note.
     */
    self->new = gtk_button_new_with_mnemonic (_("_New"));
    gtk_widget_set_valign (self->new, GTK_ALIGN_CENTER);

    gtk_header_bar_pack_start (GTK_HEADER_BAR (self), self->new);
    gtk_widget_set_size_request (self->new, 70, -1);
    gtk_widget_add_accelerator (self->new, "clicked", self->accel, GDK_KEY_n,
                                GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
    g_signal_connect(self->new,"clicked",
                     G_CALLBACK(on_new_note_clicked),self->parent);
  }

  /* Go to selection mode */
  self->select = gtk_button_new ();
  select_image = gtk_image_new_from_icon_name ("object-select-symbolic", GTK_ICON_SIZE_MENU);
  gtk_button_set_image (GTK_BUTTON (self->select), select_image);
  gtk_widget_set_valign (self->select, GTK_ALIGN_CENTER);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->select),
                               "image-button");
  gtk_header_bar_pack_end (GTK_HEADER_BAR (self), self->select);
  gtk_widget_set_tooltip_text (self->select, _("Selection mode"));

  g_signal_connect (self->select,"clicked",
                    G_CALLBACK(on_selection_mode_clicked),self);

  /* Align buttons */
  size = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), self->select);
  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), self->new);
  g_object_unref (size);

  /* Show close button */
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self), TRUE);

  /* Watch for main view changing */
  connect_main_view_handlers (self);
}


static void
add_list_button (BjbMainToolbar *self)
{
  GtkWidget *list_image;

  self->grid = NULL;
  self->list = gtk_button_new ();
  list_image = gtk_image_new_from_icon_name ("view-list-symbolic", GTK_ICON_SIZE_MENU);
  gtk_button_set_image (GTK_BUTTON (self->list), list_image);
  gtk_widget_set_valign (self->list, GTK_ALIGN_CENTER);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->list),
                               "image-button");
  gtk_header_bar_pack_end (GTK_HEADER_BAR (self), self->list);
  gtk_widget_set_tooltip_text (self->list,
                               _("View notes and notebooks in a list"));
  g_signal_connect (self->list, "clicked",
                    G_CALLBACK(on_view_mode_clicked),self);
}



static void
add_grid_button (BjbMainToolbar *self)
{
  GtkWidget *grid_image;

  self->list = NULL;
  self->grid = gtk_button_new ();
  grid_image = gtk_image_new_from_icon_name ("view-grid-symbolic", GTK_ICON_SIZE_MENU);
  gtk_button_set_image (GTK_BUTTON (self->grid), grid_image);
  gtk_widget_set_valign (self->grid, GTK_ALIGN_CENTER);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->grid),
                               "image-button");
  gtk_header_bar_pack_end (GTK_HEADER_BAR (self), self->grid);
  gtk_widget_set_tooltip_text (self->grid,
                               _("View notes and notebooks in a grid"));

  g_signal_connect (self->grid, "clicked",
                    G_CALLBACK(on_view_mode_clicked),self);
}


static void
populate_bar_for_trash (BjbMainToolbar *self)
{
  gboolean rtl;
  GtkWidget *select_image;
  GtkSizeGroup *size;
  GtkStyleContext *context;

  rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);
  gtk_header_bar_set_title (GTK_HEADER_BAR (self), _("Trash"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (self), NULL);


  self->back = gtk_button_new_from_icon_name (rtl ? "go-previous-rtl-symbolic" : "go-previous-symbolic",
                                              GTK_ICON_SIZE_MENU);
  gtk_widget_set_valign (self->back, GTK_ALIGN_CENTER);
  gtk_header_bar_pack_start (GTK_HEADER_BAR (self), self->back);

  g_signal_connect_swapped (self->back, "clicked",
                            G_CALLBACK (on_back_button_clicked), self);

  /* Go to selection mode */
  self->select = gtk_button_new ();
  select_image = gtk_image_new_from_icon_name ("object-select-symbolic", GTK_ICON_SIZE_MENU);
  gtk_button_set_image (GTK_BUTTON (self->select), select_image);
  gtk_widget_set_valign (self->select, GTK_ALIGN_CENTER);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->select),
                               "image-button");
  gtk_header_bar_pack_end (GTK_HEADER_BAR (self), self->select);
  gtk_widget_set_tooltip_text (self->select, _("Selection mode"));

  g_signal_connect (self->select,"clicked",
                    G_CALLBACK(on_selection_mode_clicked),self);



  /* Add Search ? */

  /* Grid / List */
  if (self->type == BJB_TOOLBAR_TRASH_ICON)
    add_list_button (self);

  if (self->type == BJB_TOOLBAR_TRASH_LIST)
    add_grid_button (self);

  /* Add Empty-Bin
   * translators : Empty is the verb.
   * This action permanently deletes notes */
  self->empty_bin = gtk_button_new_with_label(_("Empty"));
  context = gtk_widget_get_style_context (self->empty_bin);
  gtk_style_context_add_class (context, "destructive-action");
  gtk_widget_set_valign (self->empty_bin, GTK_ALIGN_CENTER);
  gtk_header_bar_pack_end (GTK_HEADER_BAR (self), self->empty_bin);
  g_signal_connect_swapped (self->empty_bin,
                            "clicked",
                            G_CALLBACK (on_empty_clicked_callback),
                            self);



  /* Align buttons */
  size = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), self->select);
  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), self->empty_bin);
  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), self->back);
  g_object_unref (size);

  /* Show close button */
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self), TRUE);

  /* Watch for main view changing */
  update_selection_buttons (
    self->controller, bjb_controller_shows_item (self->controller),
    FALSE, self);
  connect_main_view_handlers (self);
}





static void
populate_bar_for_icon_view(BjbMainToolbar *self)
{
  populate_bar_for_standard(self);
  add_list_button (self);
}

static void
populate_bar_for_list_view(BjbMainToolbar *self)
{
  populate_bar_for_standard(self);
  add_grid_button (self);
}


static void
disconnect_note_handlers (BjbMainToolbar *self)
{
  if (self->note_renamed != 0)
  {
    g_signal_handler_disconnect (self->note, self->note_renamed);
    self->note_renamed = 0;
  }

  if (self->note_color_changed != 0)
  {
    g_signal_handler_disconnect (self->note, self->note_color_changed);
    self->note_color_changed = 0;
  }

  self->note = NULL;
}

static void
just_switch_to_main_view (BjbMainToolbar *self)
{
  disconnect_note_handlers (self);
  bjb_window_base_switch_to (BJB_WINDOW_BASE (self->window),
                             BJB_WINDOW_BASE_MAIN_VIEW);
}

static void
on_note_renamed (BijiItem *note,
                 BjbMainToolbar *self)
{
  const gchar *str;

  str = biji_item_get_title (note);
  if (str == NULL)
    str = _("Untitled");

  gtk_header_bar_set_title (GTK_HEADER_BAR (self), str);
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (self), NULL);
}

static void
on_color_button_clicked (GtkColorButton *button,
                         BjbMainToolbar *bar)
{
  GdkRGBA color;

  if (!bar->note)
    return;

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
  biji_note_obj_set_rgba (bar->note, &color);
}

static void
on_note_color_changed (BijiNoteObj *note, GtkColorButton *button)
{
  GdkRGBA color;

  if (biji_note_obj_get_rgba (note, &color))
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (button), &color);
}



static void
on_note_content_changed (BjbMainToolbar *self)
{
  const gchar *str = NULL;
  gboolean sensitive = TRUE;

  if (self->note)
    str = biji_note_obj_get_raw_text (self->note);

  if (!str || g_strcmp0 (str, "") == 0 || g_strcmp0 (str, "\n") == 0)
    sensitive = FALSE;

  gtk_widget_set_sensitive (self->share, sensitive);
}


static void
action_view_tags_callback (GtkWidget *item, gpointer user_data)
{
  BjbMainToolbar *self = BJB_MAIN_TOOLBAR (user_data);
  GList *list = NULL;

  list = g_list_append (list, self->note);
  bjb_organize_dialog_new (self->window, list);
  g_list_free (list);
}

static void
trash_item_callback (GtkWidget *item, gpointer user_data)
{
  BjbMainToolbar *self = BJB_MAIN_TOOLBAR (user_data);

  /* Delete the note from notebook
   * The deleted note will emit a signal. */
  biji_item_trash (BIJI_ITEM (self->note));
}


static void
on_detached_clicked_cb (BjbMainToolbar *self)
{
  BijiNoteObj *note;

  note = bjb_window_base_get_note (BJB_WINDOW_BASE (self->window));
  bjb_window_base_switch_to (BJB_WINDOW_BASE (self->window),
                             BJB_WINDOW_BASE_MAIN_VIEW);
  bijiben_new_window_for_note (g_application_get_default (), note);
}


static GtkWidget *
bjb_note_menu_new (BjbMainToolbar *self)
{
  GtkWidget             *result, *item;
  BijiWebkitEditor      *editor;
  gboolean               detached;

  result = gtk_menu_new();
  editor = BIJI_WEBKIT_EDITOR (biji_note_obj_get_editor (self->note));

  detached = bjb_window_base_is_detached (BJB_WINDOW_BASE (self->window));
  if (detached == FALSE)
  {
    /*
     * Open the current note in a new window
     * in order to be able to see it and others at the same time
     */
    item = gtk_menu_item_new_with_label (_("Open in New Window"));
    gtk_menu_shell_append (GTK_MENU_SHELL (result), item);
    g_signal_connect_swapped (item, "activate",
                              G_CALLBACK (on_detached_clicked_cb), self);


    item = gtk_separator_menu_item_new ();
    gtk_menu_shell_append (GTK_MENU_SHELL (result), item);
  }

  /* Undo Redo separator */
  item = gtk_menu_item_new_with_label (_("Undo"));
  gtk_menu_shell_append (GTK_MENU_SHELL (result), item);
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (biji_webkit_editor_undo), editor);
  gtk_widget_add_accelerator (item, "activate", self->accel, GDK_KEY_z,
                             GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);


  item = gtk_menu_item_new_with_label (_("Redo"));
  gtk_menu_shell_append (GTK_MENU_SHELL (result), item);
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (biji_webkit_editor_redo), editor);
  gtk_widget_add_accelerator (item, "activate", self->accel, GDK_KEY_z,
                             GDK_CONTROL_MASK | GDK_SHIFT_MASK, GTK_ACCEL_VISIBLE);

  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (result), item);


  /* Notebooks */

  if (biji_item_is_collectable (BIJI_ITEM (self->note)))
  {
    item = gtk_menu_item_new_with_label(_("Notebooks"));
    gtk_menu_shell_append(GTK_MENU_SHELL(result),item);
    g_signal_connect(item,"activate",
                     G_CALLBACK(action_view_tags_callback),self);

  }

  /*Share */
  self->share = gtk_menu_item_new_with_label (_("Email this Note"));
  gtk_menu_shell_append (GTK_MENU_SHELL (result), self->share);
  g_signal_connect (self->share, "activate",
                    G_CALLBACK (on_email_note_callback), self->note);

  g_signal_connect_swapped (biji_note_obj_get_editor (self->note),
                            "content-changed",
                            G_CALLBACK (on_note_content_changed),
                            self);

  on_note_content_changed (self);

  /* Delete Note */
  item = gtk_menu_item_new_with_label(_("Move to Trash"));
  gtk_menu_shell_append(GTK_MENU_SHELL(result),item);
  gtk_widget_add_accelerator (item, "activate", self->accel,
                              GDK_KEY_Delete, GDK_CONTROL_MASK,
                              GTK_ACCEL_VISIBLE);
  g_signal_connect(item,"activate",
                   G_CALLBACK(trash_item_callback),self);

  gtk_widget_show_all (result);
  return result;
}

static void
populate_bar_for_note_view (BjbMainToolbar *self)
{
  GtkHeaderBar          *bar = GTK_HEADER_BAR (self);
  BjbSettings           *settings;
  GdkRGBA                color;
  GtkSizeGroup          *size;
  BijiItem *item;
  gboolean rtl, detached;

  self->note = bjb_window_base_get_note (BJB_WINDOW_BASE (self->window));
  item = BIJI_ITEM (self->note);
  size = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
  rtl = (gtk_widget_get_default_direction () == GTK_TEXT_DIR_RTL);
  detached = bjb_window_base_is_detached (BJB_WINDOW_BASE (self->window));

  if (!self->note) /* no reason this would happen */
    return;

  settings = bjb_app_get_settings (g_application_get_default());

  /* Go to main view basically means closing note */
  if (!detached)
  {
    self->back = gtk_button_new_from_icon_name (rtl ? "go-previous-rtl-symbolic" : "go-previous-symbolic",
                                                GTK_ICON_SIZE_MENU);
    gtk_header_bar_pack_start (bar, self->back);
    gtk_size_group_add_widget (GTK_SIZE_GROUP (size), self->back);

    g_signal_connect_swapped (self->back, "clicked",
                              G_CALLBACK (just_switch_to_main_view), self);
    gtk_widget_add_accelerator (self->back, "activate", self->accel,
                                GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_MASK);
  }

  /* Note Title */
  on_note_renamed (item, self);
  self->note_renamed = g_signal_connect (self->note,"renamed",
                                    G_CALLBACK (on_note_renamed), self);

  /* Menu */

  self->menu = gtk_menu_button_new ();
  gtk_menu_button_set_direction (GTK_MENU_BUTTON (self->menu), GTK_ARROW_NONE);
  gtk_style_context_add_class (gtk_widget_get_style_context (self->menu),
                               "image-button");
  gtk_header_bar_pack_end (bar, self->menu);
  gtk_widget_set_tooltip_text (self->menu, _("More options…"));
  gtk_size_group_add_widget (GTK_SIZE_GROUP (size), self->menu);

  /* Show close button */
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self), TRUE);

  gtk_menu_button_set_popup (GTK_MENU_BUTTON (self->menu),
                             bjb_note_menu_new (self));


  /* Note Color */
  if (biji_item_has_color (BIJI_ITEM (self->note)))
  {
    if (!biji_note_obj_get_rgba (self->note, &color))
    {
      gchar *default_color;

      g_object_get (G_OBJECT(settings),"color", &default_color, NULL);
      gdk_rgba_parse (&color, default_color);
      g_free (default_color);
    }

    self->color = bjb_color_button_new ();
    gtk_widget_set_tooltip_text (self->color, _("Note color"));
    gtk_style_context_add_class (gtk_widget_get_style_context (self->color),
                                 "button");
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (self->color), &color);


    gtk_header_bar_pack_end (bar, self->color);
    gtk_widget_set_size_request (gtk_bin_get_child (GTK_BIN (self->color)),
                                 COLOR_SIZE, COLOR_SIZE);
    gtk_widget_show (self->color);
    gtk_size_group_add_widget (size, self->color);


    g_signal_connect (self->color, "color-set",
                      G_CALLBACK (on_color_button_clicked), self);
    self->note_color_changed = g_signal_connect (self->note, "color-changed",
                               G_CALLBACK (on_note_color_changed), self->color);
  }

  g_object_unref (size);
}

static void
populate_bar_switch (BjbMainToolbar *self)
{
  switch (self->type)
  {
    case BJB_TOOLBAR_SELECT:
    case BJB_TOOLBAR_TRASH_SELECT:
      populate_bar_for_selection (self);
      break;

    case BJB_TOOLBAR_STD_ICON:
      populate_bar_for_icon_view(self);
      update_selection_buttons (self->controller,
                                bjb_controller_shows_item (self->controller),
                                TRUE,
                                self);
      add_search_button (self);
      break;

    case BJB_TOOLBAR_STD_LIST:
      populate_bar_for_list_view(self);
      update_selection_buttons (self->controller,
                                bjb_controller_shows_item (self->controller),
                                TRUE,
                                self);
      add_search_button (self);
      break;


    case BJB_TOOLBAR_TRASH_ICON:
    case BJB_TOOLBAR_TRASH_LIST:
      populate_bar_for_trash (self);
      break;


    case BJB_TOOLBAR_NOTE_VIEW:
      populate_bar_for_note_view (self);
      break;

    /* Spinner, Empty Results */
    case BJB_TOOLBAR_0:
    default:
      gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self), TRUE);
      break;
  }

  gtk_widget_show_all (GTK_WIDGET (self));
}

static void
populate_main_toolbar(BjbMainToolbar *self)
{
  BjbToolbarType to_be = BJB_TOOLBAR_0 ;
  BjbWindowViewType view_type;

  view_type = bjb_window_base_get_view_type (BJB_WINDOW_BASE (self->window));

  switch (view_type)
  {
    case BJB_WINDOW_BASE_NOTE_VIEW:
      to_be = BJB_TOOLBAR_NOTE_VIEW;
      break;

    case BJB_WINDOW_BASE_NO_NOTE:
    case BJB_WINDOW_BASE_NO_RESULT:
    case BJB_WINDOW_BASE_MAIN_VIEW:

      if (bjb_main_view_get_selection_mode (self->parent) == TRUE)
        to_be = BJB_TOOLBAR_SELECT;

      else if (bjb_main_view_get_view_type (self->parent) == GD_MAIN_VIEW_ICON)
        to_be = BJB_TOOLBAR_STD_ICON;

      else if (bjb_main_view_get_view_type (self->parent) == GD_MAIN_VIEW_LIST)
        to_be = BJB_TOOLBAR_STD_LIST;

      break;


    case BJB_WINDOW_BASE_ARCHIVE_VIEW:

      if (bjb_main_view_get_selection_mode (self->parent) == TRUE)
        to_be = BJB_TOOLBAR_TRASH_SELECT;

      else if (bjb_main_view_get_view_type (self->parent) == GD_MAIN_VIEW_ICON)
        to_be = BJB_TOOLBAR_TRASH_ICON;

      else if (bjb_main_view_get_view_type (self->parent) == GD_MAIN_VIEW_LIST)
        to_be = BJB_TOOLBAR_TRASH_LIST;


      break;



    /* Not really a toolbar,
     * still used for Spinner */
    case BJB_WINDOW_BASE_SPINNER_VIEW:
    case BJB_WINDOW_BASE_ERROR_TRACKER:
    case BJB_WINDOW_BASE_NO_VIEW:
    default:
      break;
  }


  /* Simply clear then populate */
  if (to_be != self->type || view_type == BJB_WINDOW_BASE_ARCHIVE_VIEW)
  {
    /* If we leave a note view */
    if (self->type == BJB_TOOLBAR_NOTE_VIEW)
      disconnect_note_handlers (self);

    self->type = to_be;
    bjb_main_toolbar_clear (self);


    if (self->search_handler != 0)
    {
      g_signal_handler_disconnect (self->controller, self->search_handler);
      self->search_handler = 0;
    }

    if (self->display_notes != 0)
    {
      g_signal_handler_disconnect (self->controller, self->display_notes);
      self->display_notes = 0;
    }

    if (self->view_selection_changed != 0)
    {
      g_signal_handler_disconnect (self->parent, self->view_selection_changed);
      self->view_selection_changed = 0;
    }

    populate_bar_switch (self);
  }
}

static void
bjb_main_toolbar_constructed (GObject *obj)
{
  BjbMainToolbar *self = BJB_MAIN_TOOLBAR (obj);

  self->accel = gtk_accel_group_new ();
  gtk_window_add_accel_group (self->window, self->accel);
  g_signal_connect_swapped (self->window, "view-changed",
                            G_CALLBACK (populate_main_toolbar), self);

  G_OBJECT_CLASS(bjb_main_toolbar_parent_class)->constructed(obj);
}

static void
bjb_main_toolbar_init (BjbMainToolbar *self)
{
  self->type = BJB_TOOLBAR_0;
  g_signal_connect (self, "button-press-event", G_CALLBACK (on_button_press), NULL);
}

static void
bjb_main_toolbar_finalize (GObject *object)
{
  BjbMainToolbar *self = BJB_MAIN_TOOLBAR(object);

  if (self->search_handler != 0)
  {
    g_signal_handler_disconnect (self->controller, self->search_handler);
    self->search_handler = 0;
  }

  gtk_window_remove_accel_group (self->window, self->accel);
  disconnect_note_handlers (self);

  /* chain up */
  G_OBJECT_CLASS (bjb_main_toolbar_parent_class)->finalize (object);
}

static void
bjb_main_toolbar_get_property (GObject     *object,
                               guint       property_id,
                               GValue      *value,
                               GParamSpec  *pspec)
{
  BjbMainToolbar *self = BJB_MAIN_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_PARENT:
      g_value_set_object (value, self->parent);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bjb_main_toolbar_set_property (GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BjbMainToolbar *self = BJB_MAIN_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_PARENT:
      self->parent = g_value_get_object(value);
      self->window = GTK_WINDOW (
                       bjb_main_view_get_window (self->parent));
      break;
    case PROP_CONTROLLER:
      self->controller = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bjb_main_toolbar_class_init (BjbMainToolbarClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bjb_main_toolbar_get_property;
  object_class->set_property = bjb_main_toolbar_set_property;
  object_class->constructed = bjb_main_toolbar_constructed;
  object_class->finalize = bjb_main_toolbar_finalize;

  properties[PROP_PARENT] = g_param_spec_object ("parent",
                                                 "Parent",
                                                 "Parent",
                                                 BJB_TYPE_MAIN_VIEW,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT |
                                                 G_PARAM_STATIC_STRINGS);

  properties[PROP_CONTROLLER] = g_param_spec_object ("controller",
                                "BjbController",
                                "Controller for notes model and search",
                                BJB_TYPE_CONTROLLER,
                                G_PARAM_READWRITE |
                                G_PARAM_CONSTRUCT |
                                G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
}

BjbMainToolbar *
bjb_main_toolbar_new (BjbMainView *parent,
                      BjbController *controller)
{
  BjbMainToolbar *self;

  self = BJB_MAIN_TOOLBAR (g_object_new (BJB_TYPE_MAIN_TOOLBAR,
                                         "controller", controller,
                                         "parent", parent,
                                         NULL));

  populate_main_toolbar(self);
  return self;
}
