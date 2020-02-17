/* bjb-selection-toolbar.c
 * Copyright © 2012, 2013 Red Hat, Inc.
 * Copyright © 2013 Pierre-Yves LUYTEN <py@luyten.fr>
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

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "bjb-application.h"
#include "bjb-color-button.h"
#include <bjb-list-view.h>
#include <bjb-main-view.h>
#include "bjb-organize-dialog.h"
#include "bjb-selection-toolbar.h"
#include "bjb-share.h"
#include "bjb-window-base.h"

enum
{
  PROP_0,
  PROP_BJB_SELECTION,
  PROP_BJB_MAIN_VIEW,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

struct _BjbSelectionToolbar
{
  GtkRevealer parent_instance;

  BjbMainView *view;
  BjbListView *selection;

  GtkWidget   *button_stack;
  GtkWidget   *notebook_button;
  GtkWidget   *detach_button;
  GtkWidget   *color_button;
  GtkWidget   *share_button;
};

G_DEFINE_TYPE (BjbSelectionToolbar, bjb_selection_toolbar, GTK_TYPE_REVEALER)

static void
action_color_selected_items (GtkWidget           *w,
                             BjbSelectionToolbar *self)
{
  GList *l, *selection;
  GdkRGBA color = {0,0,0,0};

  g_assert (GTK_IS_COLOR_CHOOSER (w));
  g_assert (BJB_IS_SELECTION_TOOLBAR (self));

  gtk_widget_set_visible (GTK_WIDGET (self), TRUE);
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (w), &color);
  selection = bjb_main_view_get_selected_items (self->view);

  for (l=selection; l !=NULL; l=l->next)
  {
    if (BIJI_IS_NOTE_OBJ (l->data))
      biji_note_obj_set_rgba (l->data, &color);
  }

  bjb_main_view_set_selection_mode (self->view, FALSE);
  g_list_free (selection);
}

static void
action_tag_selected_items (BjbSelectionToolbar *self)
{
  GList *selection;

  g_assert (BJB_IS_SELECTION_TOOLBAR (self));

  selection = bjb_main_view_get_selected_items (self->view);
  bjb_organize_dialog_new
    (GTK_WINDOW (bjb_main_view_get_window (self->view)), selection);

  bjb_main_view_set_selection_mode (self->view, FALSE);
  g_list_free (selection);
}

static void
action_trash_selected_items (BjbSelectionToolbar *self)
{
  GList *l, *selection;

  g_assert (BJB_IS_SELECTION_TOOLBAR (self));

  selection = bjb_main_view_get_selected_items (self->view);
  for (l=selection; l !=NULL; l=l->next)
    biji_item_trash (BIJI_ITEM (l->data));

  /*
   * HACK: When every item is deleted, Empty view is shown in the
   * main window. Thus the action bar is hidden in the view.
   * But it actually isn't. And the bar appears when the view
   * is changed to Trash view.
   * So hide the widget all together.
   */
  gtk_widget_hide (GTK_WIDGET (self));
  gtk_revealer_set_reveal_child (GTK_REVEALER (self), FALSE);
  bjb_main_view_set_selection_mode (self->view, FALSE);
  g_list_free (selection);
}

static void
action_pop_up_note_callback (BjbSelectionToolbar *self)
{
  GList *l, *selection;

  g_assert (BJB_IS_SELECTION_TOOLBAR (self));

  selection = bjb_main_view_get_selected_items (self->view);
  for (l=selection; l !=NULL; l=l->next)
  {
    bijiben_new_window_for_note (g_application_get_default (),
                                 BIJI_NOTE_OBJ (l->data));
  }

  bjb_main_view_set_selection_mode (self->view, FALSE);
  g_list_free (selection);
}

static void
action_share_item_callback (BjbSelectionToolbar *self,
                            GtkButton           *button)
{
  GList *l, *selection;

  g_assert (BJB_IS_SELECTION_TOOLBAR (self));
  g_assert (GTK_IS_BUTTON (button));

  selection = bjb_main_view_get_selected_items (self->view);

  for (l = selection; l != NULL; l = l->next)
    {
      on_email_note_callback (l->data);
    }

  bjb_main_view_set_selection_mode (self->view, FALSE);
  g_list_free (selection);
}

