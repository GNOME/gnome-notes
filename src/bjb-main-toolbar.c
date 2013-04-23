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

#include <glib/gi18n.h>

#include "bjb-bijiben.h"
#include "bjb-color-button.h"
#include "bjb-main-toolbar.h"
#include "bjb-note-tag-dialog.h"
#include "bjb-rename-note.h"
#include "bjb-share.h"
#include "bjb-window-base.h"
#include "utils/bjb-icons-colors.h"

/* All needed toolbars */
typedef enum
{
  BJB_TOOLBAR_0,
  BJB_TOOLBAR_STD_LIST,
  BJB_TOOLBAR_STD_ICON,
  BJB_TOOLBAR_SELECT,
  BJB_TOOLBAR_NOTE_VIEW,
  BJB_TOOLBAR_NUM
} BjbToolbarType;

/* Color Button */
#define COLOR_SIZE 24

struct _BjbMainToolbarPrivate
{
  /* Controllers */
  BjbToolbarType    type;
  BjbMainView      *parent;
  BjbController    *controller;
  GtkWindow        *window;

  /* Main View */
  GtkWidget        *new;
  GtkWidget        *list;
  GtkWidget        *grid;
  GtkWidget        *select;
  GtkWidget        *search;
  gulong            finish_sig;
  gulong            update_selection;
  gulong            search_handler;
  gulong            display_notes;

  /* When note view */
  BijiNoteObj      *note;
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

#define BJB_MAIN_TOOLBAR_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BJB_TYPE_MAIN_TOOLBAR, BjbMainToolbarPrivate))

G_DEFINE_TYPE (BjbMainToolbar, bjb_main_toolbar, GD_TYPE_MAIN_TOOLBAR);

/* Callbacks */

static void
on_new_note_clicked (GtkWidget *but, BjbMainView *view)
{
  BijiNoteObj *result ;
  BijiNoteBook *book ;

  /* append note to collection */
  book = bjb_window_base_get_book (bjb_main_view_get_window (view));
  result = biji_note_book_get_new_note_from_string (book, "");

  /* Go to that note */
  switch_to_note_view(view,result);
}

static void populate_main_toolbar(BjbMainToolbar *self);

static gboolean
update_selection_label (BjbMainToolbar *self)
{
  GList *selected;
  gint length;
  gchar *label;

  selected = bjb_main_view_get_selection (self->priv->parent);
  length = g_list_length (selected);
  g_list_free (selected);

  if (length == 0)
    label = g_strdup(_("Click on items to select them"));
  else
    label = g_strdup_printf (g_dngettext (GETTEXT_PACKAGE, "%d selected", "%d selected", length),length);

  gd_main_toolbar_set_labels (GD_MAIN_TOOLBAR (self), NULL, label);
  g_free (label);

  return TRUE;
}

void
on_view_selection_changed_cb (BjbMainToolbar *self)
{
  GtkStyleContext *context;
  GtkWidget *widget = GTK_WIDGET (self);
  context = gtk_widget_get_style_context (widget);

  g_return_if_fail (BJB_IS_MAIN_TOOLBAR (self));

  if (!bjb_main_view_get_selection_mode (self->priv->parent))
    gtk_style_context_remove_class (context, "selection-mode");

  else
    gtk_style_context_add_class (context, "selection-mode");

  gtk_widget_reset_style (widget);
  populate_main_toolbar (self);

  if (self->priv->type == BJB_TOOLBAR_SELECT)
    update_selection_label (self);

  return;
}

static void
on_selection_mode_clicked (GtkWidget *button, BjbMainToolbar *self)
{
  if (bjb_main_view_get_selection_mode (self->priv->parent))
    bjb_main_view_set_selection_mode (self->priv->parent, FALSE);

  else
    bjb_main_view_set_selection_mode (self->priv->parent, TRUE);
}

static gboolean
on_view_mode_clicked (GtkWidget *button, BjbMainToolbar *self)
{
  GdMainViewType current = bjb_main_view_get_view_type (self->priv->parent);
    
  switch ( current )
  {
    case GD_MAIN_VIEW_ICON :
      bjb_main_view_set_view_type (self->priv->parent ,GD_MAIN_VIEW_LIST );
      break ;
    case GD_MAIN_VIEW_LIST :
      bjb_main_view_set_view_type (self->priv->parent, GD_MAIN_VIEW_ICON );
      break ;
    default:
      bjb_main_view_set_view_type (self->priv->parent, GD_MAIN_VIEW_ICON );
  }

  bjb_main_view_update_model (self->priv->parent);
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
on_search_button_clicked (BjbMainToolbarPrivate *priv)
{
  BjbSearchToolbar *bar;

  bar = bjb_main_view_get_search_toolbar (priv->parent);

  if (bar == NULL)
    return;

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->search)))
    bjb_search_toolbar_fade_in (bar);

  else
    bjb_search_toolbar_fade_out (bar);
}

