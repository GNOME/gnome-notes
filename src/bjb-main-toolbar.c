/* Bijiben
 * Copyright (C) Pierre-Yves Luyten 2012, 2013 <py@luyten.fr>
 * Copyright 2017 Mohammed Sadiq <sadiq@sadiqpk.org>
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
  GtkHeaderBar parent_instance;

  /* Controllers */
  BjbToolbarType type;
  BjbMainView *parent;
  BjbController *controller;
  GtkWindow *window;

  /* Main View */
  GtkWidget *new_button;
  GtkWidget *back_button;
  GtkWidget *title_entry;
  GtkWidget *list_button;
  GtkWidget *grid_button;
  GtkWidget *select_button;
  GtkWidget *cancel_button;
  GtkWidget *search_button;
  GtkWidget *style_buttons;
  GtkWidget *empty_button;
  GtkWidget *color_button;
  GtkWidget *button_stack;
  GtkWidget *main_button;
  GtkWidget *menu_button;

  /* Menu items */
  GtkWidget *new_window_item;
  GtkWidget *undo_item;
  GtkWidget *redo_item;
  GtkWidget *trash_item;
  GtkWidget *last_update_item;

  /* Signals */
  gulong search_handler;
  gulong display_notes;
  gulong view_selection_changed;

  /* When note view */
  BijiNoteObj *note;
  gulong note_renamed;
  gulong note_color_changed;
  gulong last_updated;
  GtkAccelGroup *accel;
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
on_new_note_clicked (BjbMainToolbar *self)
{
  BijiNoteObj *result;
  BijiManager *manager;
  BjbSettings  *settings;

  g_assert (BJB_IS_MAIN_TOOLBAR (self));

  /* append note to notebook */
  manager = bjb_window_base_get_manager (bjb_main_view_get_window (self->parent));
  settings = bjb_app_get_settings (g_application_get_default ());
  result = biji_manager_note_new (manager,
                                    NULL,
                                    bjb_settings_get_default_location (settings));

  /* Go to that note */
  switch_to_note_view (self->parent, result);
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
}

static void
on_selection_mode_clicked (BjbMainToolbar *self,
                           GtkWidget      *button)
{
  g_assert (BJB_IS_MAIN_TOOLBAR (self));
  g_assert (GTK_IS_BUTTON (button));

  if (button == self->cancel_button)
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
on_view_mode_clicked (BjbMainToolbar *self,
                      GtkWidget      *button)
{
  GdMainViewType current;

  g_assert (BJB_IS_MAIN_TOOLBAR (self));
  g_assert (GTK_IS_BUTTON (button));

  current = bjb_main_view_get_view_type (self->parent);

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
  gtk_widget_hide (button);
  return TRUE;
}

static void
update_selection_buttons (BjbController *controller,
                          gboolean some_item_is_visible,
                          gboolean remaining,
                          BjbMainToolbar *self)
{
  gtk_widget_set_sensitive (self->grid_button, some_item_is_visible);
  gtk_widget_set_sensitive (self->list_button, some_item_is_visible);
  gtk_widget_set_sensitive (self->empty_button, some_item_is_visible);
  gtk_widget_set_sensitive (self->search_button, some_item_is_visible);
  gtk_widget_set_sensitive (self->select_button, some_item_is_visible);
}

static void
connect_main_view_handlers (BjbMainToolbar *self)
{
  if (self->view_selection_changed == 0)
    {
      self->view_selection_changed =
        g_signal_connect_swapped (self->parent, "view-selection-changed",
                                  G_CALLBACK (on_view_selection_changed_cb), self);
    }
}

static void
populate_bar_for_selection (BjbMainToolbar *self)
{
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self), FALSE);
  gtk_widget_hide (self->select_button);
  gtk_widget_hide (self->button_stack);
  gtk_widget_hide (self->style_buttons);
  gtk_widget_hide (self->main_button);
  gtk_widget_show (self->cancel_button);

  connect_main_view_handlers (self);
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

  if (self->last_updated != 0)
    {
      g_signal_handler_disconnect (self->note, self->last_updated);
      self->last_updated = 0;
    }

  self->note = NULL;
}

