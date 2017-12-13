/* bjb-note-tag-dialog.c
 * Copyright (C) Pierre-Yves LUYTEN 2012 <py@luyten.fr>
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

#include <glib/gi18n.h>

#include "bjb-organize-dialog.h"
#include "bjb-window-base.h"

/* Model for tree view */
enum {
  COL_SELECTION,
  COL_URN,
  COL_TAG_NAME,
  N_COLUMNS
};

enum {
  SELECTION_INCONSISTENT = -1,
  SELECTION_FALSE = 0,
  SELECTION_TRUE
};

/* Prop */
enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_ITEMS,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

struct _BjbOrganizeDialog
{
  GtkDialog parent_instance;

  // parent
  GtkWindow *window;

  // widgets
  GtkWidget  *entry;
  GtkWidget  *view;
  GtkTreeViewColumn *enabled_column;
  GtkCellRenderer   *enabled_cell;

  // data
  GtkListStore *notebook_store;
  GList *items;
  GHashTable *notebooks;

  // tmp when a new tag added
  gchar *tag_to_scroll_to;

  // for convenience, when a tag is toggled
  // stores the notebook urn
  gchar *toggled_notebook;

};

G_DEFINE_TYPE (BjbOrganizeDialog, bjb_organize_dialog, GTK_TYPE_DIALOG)

static void
append_notebook (BijiInfoSet *set, BjbOrganizeDialog *self)
{
  GtkTreeIter iter;
  gint item_has_tag;
  GList *l;

  gtk_list_store_append (self->notebook_store, &iter);
  item_has_tag = biji_item_has_notebook (self->items->data, set->title);

  /* Check if other notes have the same */
  for (l = self->items; l != NULL; l = l->next)
  {
    if (biji_item_has_notebook (l->data, set->title) != item_has_tag)
    {
      item_has_tag = SELECTION_INCONSISTENT;
      break;
    }
  }

  gtk_list_store_set (self->notebook_store,    &iter,
                      COL_SELECTION,  item_has_tag,
                      COL_URN,        set->tracker_urn,
                      COL_TAG_NAME ,  set->title, -1);
}

static gint
bjb_compare_notebook (gconstpointer a, gconstpointer b)
{

  gchar *up_a, *up_b;
  BijiInfoSet *set_a, *set_b;
  gint retval;
  set_a = (BijiInfoSet *) a;
  set_b = (BijiInfoSet *) b;

  up_a = g_utf8_strup (set_a->title, -1);
  up_b = g_utf8_strup (set_b->title, -1);
  retval = g_strcmp0 (up_a, up_b);

  g_free (up_a);
  g_free (up_b);
  return retval;
}

/* If true, free the retval with gtk_tree_path_free */
static gboolean
bjb_get_path_for_str (GtkTreeModel  *model,
                      GtkTreePath  **return_value,
                      gint           column,
                      gchar         *needle)
{
  gboolean retval= FALSE;
  GtkTreeIter iter;
  gboolean valid = gtk_tree_model_get_iter_first (model, &iter);
  *return_value = NULL;

  while (valid)
  {
    gchar *cur_str = NULL;
    gtk_tree_model_get (model, &iter, column, &cur_str, -1);

    if (cur_str)
    {
      if (g_strcmp0 (cur_str, needle)==0)
        *return_value = gtk_tree_model_get_path (model, &iter);

      g_free (cur_str);
    }

    if (*return_value)
    {
      retval = TRUE;
      break;
    }

    valid = gtk_tree_model_iter_next (model, &iter);
  }

  return retval;
}

static void
bjb_organize_dialog_handle_tags (GHashTable *result, gpointer user_data)
{
  BjbOrganizeDialog *self = BJB_ORGANIZE_DIALOG (user_data);
  GList *tracker_info;

  if (self->notebooks)
    g_hash_table_destroy (self->notebooks);

  self->notebooks = result;

  tracker_info = g_hash_table_get_values (self->notebooks);
  tracker_info = g_list_sort (tracker_info, bjb_compare_notebook);
  g_list_foreach (tracker_info, (GFunc) append_notebook, self);
  g_list_free (tracker_info);

  /* If a new tag was added, scroll & free */
  if (self->tag_to_scroll_to)
  {
    GtkTreePath *path = NULL;

    if (bjb_get_path_for_str (GTK_TREE_MODEL (self->notebook_store), &path,
                              COL_TAG_NAME, self->tag_to_scroll_to))
    {
      gtk_tree_view_scroll_to_cell (GTK_TREE_VIEW (self->view), path, NULL, TRUE, 0.5, 0.5);
      gtk_tree_path_free (path);
    }

    g_clear_pointer (& (self->tag_to_scroll_to), g_free);
  }
}