static void
add_search_button (BjbMainToolbar *self)
{
  BjbMainToolbarPrivate *priv = self->priv;

  priv->search = gd_main_toolbar_add_toggle (GD_MAIN_TOOLBAR (self),
                                             "edit-find-symbolic",
                                             NULL,
                                             FALSE);

  g_signal_connect_swapped (priv->search, "clicked",
                            G_CALLBACK (on_search_button_clicked), priv);
}

static void
update_selection_buttons (BjbMainToolbarPrivate *priv)
{
  gboolean some_note_is_visible = bjb_controller_shows_notes (priv->controller);

  if (priv->grid)
    gtk_widget_set_sensitive (priv->grid, some_note_is_visible);

  if (priv->list)
    gtk_widget_set_sensitive (priv->list, some_note_is_visible);

  gtk_widget_set_sensitive (priv->select, some_note_is_visible);
}

static void
populate_bar_for_selection (BjbMainToolbar *self)
{
  BjbMainToolbarPrivate *priv = self->priv;
  GtkStyleContext *context;

  /* Search button */
  add_search_button (self);

  /* Select */
  priv->select = gd_main_toolbar_add_button (GD_MAIN_TOOLBAR (self),
                                             NULL,"Done", FALSE);
  context = gtk_widget_get_style_context (priv->select);
  gtk_style_context_add_class (context, "suggested-action");
  gtk_widget_reset_style (priv->select);

  g_signal_connect (priv->select, "clicked",
                    G_CALLBACK (on_selection_mode_clicked), self);

  update_selection_label (self);
}

static void
update_label_for_standard (BjbMainToolbar *self)
{
  BjbMainToolbarPrivate *priv = self->priv;
  gchar *needle = bjb_controller_get_needle (priv->controller);
  gchar *label ;

  if (needle && g_strcmp0 (needle, "") !=0)
    label = g_strdup_printf (_("Results for %s"), needle);

  else
    label = g_strdup (_("New and Recent"));

  gd_main_toolbar_set_labels (GD_MAIN_TOOLBAR (self), label, NULL);
  g_free (label);

  self->priv->display_notes = g_signal_connect_swapped (self->priv->controller,
                                                        "display-notes-changed",
                                                        G_CALLBACK (update_selection_buttons),
                                                        self->priv);
}

static void
populate_bar_for_standard(BjbMainToolbar *self)
{
  BjbMainToolbarPrivate *priv = self->priv;
  GtkWidget *bin = NULL;

  /* Label */
  update_label_for_standard (self);
  priv->search_handler = g_signal_connect_swapped (priv->controller,
         "search-changed", G_CALLBACK(update_label_for_standard), self);

  /* New Note button */
  priv->new = gd_main_toolbar_add_button(GD_MAIN_TOOLBAR (self),
                                         NULL,
                                         _("New"),
                                         TRUE);
  gtk_widget_set_size_request (priv->new, 70, -1);
  bin = gtk_bin_get_child (GTK_BIN (priv->new));

  if (bin)
  {
    gint y_padding = 0;
    gtk_misc_get_padding (GTK_MISC (bin), NULL, &y_padding);
    gtk_misc_set_padding (GTK_MISC (bin), 12, y_padding);
  }

  g_signal_connect(priv->new,"clicked",
                   G_CALLBACK(on_new_note_clicked),priv->parent);

  /* Go to selection mode */
  priv->select = gd_main_toolbar_add_button(GD_MAIN_TOOLBAR (self),
                                            "object-select-symbolic",
                                            NULL,
                                            FALSE);

  g_signal_connect (priv->select,"clicked",
                    G_CALLBACK(on_selection_mode_clicked),self);
}