static void
on_back_button_clicked (BjbMainToolbar *self)
{
  BijiItemsGroup group;

  if (self->note)
    {
      if (biji_note_obj_is_trashed (self->note))
      {
        bjb_window_base_switch_to (BJB_WINDOW_BASE (self->window),
                                   BJB_WINDOW_BASE_ARCHIVE_VIEW);
      }
      else
      {
        bjb_window_base_switch_to (BJB_WINDOW_BASE (self->window),
                                   BJB_WINDOW_BASE_MAIN_VIEW);
      }
      disconnect_note_handlers (self);
      bjb_main_view_update_model (self->parent);
      return;
    }

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
  /* Check for Notebook view */
  if (bjb_controller_get_notebook (self->controller))
    {
      gtk_widget_hide (self->new_button);
      gtk_widget_hide (self->main_button);
      gtk_widget_show (self->back_button);
    }

  /* Label */
  update_label_for_standard (self);
  self->search_handler = g_signal_connect_swapped (self->controller,
         "search-changed", G_CALLBACK(update_label_for_standard), self);
  connect_main_view_handlers (self);
}

static void
populate_bar_for_trash (BjbMainToolbar *self)
{
  gtk_header_bar_set_title (GTK_HEADER_BAR (self), _("Trash"));
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (self), NULL);

  gtk_widget_hide (self->new_button);
  gtk_widget_hide (self->search_button);
  gtk_widget_hide (self->main_button);
  gtk_widget_show (self->empty_button);
  gtk_widget_show (self->back_button);

  /* Watch for main view changing */
  update_selection_buttons (
    self->controller, bjb_controller_shows_item (self->controller),
    FALSE, self);
  connect_main_view_handlers (self);
}

static void
on_note_renamed (BijiItem *note,
                 BjbMainToolbar *self)
{
  const gchar *str;

  str = biji_item_get_title (note);
  if (str == NULL || strlen(str) == 0)
    str = _("Untitled");

  gtk_header_bar_set_title (GTK_HEADER_BAR (self), str);
  gtk_header_bar_set_subtitle (GTK_HEADER_BAR (self), NULL);

  gtk_entry_set_text (GTK_ENTRY (self->title_entry), str);
}

static void
on_color_button_clicked (BjbMainToolbar *self,
                         GtkColorButton *button)
{
  GdkRGBA color;

  if (!self->note)
    return;

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
  biji_note_obj_set_rgba (self->note, &color);
}

static void
on_note_color_changed (BijiNoteObj    *note,
                       GtkColorButton *button)
{
  GdkRGBA color;

  if (biji_note_obj_get_rgba (note, &color))
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (button), &color);
}

static void
on_last_updated_cb (BijiItem       *note,
                    BjbMainToolbar *self)
{
  g_autofree gchar *label;
  g_autofree gchar *time_str = NULL;

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
on_title_changed (BjbMainToolbar *self,
                  GtkEntry       *title)
{
  const char *str = gtk_entry_get_text (title);
  if (strlen (str) > 0)
    biji_note_obj_set_title (self->note, str);
}

static void
populate_bar_for_note_view (BjbMainToolbar *self)
{
  BjbSettings *settings;
  GdkRGBA      color;
  gboolean     detached;

  self->note = bjb_window_base_get_note (BJB_WINDOW_BASE (self->window));
  detached = bjb_window_base_is_detached (BJB_WINDOW_BASE (self->window));

  if (!self->note) /* no reason this would happen */
    return;

  settings = bjb_app_get_settings (g_application_get_default());

  gtk_header_bar_set_custom_title (GTK_HEADER_BAR (self), self->title_entry);

  gtk_widget_hide (self->new_button);
  gtk_widget_hide (self->style_buttons);
  gtk_widget_hide (self->search_button);
  gtk_widget_hide (self->select_button);
  gtk_widget_hide (self->main_button);

  gtk_widget_show (self->back_button);
  gtk_widget_show (self->menu_button);

  if (detached)
    {
      gtk_widget_hide (self->new_window_item);
      gtk_widget_hide (self->button_stack);
    }

  /* Note Title */
  on_note_renamed (BIJI_ITEM (self->note), self);
  self->note_renamed = g_signal_connect (self->note,"renamed",
                                    G_CALLBACK (on_note_renamed), self);

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

    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (self->color_button), &color);
    gtk_widget_show (self->color_button);
    self->note_color_changed = g_signal_connect (self->note, "color-changed",
                               G_CALLBACK (on_note_color_changed), self->color_button);
  }

  if (biji_note_obj_is_trashed (self->note))
  {
    gtk_widget_hide (self->trash_item);
  }

  /* Note Last Updated */
  on_last_updated_cb (BIJI_ITEM (self->note), self);
  self->last_updated = g_signal_connect (self->note,"changed",
                                         G_CALLBACK (on_last_updated_cb), self);

}