static void
update_notebooks_model_async (BjbOrganizeDialog *self)
{
  BijiManager *manager;

  manager = bjb_window_base_get_manager (GTK_WIDGET (self->window));
  gtk_list_store_clear (self->notebook_store);
  biji_get_all_notebooks_async (manager, bjb_organize_dialog_handle_tags, self);
}

/* Libbiji handles tracker & saving */
static void
note_dialog_add_notebook (gpointer iter, gpointer user_data)
{
  g_return_if_fail (BIJI_IS_NOTEBOOK (user_data));
  biji_item_add_notebook (iter, user_data, NULL);
}


static void
note_dialog_remove_notebook (gpointer iter, gpointer user_data)
{
  g_return_if_fail (BIJI_IS_NOTEBOOK (user_data));
  biji_item_remove_notebook (iter, user_data);
}

static void
on_tag_toggled (GtkCellRendererToggle *cell,
                gchar *path_str,
                BjbOrganizeDialog *self)
{
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  GtkTreeModel *model = GTK_TREE_MODEL (self->notebook_store);
  GtkTreeIter iter;
  gint toggle_item;
  gint *column;
  gchar *tag;
  BijiManager *manager;
  BijiItem *notebook;

  column = g_object_get_data (G_OBJECT (cell), "column");
  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter, column, &toggle_item, -1);
  gtk_tree_model_get (model, &iter, COL_URN, &tag, -1);

  self->toggled_notebook = tag;
  manager = bjb_window_base_get_manager (GTK_WIDGET (self->window));
  notebook = biji_manager_get_item_at_path (manager, tag);

  if (BIJI_IS_NOTEBOOK (notebook))
  {
    if (toggle_item == SELECTION_INCONSISTENT || toggle_item == SELECTION_FALSE)
    {
      g_list_foreach (self->items, note_dialog_add_notebook, notebook);
      toggle_item = SELECTION_TRUE;
    }

    else
    {
      g_list_foreach (self->items, note_dialog_remove_notebook, notebook);
      toggle_item = SELECTION_FALSE;
    }
  }

  self->toggled_notebook = NULL;
  gtk_list_store_set (self->notebook_store, &iter, column, toggle_item, -1);
  gtk_tree_path_free (path);
}


/* If the notebook with same title already existed,
 * libbiji has to avoid creating a new one
 * and also check before tagging items */
static void
on_new_notebook_created_cb (BijiItem *coll, gpointer user_data)
{
  BjbOrganizeDialog *self = user_data;

  self->tag_to_scroll_to = g_strdup (gtk_entry_get_text (GTK_ENTRY (self->entry)));
  g_list_foreach (self->items, note_dialog_add_notebook, coll);

  update_notebooks_model_async (self);
  gtk_entry_set_text (GTK_ENTRY (self->entry), "");
}

/* Gives the title and manager :
 * the notebook is created & manager updated.
 * afterward, our callback comes */
static void
add_new_tag (BjbOrganizeDialog *self)
{
  BijiManager *manager = bjb_window_base_get_manager (GTK_WIDGET (self->window));
  const gchar *title = gtk_entry_get_text (GTK_ENTRY (self->entry));

  if (title && title[0])
    biji_create_new_notebook_async (manager, title, on_new_notebook_created_cb, self);
}