static void
populate_bar_for_icon_view(BjbMainToolbar *self)
{
  BjbMainToolbarPrivate *priv = self->priv;

  /* Switch to list */
  priv->grid = NULL;
  priv->list= gd_main_toolbar_add_button(GD_MAIN_TOOLBAR (self),
                                         "view-list-symbolic",
                                         NULL,
                                         FALSE);

  g_signal_connect (priv->list, "clicked",
                    G_CALLBACK(on_view_mode_clicked),self);

  populate_bar_for_standard(self);
}

static void
populate_bar_for_list_view(BjbMainToolbar *self)
{
  BjbMainToolbarPrivate *priv = self->priv;

  /* Switch to icon view */
  priv->list = NULL;
  priv->grid = gd_main_toolbar_add_button(GD_MAIN_TOOLBAR (self),
                                          "view-grid-symbolic",
                                          NULL,
                                          FALSE);

  g_signal_connect (priv->grid, "clicked",
                    G_CALLBACK(on_view_mode_clicked),self);

  populate_bar_for_standard(self);
}


static void
disconnect_note_handlers (BjbMainToolbarPrivate *priv)
{
  if (priv->accel)
    gtk_window_remove_accel_group (priv->window, priv->accel);

  if (priv->note_renamed != 0)
  {
    g_signal_handler_disconnect (priv->note, priv->note_renamed);
    priv->note_renamed = 0;
  }

  if (priv->note_color_changed != 0)
  {
    g_signal_handler_disconnect (priv->note, priv->note_color_changed);
    priv->note_color_changed = 0;
  }

  priv->note = NULL;
}

static void
just_switch_to_main_view (BjbMainToolbar *self)
{
  disconnect_note_handlers (self->priv);
  bjb_window_base_switch_to (BJB_WINDOW_BASE (self->priv->window),
                             BJB_WINDOW_BASE_MAIN_VIEW);
}

static void
on_note_renamed (BijiNoteObj *note,
                 BjbMainToolbar *self)
{
  gd_main_toolbar_set_labels (
            GD_MAIN_TOOLBAR (self),
            biji_note_obj_get_title (note),
            NULL);
}

static void
on_color_button_clicked (GtkColorButton *button,
                         BjbMainToolbar *bar)
{
  GdkRGBA color;

  if (!bar->priv->note)
    return;

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
  biji_note_obj_set_rgba (bar->priv->note, &color);
}

static void
on_note_color_changed (BijiNoteObj *note, GtkColorButton *button)
{
  GdkRGBA color;

  if (biji_note_obj_get_rgba (note, &color))
    gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (button), &color);
}

static void
bjb_toggle_bullets (BijiWebkitEditor *editor)
{
  biji_webkit_editor_apply_format (editor, BIJI_BULLET_LIST);
}

static void
bjb_toggle_list (BijiWebkitEditor *editor)
{
  biji_webkit_editor_apply_format (editor, BIJI_ORDER_LIST);
}

static void
action_view_tags_callback (GtkWidget *item, gpointer user_data)
{
  BjbMainToolbar *self = BJB_MAIN_TOOLBAR (user_data);
  GList *list = NULL;

  list = g_list_append (list, self->priv->note);
  bjb_note_tag_dialog_new (self->priv->window, list);
  g_list_free (list);
}