static void
on_restore_clicked_callback (BjbSelectionToolbar *self)
{
  GList *selection, *l;

  g_assert (BJB_IS_SELECTION_TOOLBAR (self));

  selection = bjb_main_view_get_selected_items (self->view);

  for (l=selection; l!=NULL; l=l->next)
    biji_item_restore (BIJI_ITEM (l->data));

  bjb_main_view_set_selection_mode (self->view, FALSE);
  g_list_free (selection);
}

static void
on_delete_clicked_callback (BjbSelectionToolbar *self)
{
  GList *selection, *l;

  g_assert (BJB_IS_SELECTION_TOOLBAR (self));

  selection = bjb_main_view_get_selected_items (self->view);
  for (l=selection; l!=NULL; l=l->next)
    biji_item_delete (BIJI_ITEM (l->data));

  bjb_main_view_set_selection_mode (self->view, FALSE);
  g_list_free (selection);
}

static void
set_sensitivity (BjbSelectionToolbar *self)
{
  GList *l, *selection;
  GdkRGBA color;
  gboolean can_tag, can_color, can_share, are_notes;

  g_assert (BJB_IS_SELECTION_TOOLBAR (self));

  selection = bjb_main_view_get_selected_items (self->view);

  /* Default */
  can_color = TRUE;
  can_tag = TRUE;
  can_share = TRUE;
  are_notes = TRUE;


  /* Adapt */
  for (l=selection; l !=NULL; l=l->next)
  {
    if (can_tag == TRUE) /* tag is default. check if still applies */
    {
      if (!biji_item_is_collectable (l->data))
        can_tag = FALSE;
    }

   if (are_notes == FALSE || (BIJI_IS_NOTE_OBJ (l->data)) == FALSE)
     are_notes = FALSE;


    if (can_color == TRUE) /* color is default. check */
    {
      if (!BIJI_IS_NOTE_OBJ (l->data)
          || !biji_item_has_color (l->data)
          || !biji_note_obj_get_rgba (BIJI_NOTE_OBJ (l->data), &color))
        can_color = FALSE;

     else
       gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (self->color_button), &color);
    }

    if (can_share == TRUE) /* share is default. check. */
    {
      if (!BIJI_IS_NOTE_OBJ (l->data))
        can_share = FALSE;
    }
  }

  gtk_widget_set_sensitive (self->notebook_button, can_tag);
  gtk_widget_set_sensitive (self->detach_button, are_notes);
  gtk_widget_set_sensitive (self->color_button, can_color);
  gtk_widget_set_sensitive (self->share_button, can_share);

  g_list_free (selection);
}

static void
bjb_selection_toolbar_set_item_visibility (BjbSelectionToolbar *self)
{
  BijiItemsGroup group;

  g_assert (BJB_IS_SELECTION_TOOLBAR (self));

  group = bjb_controller_get_group (
            bjb_window_base_get_controller (
              BJB_WINDOW_BASE (
                bjb_main_view_get_window (self->view))));

  if (group == BIJI_LIVING_ITEMS)
    {
      gtk_stack_set_visible_child_name (GTK_STACK (self->button_stack), "main");
      set_sensitivity (self);
    }
  else if (group == BIJI_ARCHIVED_ITEMS)
    {
      gtk_stack_set_visible_child_name (GTK_STACK (self->button_stack), "trash");
    }
}

static void
bjb_selection_toolbar_fade_in (BjbSelectionToolbar *self)
{
  gtk_revealer_set_reveal_child (GTK_REVEALER (self), TRUE);
  //bjb_selection_toolbar_set_item_visibility
}

static void
bjb_selection_toolbar_fade_out (BjbSelectionToolbar *self)
{
  gtk_revealer_set_reveal_child (GTK_REVEALER (self), FALSE);
}