static void
bjb_main_toolbar_reset (BjbMainToolbar *self)
{
  gtk_header_bar_set_custom_title (GTK_HEADER_BAR (self), NULL);
  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (self), TRUE);

  gtk_widget_show (self->button_stack);
  gtk_widget_show (self->style_buttons);
  gtk_widget_show (self->select_button);
  gtk_widget_show (self->search_button);
  gtk_widget_show (self->new_button);
  gtk_widget_show (self->main_button);

  gtk_widget_hide (self->back_button);
  gtk_widget_hide (self->color_button);
  gtk_widget_hide (self->empty_button);
  gtk_widget_hide (self->cancel_button);
  gtk_widget_hide (self->menu_button);
}

static void
populate_bar_switch (BjbMainToolbar *self)
{
  bjb_main_toolbar_reset (self);

  g_assert (GTK_IS_BUTTON (self->color_button));
  switch (self->type)
  {
    case BJB_TOOLBAR_SELECT:
    case BJB_TOOLBAR_TRASH_SELECT:
      populate_bar_for_selection (self);
      break;

    case BJB_TOOLBAR_STD_ICON:
      populate_bar_for_standard(self);
      gtk_widget_show (self->list_button);
      update_selection_buttons (self->controller,
                                bjb_controller_shows_item (self->controller),
                                TRUE,
                                self);
      break;

    case BJB_TOOLBAR_STD_LIST:
      populate_bar_for_standard(self);
      gtk_widget_show (self->grid_button);
      update_selection_buttons (self->controller,
                                bjb_controller_shows_item (self->controller),
                                TRUE,
                                self);
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
      break;
  }
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
    /* bjb_main_toolbar_clear (self); */


    if (self->search_handler != 0)
    {
      g_signal_handler_disconnect (self->controller, self->search_handler);
      self->search_handler = 0;
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
  gtk_widget_add_accelerator (self->new_button, "clicked", self->accel, GDK_KEY_n,
                              GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_add_accelerator (self->search_button, "clicked", self->accel, GDK_KEY_f,
                              GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  g_signal_connect_swapped (self->window, "view-changed",
                            G_CALLBACK (populate_main_toolbar), self);

  g_object_bind_property (self->search_button,
                          "active",
                          bjb_window_base_get_search_bar (BJB_WINDOW_BASE (self->window)),
                          "search-mode-enabled",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  G_OBJECT_CLASS(bjb_main_toolbar_parent_class)->constructed(obj);
}

static void
bjb_main_toolbar_init (BjbMainToolbar *self)
{
  self->type = BJB_TOOLBAR_0;
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bjb_main_toolbar_finalize (GObject *object)
{
  BjbMainToolbar *self = BJB_MAIN_TOOLBAR(object);

  if (self->search_handler != 0)
    g_signal_handler_disconnect (self->controller, self->search_handler);

  if (self->display_notes != 0)
      g_signal_handler_disconnect (self->controller, self->display_notes);

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
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

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

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/main-toolbar.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, button_stack);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, new_button);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, back_button);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, title_entry);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, list_button);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, grid_button);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, search_button);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, empty_button);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, style_buttons);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, cancel_button);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, select_button);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, color_button);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, main_button);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, menu_button);

  /* Menu items */
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, new_window_item);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, undo_item);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, redo_item);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, trash_item);
  gtk_widget_class_bind_template_child (widget_class, BjbMainToolbar, last_update_item);

  gtk_widget_class_bind_template_callback (widget_class, on_new_note_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_selection_mode_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_back_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_view_mode_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_empty_clicked_callback);
  gtk_widget_class_bind_template_callback (widget_class, on_color_button_clicked);

  gtk_widget_class_bind_template_callback (widget_class, on_title_changed);
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

  self->display_notes = g_signal_connect (self->controller,
                                          "display-items-changed",
                                          G_CALLBACK (update_selection_buttons),
                                          self);
  populate_main_toolbar(self);
  return self;
}

void
bjb_main_toolbar_title_focus (BjbMainToolbar *self)
{
  gtk_entry_grab_focus_without_selecting (GTK_ENTRY (self->title_entry));
}