static void
delete_item_callback (GtkWidget *item, gpointer user_data)
{
  BjbMainToolbar *self = BJB_MAIN_TOOLBAR (user_data);

  /* Delete the note from collection
   * The deleted note will emit a signal. */
  biji_note_book_remove_note (
          bjb_window_base_get_book (GTK_WIDGET (self->priv->window)),
          self->priv->note);
}

static void
action_rename_note_callback (GtkWidget *item, gpointer user_data)
{
  BjbMainToolbar        *bar;
  BjbMainToolbarPrivate *priv;
  gchar                 *title;

  bar = BJB_MAIN_TOOLBAR (user_data);
  priv = bar->priv;

  title = note_title_dialog (priv->window,
                             _("Rename Note"),
                             biji_note_obj_get_title (priv->note));

  if (!title)
    return;

  biji_note_obj_set_title (priv->note, title);
  biji_note_obj_save_note (priv->note); //FIXME libbiji needs to to this auto
}

GtkWidget *
bjb_note_menu_new (BjbMainToolbar *self)
{
  BjbMainToolbarPrivate *priv = self->priv;
  GtkWidget             *result, *item;
  BijiWebkitEditor      *editor;

  result = gtk_menu_new();
  editor = BIJI_WEBKIT_EDITOR (biji_note_obj_get_editor (priv->note));

  /* Undo Redo separator */
  item = gtk_menu_item_new_with_label (_("Undo"));
  gtk_menu_shell_append (GTK_MENU_SHELL (result), item);
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (webkit_web_view_undo), editor);
  gtk_widget_add_accelerator (item, "activate", priv->accel, GDK_KEY_u,
                             GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_show (item);

  item = gtk_menu_item_new_with_label (_("Redo"));
  gtk_menu_shell_append (GTK_MENU_SHELL (result), item);
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (webkit_web_view_redo), editor);
  gtk_widget_add_accelerator (item, "activate", priv->accel, GDK_KEY_r,
                             GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
  gtk_widget_show (item);

  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (result), item);
  gtk_widget_show (item);

  /* Bullets, ordered list, separator */
  /* Bullets : unordered list format */
  item = gtk_menu_item_new_with_label (_("Bullets"));
  gtk_menu_shell_append (GTK_MENU_SHELL (result), item);
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (bjb_toggle_bullets), editor);
  gtk_widget_show (item);

  /* Ordered list as 1.mouse 2.cats 3.dogs */
  item = gtk_menu_item_new_with_label (_("List"));
  gtk_menu_shell_append (GTK_MENU_SHELL (result), item);
  g_signal_connect_swapped (item, "activate",
                            G_CALLBACK (bjb_toggle_list), editor);
  gtk_widget_show(item);

  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (result), item);
  gtk_widget_show (item);

  /* Rename, view tags, separtor */
  item = gtk_menu_item_new_with_label(_("Rename"));
  gtk_menu_shell_append(GTK_MENU_SHELL(result),item);
  g_signal_connect(item,"activate",
                   G_CALLBACK(action_rename_note_callback),self);
  gtk_widget_show(item);

  item = gtk_menu_item_new_with_label(_("Tags"));
  gtk_menu_shell_append(GTK_MENU_SHELL(result),item);
  g_signal_connect(item,"activate",
                   G_CALLBACK(action_view_tags_callback),self);
  gtk_widget_show(item);

  item = gtk_separator_menu_item_new ();
  gtk_menu_shell_append (GTK_MENU_SHELL (result), item);
  gtk_widget_show (item);

  /* Delete Note */
  item = gtk_menu_item_new_with_label(_("Delete this Note"));
  gtk_menu_shell_append(GTK_MENU_SHELL(result),item);
  g_signal_connect(item,"activate",
                   G_CALLBACK(delete_item_callback),self);
  gtk_widget_show(item);

  return result;
}

