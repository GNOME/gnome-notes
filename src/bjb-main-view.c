/* bjb-main-view.c
 * Copyright (C) Pierre-Yves LUYTEN 2012 <py@luyten.fr>
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

#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <libbiji/libbiji.h>
#include <libgd/gd-main-view.h>

#include "bjb-app-menu.h"
#include "bjb-bijiben.h"
#include "bjb-controller.h"
#include "bjb-load-more-button.h"
#include "bjb-main-toolbar.h"
#include "bjb-main-view.h"
#include "bjb-note-tag-dialog.h"
#include "bjb-note-view.h"
#include "bjb-rename-note.h"
#include "bjb-search-toolbar.h"
#include "bjb-selection-toolbar.h"
#include "bjb-window-base.h"

#define DEFAULT_VIEW GD_MAIN_VIEW_ICON

enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_BJB_CONTROLLER,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

enum {
  VIEW_SELECTION_CHANGED,
  BJB_MAIN_VIEW_SIGNALS
};

static guint bjb_main_view_signals [BJB_MAIN_VIEW_SIGNALS] = { 0 };

/************************** Gobject ***************************/

struct _BjbMainViewPriv {  
  GtkWidget        *window;
  GtkWidget        *label;

  /* Toolbar */
  BjbMainToolbar   *main_toolbar;

  /* Selection Mode */
  BjbSelectionToolbar  *select_bar;

  /* View Notes , model */
  GdMainView       *view ;
  BjbController    *controller;
  GtkWidget        *load_more;

  /* Signals */
  gulong key;
  gulong activated;
  gulong data;
  gulong view_selection_changed;
};

G_DEFINE_TYPE (BjbMainView, bjb_main_view, GTK_TYPE_BOX);

static void
bjb_main_view_init (BjbMainView *object)
{
  object->priv = 
  G_TYPE_INSTANCE_GET_PRIVATE(object,BJB_TYPE_MAIN_VIEW,BjbMainViewPriv);

  object->priv->key = 0;
  object->priv->activated = 0;
  object->priv->data = 0;
  object->priv->view_selection_changed =0;
}

static void
bjb_main_view_finalize (GObject *object)
{
  G_OBJECT_CLASS (bjb_main_view_parent_class)->finalize (object);
}

static void
bjb_main_view_disconnect_handlers (BjbMainView *self)
{
  BjbMainViewPriv *priv = self->priv;

  g_signal_handler_disconnect (priv->window, priv->key);
  g_signal_handler_disconnect (priv->view, priv->activated);
  g_signal_handler_disconnect (priv->view, priv->data);
  g_signal_handler_disconnect (priv->view, priv->view_selection_changed);

  priv->key = 0;
  priv->activated = 0;
  priv->data = 0;
  priv->view_selection_changed =0;
}

static void
bjb_main_view_set_controller ( BjbMainView *self, BjbController *controller)
{
  self->priv->controller = controller ;
}

