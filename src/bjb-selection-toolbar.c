/* bjb-selection-toolbar.c
 * Copyright © 2012, 2013 Red Hat, Inc.
 * Copyright © 2013 Pierre-Yves LUYTEN <py@luyten.fr>
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
#include <libgd/gd.h>

#include "bjb-color-button.h"
#include "bjb-note-tag-dialog.h"
#include "bjb-main-view.h"
#include "bjb-selection-toolbar.h"
#include "bjb-window-base.h"

enum
{
  PROP_0,
  PROP_BJB_SELECTION,
  PROP_BJB_MAIN_VIEW,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

struct _BjbSelectionToolbarPrivate
{
  GtkWidget          *toolbar_trash;
  GtkWidget          *toolbar_color;
  GtkWidget          *toolbar_tag;

  /* sure */
  BjbMainView        *view ;
  GtkWidget          *widget ;
  GdMainView         *selection ;

  /* misc gtk */
  GtkToolItem        *left_group;
  GtkToolItem        *right_group;
  GtkToolItem        *separator;
  GtkWidget          *left_box;
  GtkWidget          *right_box;
};

G_DEFINE_TYPE (BjbSelectionToolbar, bjb_selection_toolbar, GTK_TYPE_TOOLBAR);


/*
 * Color dialog is transient and could damage the display of self
 * We do not want a modal window since the app may have several
 * The fix is to hide self untill dialog has run
 *
 */
static void
hide_self (GtkWidget *self)
{
  gtk_widget_set_visible (self, FALSE);
}


static void
action_color_selected_items (GtkWidget *w, BjbSelectionToolbar *self)
{
  GList *l, *selection;
  GdkRGBA color = {0,0,0,0};

  gtk_widget_set_visible (GTK_WIDGET (self), TRUE);
  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (w), &color);
  selection = bjb_main_view_get_selected_items (self->priv->view);

  for (l=selection; l !=NULL; l=l->next)
  {
    if (BIJI_IS_NOTE_OBJ (l->data))
      biji_note_obj_set_rgba (l->data, &color);
  }


  g_list_free (selection);
}


static void
action_tag_selected_items (GtkWidget *w, BjbSelectionToolbar *self)
{
  GList *selection;

  selection = bjb_main_view_get_selected_items (self->priv->view);
  bjb_note_tag_dialog_new
    (GTK_WINDOW (bjb_main_view_get_window (self->priv->view)), selection);

  g_list_free (selection);
}


static void
action_delete_selected_items (GtkWidget *w, BjbSelectionToolbar *self)
{
  GList *l, *selection;
  BijiNoteBook *book;

  selection = bjb_main_view_get_selected_items (self->priv->view);
  book = bjb_window_base_get_book (bjb_main_view_get_window (self->priv->view));

  for (l=selection; l !=NULL; l=l->next)
  {
    biji_note_book_remove_item (book, BIJI_ITEM (l->data));
  }

  g_list_free (selection);
}


static void
bjb_selection_toolbar_fade_in (BjbSelectionToolbar *self)
{
  gtk_widget_set_opacity (self->priv->widget, 1);
}


static void
bjb_selection_toolbar_fade_out (BjbSelectionToolbar *self)
{
  gtk_widget_set_opacity (self->priv->widget, 0);
}