static void
on_selected_rows_changed_cb (BjbListView *view,
                             gpointer     user_data)
{
  BjbSelectionToolbar *self;
  GList *selection;

  self = BJB_SELECTION_TOOLBAR (user_data);
  selection = bjb_main_view_get_selected_items (self->view);

  if (g_list_length (selection) > 0)
  {
    gtk_widget_show (GTK_WIDGET (self));
    bjb_selection_toolbar_set_item_visibility (self);
    bjb_selection_toolbar_fade_in (self);
  }
  else
    {
      bjb_selection_toolbar_fade_out (self);
    }

  g_list_free (selection);
}

static void
bjb_selection_toolbar_init (BjbSelectionToolbar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bjb_selection_toolbar_get_property (GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
  BjbSelectionToolbar *self = BJB_SELECTION_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_BJB_SELECTION:
      g_value_set_object(value, self->selection);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bjb_selection_toolbar_set_property (GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
  BjbSelectionToolbar *self = BJB_SELECTION_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_BJB_SELECTION:
      self->selection = g_value_get_object (value);
      break;
    case PROP_BJB_MAIN_VIEW:
      self->view = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bjb_selection_toolbar_constructed(GObject *obj)
{
  BjbSelectionToolbar *self = BJB_SELECTION_TOOLBAR(obj);

  G_OBJECT_CLASS (bjb_selection_toolbar_parent_class)->constructed (obj);

  g_signal_connect (bjb_list_view_get_list_box (self->selection),
                    "selected-rows-changed",
                    G_CALLBACK (on_selected_rows_changed_cb),
                    self);
}

static void
bjb_selection_toolbar_class_init (BjbSelectionToolbarClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (class);

  object_class->get_property = bjb_selection_toolbar_get_property;
  object_class->set_property = bjb_selection_toolbar_set_property;
  object_class->constructed = bjb_selection_toolbar_constructed;

  properties[PROP_BJB_SELECTION] = g_param_spec_object ("selection",
                                                        "Selection",
                                                        "SelectionController",
                                                        BJB_TYPE_LIST_VIEW,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class,PROP_BJB_SELECTION,properties[PROP_BJB_SELECTION]);

  properties[PROP_BJB_MAIN_VIEW] = g_param_spec_object ("bjbmainview",
                                                        "Bjbmainview",
                                                        "BjbMainView",
                                                        BJB_TYPE_MAIN_VIEW,
                                                        G_PARAM_READWRITE  |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class,PROP_BJB_MAIN_VIEW,properties[PROP_BJB_MAIN_VIEW]);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/selection-toolbar.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbSelectionToolbar, button_stack);
  gtk_widget_class_bind_template_child (widget_class, BjbSelectionToolbar, notebook_button);
  gtk_widget_class_bind_template_child (widget_class, BjbSelectionToolbar, color_button);
  gtk_widget_class_bind_template_child (widget_class, BjbSelectionToolbar, share_button);
  gtk_widget_class_bind_template_child (widget_class, BjbSelectionToolbar, detach_button);

  gtk_widget_class_bind_template_callback (widget_class, action_tag_selected_items);
  gtk_widget_class_bind_template_callback (widget_class, action_color_selected_items);
  gtk_widget_class_bind_template_callback (widget_class, action_share_item_callback);
  gtk_widget_class_bind_template_callback (widget_class, action_pop_up_note_callback);
  gtk_widget_class_bind_template_callback (widget_class, action_trash_selected_items);
  gtk_widget_class_bind_template_callback (widget_class, on_restore_clicked_callback);
  gtk_widget_class_bind_template_callback (widget_class, on_delete_clicked_callback);
}


BjbSelectionToolbar *
bjb_selection_toolbar_new (BjbListView  *selection,
                           BjbMainView  *bjb_main_view)
{
  return g_object_new (BJB_TYPE_SELECTION_TOOLBAR,
                       "selection", selection,
                       "bjbmainview",bjb_main_view,
                       NULL);
}