static void
bjb_main_view_get_property ( GObject      *object,
                             guint        prop_id,
                             GValue       *value,
                             GParamSpec   *pspec)
{
  BjbMainView *self = BJB_MAIN_VIEW (object);

  switch (prop_id)
  {
    case PROP_WINDOW:
      g_value_set_object (value, self->priv->window);
      break;
    case PROP_BJB_CONTROLLER:
      g_value_set_object (value, self->priv->controller);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bjb_main_view_set_property ( GObject        *object,
                             guint          prop_id,
                             const GValue   *value,
                             GParamSpec     *pspec)
{
  BjbMainView *self = BJB_MAIN_VIEW (object);

  switch (prop_id)
  {
    case PROP_WINDOW:
      self->priv->window = g_value_get_object(value);
      break;
    case PROP_BJB_CONTROLLER:
      bjb_main_view_set_controller(self,g_value_get_object(value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GObject *
biji_main_view_constructor (GType                  gtype,
                            guint                  n_properties,
                            GObjectConstructParam  *properties)
{
  GObject *obj;
  {
    obj = G_OBJECT_CLASS (bjb_main_view_parent_class)->constructor (gtype, n_properties, properties);
  }
  return obj;
}

/* Callbacks */

void
switch_to_note_view (BjbMainView *self, BijiNoteObj *note)
{
  bjb_main_view_disconnect_handlers (self);
  bjb_window_base_switch_to_item (BJB_WINDOW_BASE (self->priv->window), BIJI_ITEM (note));
}

static void
show_window_if_note (gpointer data, gpointer user_data)
{
  BjbWindowBase *window = data;
  BijiNoteObj *to_open = user_data;
  BijiNoteObj *cur = NULL;

  cur = bjb_window_base_get_note (window);

  if (cur && biji_note_obj_are_same (to_open, cur))
    gtk_window_present (data);
}

static void
switch_to_item (BjbMainView *view, BijiItem *to_open)
{
  if (BIJI_IS_NOTE_OBJ (to_open))
  {
    /* If the note is already opened in another window, just show it. */
    if (biji_note_obj_is_opened (BIJI_NOTE_OBJ (to_open)))
    {
      GList *notes ;

      notes = gtk_application_get_windows(GTK_APPLICATION(g_application_get_default()));
      g_list_foreach (notes, show_window_if_note, to_open);
      return ;
    }

    /* Otherwise, leave main view */
    switch_to_note_view (view, BIJI_NOTE_OBJ (to_open));
  }

  /* Collection
   * TODO : check if already opened (same as above) */
  else if (BIJI_IS_COLLECTION (to_open))
  {
    bjb_controller_set_collection (view->priv->controller,
                                   BIJI_COLLECTION (to_open));
  }
}

static GList *
get_selected_paths(BjbMainView *self)
{
  return gd_main_view_get_selection ( self->priv->view ) ; 
}

static gchar *
get_note_url_from_tree_path(GtkTreePath *path, BjbMainView *self)
{
  GtkTreeIter iter ;
  gchar *note_path ;
  GtkTreeModel *model ;

  model = bjb_controller_get_model(self->priv->controller);  
  gtk_tree_model_get_iter (model,&iter, path);
  gtk_tree_model_get (model, &iter,GD_MAIN_COLUMN_URI, &note_path,-1);

  return note_path ;
}


GList *
bjb_main_view_get_selected_items (BjbMainView *view)
{
  GList *l, *paths, *result = NULL;
  gchar *url;
  BijiItem *item;

  /*  GtkTreePath */
  paths = get_selected_paths (view);


  for (l=paths; l!= NULL; l=l->next)
  {
    url = get_note_url_from_tree_path (l->data, view);
    item = biji_note_book_get_item_at_path (
              bjb_window_base_get_book (view->priv->window), url);
    if (BIJI_IS_ITEM (item))
      result = g_list_prepend (result, item);

    g_free (url);
  }

  g_list_free_full (paths, (GDestroyNotify) gtk_tree_path_free);
  return result;
}




static void
on_selection_mode_changed_cb (BjbMainView *self)
{
  GList *select;

  /* Workaround if items are selected
   * but selection mode not really active (?) */
  select = gd_main_view_get_selection (self->priv->view);
  if (select)
  {
    g_list_free (select);
    gd_main_view_set_selection_mode (self->priv->view, TRUE);
  }

  /* Any case, tell */
  g_signal_emit (G_OBJECT (self),
                 bjb_main_view_signals[VIEW_SELECTION_CHANGED],0);
}

/* Select all, escape */
static gboolean
on_key_press_event_cb (GtkWidget *widget,
                       GdkEvent  *event,
                       gpointer   user_data)
{
  BjbMainView *self = BJB_MAIN_VIEW (user_data);
  BjbMainViewPriv *priv = self->priv;

  switch (event->key.keyval)
  {
    case GDK_KEY_a:
    case GDK_KEY_A:
      if (gd_main_view_get_selection_mode (priv->view) && event->key.state & GDK_CONTROL_MASK)
      {
        gd_main_view_select_all (priv->view);
        return TRUE;
      }
      break;

    case GDK_KEY_Escape:
      if (gd_main_view_get_selection_mode (priv->view))
      {
        gd_main_view_set_selection_mode (priv->view, FALSE);
        return TRUE;
      }

    default:
      break;
  }

  return FALSE;
}

static gboolean
on_item_activated (GdMainView        * gd,
                   const gchar       * id,
                   const GtkTreePath * path,
                   BjbMainView       * view)
{
  BijiNoteBook * book ;
  BijiItem     * to_open ;
  GtkTreeIter    iter ;
  gchar        * item_path ;
  GtkTreeModel * model ;

  /* Get Item Path */
  model = gd_main_view_get_model (gd);
  gtk_tree_model_get_iter (model, &iter, (GtkTreePath*) path);
  gtk_tree_model_get (model, &iter, GD_MAIN_COLUMN_URI, &item_path,-1);

  g_return_val_if_fail (item_path != NULL, FALSE); // #709197

  /* Switch to that item */
  book = bjb_window_base_get_book (view->priv->window); 
  to_open = biji_note_book_get_item_at_path (book, item_path);
  g_free (item_path);

  if (to_open)
    switch_to_item (view, to_open);

  return FALSE ;
}

static GtkTargetEntry target_list[] = {
  { "text/plain", 0, 2}
};

static void
on_drag_data_received (GtkWidget        *widget,
                       GdkDragContext   *context,
                       gint              x,
                       gint              y,
                       GtkSelectionData *data,
                       guint             info,
                       guint             time,
                       gpointer          user_data)
{
  gint length = gtk_selection_data_get_length (data) ;

  if (length >= 0)
  {
    guchar *text = gtk_selection_data_get_text(data);

    if (text)
    {
      BijiNoteBook *book;
      BijiNoteObj *ret;
      BjbMainView *self = BJB_MAIN_VIEW (user_data);
      BjbSettings *settings;

      /* FIXME Text is guchar utf 8, conversion to perform */
      book =  bjb_window_base_get_book (self->priv->window);
      settings = bjb_app_get_settings (g_application_get_default ());
      ret = biji_note_book_note_new (book,
                                     (gchar*) text,
                                     bjb_settings_get_default_location (settings));
      switch_to_note_view (self, ret); // maybe AFTER drag finish?

      g_free (text);
    }
  }

  /* Return false to ensure text is not removed from source
   * We just want to create a note. */
  gtk_drag_finish (context, FALSE, FALSE, time);
}


void
bjb_main_view_connect_signals (BjbMainView *self)
{
  BjbMainViewPriv *priv = self->priv;

  if (priv->view_selection_changed == 0)
    priv->view_selection_changed = g_signal_connect_swapped
                                  (priv->view,
                                   "view-selection-changed",
                                   G_CALLBACK (on_selection_mode_changed_cb),
                                   self);

  if (priv->key == 0)
    priv->key = g_signal_connect (priv->window, "key-press-event",
                              G_CALLBACK (on_key_press_event_cb), self);

  if (priv->activated == 0)
    priv->activated = g_signal_connect(priv->view,"item-activated",
                                    G_CALLBACK(on_item_activated),self);

  if (priv->data == 0)
    priv->data = g_signal_connect (priv->view, "drag-data-received",
                              G_CALLBACK (on_drag_data_received), self);
}

void
__destroy_n_notify__ (gpointer data)
{
}



BijiItem *
_get_item_for_tree_path (GtkTreeModel *tree_model,
                         GtkTreeIter *iter,
                         BjbMainView *self)
{
  BijiItem *retval;
  gchar *uuid;


  retval = NULL;
  uuid = NULL;
  gtk_tree_model_get (tree_model,
                      iter,
                      GD_MAIN_COLUMN_ID,
                      &uuid,
                      -1);


  if (uuid != NULL)
  {
    retval = biji_note_book_get_item_at_path (
               bjb_window_base_get_book (self->priv->window), uuid);
    g_free (uuid);
  }

  return retval;
}



static void
render_type     (GtkTreeViewColumn *tree_column,
                 GtkCellRenderer *cell,
                 GtkTreeModel *tree_model,
                 GtkTreeIter *iter,
                 gpointer data)
{
  BijiItem *item;
  const gchar *str;
  BjbMainView *self;

  self = data;
  str = NULL;
  item = _get_item_for_tree_path (tree_model, iter, self);

  if (item != NULL)
  {
    if BIJI_IS_COLLECTION (item)
      str= _("Collection");

    else if BIJI_IS_NOTE_OBJ (item)
      str = _("Note");
  }

  if (str != NULL)
    g_object_set (cell, "text", str, NULL);
}



static void
render_where    (GtkTreeViewColumn *tree_column,
                 GtkCellRenderer *cell,
                 GtkTreeModel *tree_model,
                 GtkTreeIter *iter,
                 gpointer data)
{
  BijiItem *item;
  const gchar *str;
  BjbMainView *self;

  self = data;
  item = _get_item_for_tree_path (tree_model, iter, self);

  if (item != NULL)
  {
    str = biji_item_get_place (item);
    g_object_set (cell, "text", str, NULL);
  }
}



static void
render_date     (GtkTreeViewColumn *tree_column,
                 GtkCellRenderer *cell,
                 GtkTreeModel *tree_model,
                 GtkTreeIter *iter,
                 gpointer data)
{
  BijiItem *item;
  gchar *str;
  BjbMainView *self;

  self = data;
  item = _get_item_for_tree_path (tree_model, iter, self);

  if (item != NULL)
  {
    str = biji_get_time_diff_with_time (biji_item_get_mtime (item));
    g_object_set (cell, "text", str, NULL);
  }
}


static void
add_list_renderers (BjbMainView *self)
{
  GtkWidget *generic;
  GtkCellRenderer *cell;

  generic =  gd_main_view_get_generic_view (self->priv->view);

  /* Type Renderer */
  cell = gd_styled_text_renderer_new ();
  gd_styled_text_renderer_add_class (GD_STYLED_TEXT_RENDERER (cell), "dim-label");
  gtk_cell_renderer_set_padding (cell, 16, 0);

  gd_main_list_view_add_renderer (GD_MAIN_LIST_VIEW (generic),
                                  cell,
                                  render_type,
                                  self,
                                  __destroy_n_notify__);


  /* Where Renderer */
  cell = gd_styled_text_renderer_new ();
  gd_styled_text_renderer_add_class (GD_STYLED_TEXT_RENDERER (cell), "dim-label");
  gtk_cell_renderer_set_padding (cell, 16, 0);

  gd_main_list_view_add_renderer (GD_MAIN_LIST_VIEW (generic),
                                  cell,
                                  render_where,
                                  self,
                                  __destroy_n_notify__);


  /* Date renderer */
  cell = gtk_cell_renderer_text_new ();
  gtk_cell_renderer_set_padding (cell, 32, 0);

  gd_main_list_view_add_renderer (GD_MAIN_LIST_VIEW (generic),
                                  cell,
                                  render_date,
                                  self,
                                  __destroy_n_notify__);

}


static void
bjb_main_view_constructed(GObject *o)
{
  BjbMainView          *self;
  GtkBox               *vbox; //self, too
  BjbMainViewPriv      *priv;

  G_OBJECT_CLASS (bjb_main_view_parent_class)->constructed(G_OBJECT(o));

  self = BJB_MAIN_VIEW(o);
  priv = self->priv ;
  vbox = GTK_BOX (self);

  gtk_box_set_homogeneous (vbox, FALSE);
  gtk_box_set_spacing (vbox, 0);
  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_VERTICAL);

  priv->view = gd_main_view_new (DEFAULT_VIEW);

  /* Main view */
  gd_main_view_set_selection_mode (priv->view, FALSE);
  gd_main_view_set_model (priv->view, bjb_controller_get_model(priv->controller));
  gtk_box_pack_start (vbox, GTK_WIDGET (priv->view), TRUE, TRUE, 0);



  /* Load more */
  priv->load_more = bjb_load_more_button_new (priv->controller);
  gtk_box_pack_start (vbox, priv->load_more, FALSE, FALSE, 0);

  /* Selection Panel */
  priv->select_bar = bjb_selection_toolbar_new (priv->view, self);
  gtk_box_pack_start (vbox, GTK_WIDGET (priv->select_bar), FALSE, FALSE, 0);

  /* Drag n drop */
  gtk_drag_dest_set (GTK_WIDGET (priv->view), GTK_DEST_DEFAULT_ALL,
                     target_list, 1, GDK_ACTION_COPY);

  bjb_main_view_connect_signals (self);
  gtk_widget_show_all (GTK_WIDGET (self));
}

static void
bjb_main_view_class_init (BjbMainViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bjb_main_view_finalize;
  object_class->get_property = bjb_main_view_get_property;
  object_class->set_property = bjb_main_view_set_property;
  object_class->constructor = biji_main_view_constructor;
  object_class->constructed = bjb_main_view_constructed;
    
  g_type_class_add_private (klass, sizeof (BjbMainViewPriv));
  
  properties[PROP_WINDOW] = g_param_spec_object ("window",
                                                 "Window",
                                                 "Parent Window",
                                                 GTK_TYPE_WIDGET,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT |
                                                 G_PARAM_STATIC_STRINGS);

  properties[PROP_BJB_CONTROLLER] = g_param_spec_object ("controller",
                                                         "Controller",
                                                         "BjbController",
                                                         BJB_TYPE_CONTROLLER,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_CONSTRUCT |
                                                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  bjb_main_view_signals[VIEW_SELECTION_CHANGED] = g_signal_new ( "view-selection-changed" ,
                                                  G_OBJECT_CLASS_TYPE (klass),
                                                  G_SIGNAL_RUN_LAST,
                                                  0,
                                                  NULL,
                                                  NULL,
                                                  g_cclosure_marshal_VOID__VOID,
                                                  G_TYPE_NONE,
                                                  0);

}

BjbMainView *
bjb_main_view_new(GtkWidget *win,
                  BjbController *controller)
{
  return g_object_new( BJB_TYPE_MAIN_VIEW,
                       "window", win,
                       "controller", controller,
                       NULL);
}

GtkWidget *
bjb_main_view_get_window(BjbMainView *view)
{
  return view->priv->window ;
}

void
bjb_main_view_update_model (BjbMainView *self)
{
  BjbMainViewPriv *priv = self->priv;

  bjb_controller_update_view (priv->controller);
  gd_main_view_set_model (priv->view, bjb_controller_get_model (priv->controller));
}

/* interface for notes view (GdMainView)
 * TODO - BjbMainView should rather be a GdMainView */

gboolean
bjb_main_view_get_selection_mode (BjbMainView *self)
{
  return gd_main_view_get_selection_mode (self->priv->view);
}

void
bjb_main_view_set_selection_mode (BjbMainView *self, gboolean mode)
{
  gd_main_view_set_selection_mode (self->priv->view, mode);
}

GdMainViewType
bjb_main_view_get_view_type (BjbMainView *view)
{
  return gd_main_view_get_view_type (view->priv->view);
}

void
bjb_main_view_set_view_type (BjbMainView *self, GdMainViewType type)
{
  gd_main_view_set_view_type (self->priv->view, type);

  if (type == GD_MAIN_VIEW_LIST)
    add_list_renderers (self);
}