static void
bjb_selection_toolbar_set_item_visibility (BjbSelectionToolbar *self)
{
  BjbSelectionToolbarPrivate *priv;
  GList *l, *selection;
  GdkRGBA color;

  g_return_if_fail (BJB_IS_SELECTION_TOOLBAR (self));

  priv = self->priv;
  selection = bjb_main_view_get_selected_items (priv->view);

  /* Trash, always */
  gtk_widget_set_visible (priv->toolbar_trash, TRUE);


  /* Color */
  gtk_widget_set_visible (priv->toolbar_color, FALSE);


  for (l=selection; l !=NULL; l=l->next)
  {
    if (!biji_item_has_color (l->data))
    {
      gtk_widget_set_visible (priv->toolbar_color, FALSE);
      break;
    }

    else if (BIJI_IS_NOTE_OBJ (l->data))
    {
      if (biji_note_obj_get_rgba (BIJI_NOTE_OBJ (l->data), &color))
      {
        gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (priv->toolbar_color), &color);
        gtk_widget_set_visible (priv->toolbar_color, TRUE);
        break;
      }
    }
  }


  /* Organize */
  gtk_widget_set_visible (priv->toolbar_tag, TRUE);

  for (l=selection; l!=NULL; l=l->next)
  {
    if (!biji_item_is_collectable (l->data))
    {
      gtk_widget_set_visible (priv->toolbar_tag, FALSE);
      break;
    }
  }

  g_list_free (selection);
}

static void
bjb_selection_toolbar_selection_changed (GdMainView *view, gpointer user_data)
{
  
  BjbSelectionToolbar *self = BJB_SELECTION_TOOLBAR (user_data);
  GList *selection;

  selection = gd_main_view_get_selection(view);

  if (g_list_length (selection) > 0)
  {
    bjb_selection_toolbar_set_item_visibility (self);
    bjb_selection_toolbar_fade_in (self);
  }
  
  else
    bjb_selection_toolbar_fade_out (self);
}

static void
bjb_selection_toolbar_dispose (GObject *object)
{
  G_OBJECT_CLASS (bjb_selection_toolbar_parent_class)->dispose (object);
}


static void
bjb_selection_toolbar_init (BjbSelectionToolbar *self)
{
  BjbSelectionToolbarPrivate *priv;
  GtkWidget                  *image;
  GtkStyleContext            *context;
  GdkRGBA                     color = {0.0, 0.0, 0.0, 0.0};
  GtkToolbar                 *tlbar;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BJB_TYPE_SELECTION_TOOLBAR, BjbSelectionToolbarPrivate);
  priv = self->priv;
  priv->widget = GTK_WIDGET (self);
  tlbar = GTK_TOOLBAR (self);

  gtk_toolbar_set_show_arrow (tlbar, FALSE);
  gtk_toolbar_set_icon_size (tlbar, GTK_ICON_SIZE_LARGE_TOOLBAR);

  gtk_widget_set_halign (priv->widget, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (priv->widget, GTK_ALIGN_END);
  gtk_widget_set_margin_bottom (priv->widget, 40);
  gtk_widget_set_opacity (priv->widget, 0);
  gtk_widget_set_size_request (priv->widget, 500, -1);

  context = gtk_widget_get_style_context (priv->widget);
  gtk_style_context_add_class (context, "osd");

  gtk_widget_override_background_color (priv->widget,
                                        GTK_STATE_FLAG_NORMAL,
                                        &color);

  priv->left_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  priv->left_group = gtk_tool_item_new ();
  gtk_container_add (GTK_CONTAINER (priv->left_group), priv->left_box);
  gtk_toolbar_insert (tlbar, priv->left_group, -1);
  gtk_widget_show_all (GTK_WIDGET (priv->left_group));

  /* Trash notes */
  priv->toolbar_trash = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("user-trash-symbolic", GTK_ICON_SIZE_INVALID);
  gtk_image_set_pixel_size (GTK_IMAGE (image), 32);
  gtk_container_add (GTK_CONTAINER (priv->toolbar_trash), image);
  gtk_widget_set_tooltip_text (GTK_WIDGET (priv->toolbar_trash), _("Delete"));
  gtk_container_add (GTK_CONTAINER (priv->left_box), priv->toolbar_trash);

  /* Notes color */
  priv->toolbar_color = bjb_color_button_new ();
  gtk_container_add (GTK_CONTAINER (priv->left_box), priv->toolbar_color);
  gtk_widget_set_tooltip_text (GTK_WIDGET (priv->toolbar_color),
                               _("Note color"));

  priv->separator = gtk_separator_tool_item_new ();
  gtk_separator_tool_item_set_draw (GTK_SEPARATOR_TOOL_ITEM (priv->separator), FALSE);
  gtk_widget_set_visible (GTK_WIDGET (priv->separator), TRUE);
  gtk_tool_item_set_expand (priv->separator, TRUE);
  gtk_toolbar_insert (tlbar, priv->separator, -1);

  priv->right_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  priv->right_group = gtk_tool_item_new ();
  gtk_container_add (GTK_CONTAINER (priv->right_group), priv->right_box);
  gtk_toolbar_insert (tlbar, priv->right_group, -1);
  gtk_widget_show_all (GTK_WIDGET (priv->right_group));

  /* Notes tags */
  priv->toolbar_tag = gtk_button_new ();
  image = gtk_image_new_from_icon_name ("list-add-symbolic", GTK_ICON_SIZE_INVALID);
  gtk_image_set_pixel_size (GTK_IMAGE (image), 32);
  gtk_container_add (GTK_CONTAINER (priv->toolbar_tag), image);
  gtk_widget_set_tooltip_text (GTK_WIDGET (priv->toolbar_tag),
                               _("Edit collections"));
  gtk_container_add (GTK_CONTAINER (priv->right_box), priv->toolbar_tag);

  gtk_widget_show_all (priv->widget);
}