static void
bjb_organize_toggle_cell_data_func (GtkTreeViewColumn *column,
                                    GtkCellRenderer   *cell_renderer,
                                    GtkTreeModel      *tree_model,
                                    GtkTreeIter       *iter,
                                    gpointer           user_data)
{
  GValue inconsistent = { 0 };
  gint selection;

  gtk_tree_model_get (tree_model, iter, COL_SELECTION, &selection, -1);
  gtk_cell_renderer_toggle_set_active (GTK_CELL_RENDERER_TOGGLE (cell_renderer),
                                       SELECTION_TRUE == selection);

  g_value_init (&inconsistent, G_TYPE_BOOLEAN);
  g_value_set_boolean (&inconsistent, SELECTION_INCONSISTENT == selection);
  g_object_set_property (G_OBJECT (cell_renderer), "inconsistent", &inconsistent);
}

static void
bjb_organize_dialog_init (BjbOrganizeDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bjb_organize_dialog_constructed (GObject *obj)
{
  BjbOrganizeDialog *self = BJB_ORGANIZE_DIALOG (obj);

  update_notebooks_model_async (self);
  gtk_tree_view_column_set_cell_data_func (self->enabled_column,
                                           self->enabled_cell,
                                           bjb_organize_toggle_cell_data_func,
                                           NULL,
                                           NULL);
}

static void
bjb_organize_dialog_finalize (GObject *object)
{
  BjbOrganizeDialog *self = BJB_ORGANIZE_DIALOG (object);

  g_hash_table_destroy (self->notebooks);

  /* no reason, it should have been freed earlier */
  if (self->tag_to_scroll_to)
    g_free (self->tag_to_scroll_to);

  G_OBJECT_CLASS (bjb_organize_dialog_parent_class)->finalize (object);
}

static void
bjb_organize_dialog_get_property (GObject      *object,
                                  guint        prop_id,
                                  GValue       *value,
                                  GParamSpec   *pspec)
{
  BjbOrganizeDialog *self = BJB_ORGANIZE_DIALOG (object);

  switch (prop_id)
  {
    case PROP_WINDOW:
      g_value_set_object (value, self->window);
      break;
    case PROP_ITEMS:
      g_value_set_pointer (value, self->items);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bjb_organize_dialog_set_property (GObject        *object,
                                  guint          prop_id,
                                  const GValue   *value,
                                  GParamSpec     *pspec)
{
  BjbOrganizeDialog *self = BJB_ORGANIZE_DIALOG (object);

  switch (prop_id)
  {
    case PROP_WINDOW:
      self->window = g_value_get_object(value);
      break;
    case PROP_ITEMS:
      self->items = g_value_get_pointer (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bjb_organize_dialog_class_init (BjbOrganizeDialogClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = bjb_organize_dialog_constructed;
  object_class->finalize = bjb_organize_dialog_finalize;
  object_class->get_property = bjb_organize_dialog_get_property;
  object_class->set_property = bjb_organize_dialog_set_property;

  properties[PROP_WINDOW] = g_param_spec_object ("window",
                                                 "Window",
                                                 "Parent Window",
                                                 GTK_TYPE_WIDGET,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT |
                                                 G_PARAM_STATIC_STRINGS);

  properties[PROP_ITEMS] = g_param_spec_pointer ("items",
                                                 "Biji Items",
                                                 "Notes and Notebooks to tag",
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT_ONLY |
                                                 G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/bijiben/ui/organize-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbOrganizeDialog, entry);
  gtk_widget_class_bind_template_child (widget_class, BjbOrganizeDialog, view);
  gtk_widget_class_bind_template_child (widget_class, BjbOrganizeDialog, notebook_store);
  gtk_widget_class_bind_template_child (widget_class, BjbOrganizeDialog, enabled_column);
  gtk_widget_class_bind_template_child (widget_class, BjbOrganizeDialog, enabled_cell);

  gtk_widget_class_bind_template_callback (widget_class, add_new_tag);
  gtk_widget_class_bind_template_callback (widget_class, on_tag_toggled);
}

void
bjb_organize_dialog_new (GtkWindow *parent,
                         GList     *biji_items)
{
  BjbOrganizeDialog *self = g_object_new (BJB_TYPE_ORGANIZE_DIALOG,
					  "use-header-bar", TRUE,
                                          "window", parent,
                                          "items", biji_items,
                                          "transient-for", parent,
                                          NULL);
  gtk_dialog_run (GTK_DIALOG (self));
}