static void
populate_bar_for_note_view (BjbMainToolbar *self)
{
  BjbMainToolbarPrivate *priv = self->priv;
  GdMainToolbar         *bar = GD_MAIN_TOOLBAR (self);
  BjbSettings           *settings;
  GtkWidget             *grid, *notes_icon, *notes_label, *button;
  GdkRGBA                color;

  priv->note = bjb_window_base_get_note (BJB_WINDOW_BASE (self->priv->window));

  if (!priv->note) /* no reason this would happen */
    return;

  settings = bjb_app_get_settings (g_application_get_default());
  priv->accel = gtk_accel_group_new ();
  gtk_window_add_accel_group (priv->window, priv->accel);

  /* Go to main view basically means closing note */
  grid = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
  notes_icon = get_icon ("go-previous-symbolic");
  gtk_box_pack_start (GTK_BOX (grid), notes_icon, TRUE, TRUE, TRUE);

  notes_label = gtk_label_new (_("Notes"));
  gtk_box_pack_start (GTK_BOX (grid), notes_label, TRUE, TRUE, TRUE);

  button = gd_main_toolbar_add_button (bar, NULL, NULL, TRUE);

  gtk_container_add (GTK_CONTAINER (button), grid);

  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (just_switch_to_main_view), self);
  gtk_widget_add_accelerator (button, "activate", self->priv->accel,
                              GDK_KEY_w, GDK_CONTROL_MASK, GTK_ACCEL_MASK);

  /* Note Title */
  gd_main_toolbar_set_labels (bar, biji_note_obj_get_title (priv->note), NULL);

  self->priv->note_renamed = g_signal_connect (priv->note,"renamed",
                                    G_CALLBACK (on_note_renamed), self);


  /* Note Color */
  if (!biji_note_obj_get_rgba (priv->note, &color))
  {
    gchar *default_color;

    g_object_get (G_OBJECT(settings),"color", &default_color, NULL);
    gdk_rgba_parse (&color, default_color);
    g_free (default_color);
  }

  button = bjb_color_button_new ();
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (button), &color);

  gd_main_toolbar_add_widget (bar, button, FALSE);
  gtk_widget_set_size_request (gtk_bin_get_child (GTK_BIN (button)),
                               COLOR_SIZE, COLOR_SIZE);
  gtk_widget_show (button);

  g_signal_connect (button, "color-set",
                    G_CALLBACK (on_color_button_clicked), self);
  priv->note_color_changed = g_signal_connect (priv->note, "color-changed",
                             G_CALLBACK (on_note_color_changed), button);

  /* Sharing */
  button = gd_main_toolbar_add_button (bar, "send-to-symbolic",
                                       NULL, FALSE);

  g_signal_connect (button, "clicked",
                    G_CALLBACK (on_email_note_callback), priv->note);

  /* Menu */
  button = gd_main_toolbar_add_menu (bar,
                                     "emblem-system-symbolic",
                                     NULL,
                                     FALSE);

  gtk_menu_button_set_popup (GTK_MENU_BUTTON (button),
                             bjb_note_menu_new (self));
}

static void
populate_bar_switch(BjbMainToolbar *self)
{
  switch (self->priv->type)
  {
    case BJB_TOOLBAR_SELECT:
      populate_bar_for_selection (self);
      break;

    case BJB_TOOLBAR_STD_ICON:
      add_search_button (self);
      populate_bar_for_icon_view(self);
      update_selection_buttons (self->priv);
      break;

    case BJB_TOOLBAR_STD_LIST:
      add_search_button (self);
      populate_bar_for_list_view(self);
      update_selection_buttons (self->priv);
      break;

    case BJB_TOOLBAR_NOTE_VIEW:
      populate_bar_for_note_view (self);
      break;

    default:
      g_warning ("Main Toolbar implementation is erroneous.\
                 Please fill in a bug report");
  }

  gtk_widget_show_all (GTK_WIDGET (self));
}