static void
bjb_selection_toolbar_get_property (GObject  *object,
                                    guint     property_id,
                                    GValue   *value,
                                    GParamSpec *pspec)
{
  BjbSelectionToolbar *self = BJB_SELECTION_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_BJB_SELECTION:
      g_value_set_object(value, self->priv->selection);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bjb_selection_toolbar_set_property (GObject  *object,
                                    guint     property_id,
                                    const GValue *value,
                                    GParamSpec *pspec)
{
  BjbSelectionToolbar *self = BJB_SELECTION_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_BJB_SELECTION:
      self->priv->selection = g_value_get_object (value);
      break;
    case PROP_BJB_MAIN_VIEW:
      self->priv->view = g_value_get_object (value);
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
  BjbSelectionToolbarPrivate *priv = self->priv ;
  
  G_OBJECT_CLASS (bjb_selection_toolbar_parent_class)->constructed (obj);

  /* item(s) selected --> fade in */
  g_signal_connect (self->priv->selection,
                    "view-selection-changed", 
                    G_CALLBACK(bjb_selection_toolbar_selection_changed),
                    self);

  g_signal_connect_swapped (priv->toolbar_color,"clicked",
                    G_CALLBACK (hide_self), self);

  g_signal_connect (priv->toolbar_color,"color-set",
                    G_CALLBACK (action_color_selected_items), self);

  g_signal_connect (priv->toolbar_tag,"clicked",
                    G_CALLBACK (action_tag_selected_items), self);

  g_signal_connect (priv->toolbar_trash,"clicked",
                    G_CALLBACK (action_delete_selected_items), self);
}

static void
bjb_selection_toolbar_class_init (BjbSelectionToolbarClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->dispose = bjb_selection_toolbar_dispose;
  object_class->get_property = bjb_selection_toolbar_get_property ;
  object_class->set_property = bjb_selection_toolbar_set_property ;
  object_class->constructed = bjb_selection_toolbar_constructed ;

  properties[PROP_BJB_SELECTION] = g_param_spec_object ("selection",
                                                        "Selection",
                                                        "SelectionController",
                                                        GD_TYPE_MAIN_VIEW,
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


  g_type_class_add_private (class, sizeof (BjbSelectionToolbarPrivate));
}


BjbSelectionToolbar *
bjb_selection_toolbar_new (GdMainView   *selection,
                           BjbMainView  *bjb_main_view)
{
  return g_object_new (BJB_TYPE_SELECTION_TOOLBAR,
                       "selection", selection,
                       "bjbmainview",bjb_main_view,
                       NULL);
}