static void
populate_main_toolbar(BjbMainToolbar *self)
{
  BjbMainToolbarPrivate *priv = self->priv;
  BjbToolbarType to_be = BJB_TOOLBAR_0 ;
  BjbWindowViewType view_type;

  view_type = bjb_window_base_get_view_type (BJB_WINDOW_BASE (priv->window));

  if (view_type == BJB_WINDOW_BASE_NOTE_VIEW)
    to_be = BJB_TOOLBAR_NOTE_VIEW;

  else if (bjb_main_view_get_selection_mode (priv->parent) == TRUE)
    to_be = BJB_TOOLBAR_SELECT;

  else if (bjb_main_view_get_view_type (priv->parent) == GD_MAIN_VIEW_ICON)
    to_be = BJB_TOOLBAR_STD_ICON;

  else if (bjb_main_view_get_view_type (priv->parent) == GD_MAIN_VIEW_LIST)
    to_be = BJB_TOOLBAR_STD_LIST;

  /* Simply clear then populate */
  if (to_be != priv->type)
  {
    /* If we leave a note view */
    if (priv->type == BJB_TOOLBAR_NOTE_VIEW)
      disconnect_note_handlers (priv);

    priv->type = to_be;
    gd_main_toolbar_clear (GD_MAIN_TOOLBAR (self));

    if (priv->search_handler != 0)
    {
      g_signal_handler_disconnect (priv->controller, priv->search_handler);
      priv->search_handler = 0;
    }

    if (priv->display_notes != 0)
    {
      g_signal_handler_disconnect (priv->controller, priv->display_notes);
      priv->display_notes = 0;
    }

    populate_bar_switch (self);
  }
}

static void
bjb_main_toolbar_constructed (GObject *obj)
{
  BjbMainToolbar *self = BJB_MAIN_TOOLBAR (obj);

  g_signal_connect_swapped (self->priv->window, "view-changed",
                            G_CALLBACK (populate_main_toolbar), self);

  g_signal_connect_swapped (self->priv->parent, "view-selection-changed",
                            G_CALLBACK (on_view_selection_changed_cb), self);

  G_OBJECT_CLASS(bjb_main_toolbar_parent_class)->constructed(obj);
}

static void
bjb_main_toolbar_init (BjbMainToolbar *self)
{
  self->priv = BJB_MAIN_TOOLBAR_GET_PRIVATE(self);
  BjbMainToolbarPrivate *priv = self->priv;

  priv->type = BJB_TOOLBAR_0 ;

  priv->grid = NULL;
  priv->list = NULL;
  priv->search = NULL;
  priv->search_handler = 0;
  priv->display_notes = 0;

  priv->accel = NULL;
  priv->note = NULL;
  priv->note_renamed = 0;
  priv->note_color_changed = 0;

  g_signal_connect (self, "button-press-event", G_CALLBACK (on_button_press), NULL);
}

static void
bjb_main_toolbar_finalize (GObject *object)
{
  BjbMainToolbar *self = BJB_MAIN_TOOLBAR(object);
  BjbMainToolbarPrivate *priv = self->priv;

  if (priv->search_handler != 0)
  {
    g_signal_handler_disconnect (priv->controller, priv->search_handler);
    priv->search_handler = 0;
  }

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
      g_value_set_object (value, self->priv->parent);
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
      self->priv->parent = g_value_get_object(value);
      self->priv->window = GTK_WINDOW (
                       bjb_main_view_get_window (self->priv->parent));
      break;
    case PROP_CONTROLLER:
      self->priv->controller = g_value_get_object (value);
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

  g_type_class_add_private (klass, sizeof (BjbMainToolbarPrivate));

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
  /* Since heriting GdMainToolbar, we populate bar _after_ construct */
  BjbMainToolbar *self;

  self = BJB_MAIN_TOOLBAR (g_object_new (BJB_TYPE_MAIN_TOOLBAR,
                                         "controller", controller,
                                         "parent", parent,
                                         NULL));

  populate_main_toolbar(self);
  return self;
}

void
bjb_main_toolbar_set_search_toggle_state (BjbMainToolbar *self,
                                          gboolean active)
{
  g_return_if_fail (BJB_IS_MAIN_TOOLBAR (self));

  if (self->priv->search)
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->search), active);
}
