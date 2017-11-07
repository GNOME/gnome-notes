/* bjb-note-tag-dialog.c
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

#include <glib/gi18n.h>

#include "bjb-organize-dialog.h"
#include "bjb-window-base.h"

#define BJB_ORGANIZE_DIALOG_DEFAULT_WIDTH  460
#define BJB_ORGANIZE_DIALOG_DEFAULT_HEIGHT 380

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
  GtkWidget    * entry;
  GtkTreeView  * view;

  // data
  GList *items;
  GtkListStore *store;
  GHashTable *notebooks;

  // tmp when a new tag added
  gchar *tag_to_scroll_to;

  // for convenience, when a tag is toggled
  // stores the notebook urn
  gchar *toggled_notebook;

};

#define BJB_ORGANIZE_DIALOG_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BJB_TYPE_ORGANIZE_DIALOG, BjbOrganizeDialogPrivate))

G_DEFINE_TYPE (BjbOrganizeDialog, bjb_organize_dialog, GTK_TYPE_DIALOG);

static void
append_notebook (BijiInfoSet *set, BjbOrganizeDialog *self)
{
  GtkTreeIter iter;
  gint item_has_tag;
  GList *l;

  gtk_list_store_append (self->store, &iter);
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

  gtk_list_store_set (self->store,    &iter,
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

    if (bjb_get_path_for_str (GTK_TREE_MODEL (self->store), &path,
                              COL_TAG_NAME, self->tag_to_scroll_to))
    {
      gtk_tree_view_scroll_to_cell (self->view, path, NULL, TRUE, 0.5, 0.5);
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
  gtk_list_store_clear (self->store);
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
  GtkTreeModel *model = GTK_TREE_MODEL (self->store);
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
  gtk_list_store_set (self->store, &iter, column, toggle_item, -1);
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

  if (title && g_utf8_strlen (title, -1) > 0)
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
add_columns (GtkTreeView *view, BjbOrganizeDialog *self)
{
  GtkTreeViewColumn *column;
  GtkCellRenderer *cell_renderer;

  /* List column: toggle */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (view, column);

  gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
  gtk_tree_view_column_set_fixed_width (column, 50);

  cell_renderer = gtk_cell_renderer_toggle_new ();
  g_signal_connect (cell_renderer, "toggled",
                    G_CALLBACK (on_tag_toggled), self);

  gtk_tree_view_column_pack_start (column, cell_renderer, TRUE);
  gtk_tree_view_column_set_cell_data_func (column,
                                           cell_renderer,
                                           bjb_organize_toggle_cell_data_func,
                                           NULL,
                                           NULL);

  /* URN */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (view, column);


  /* List column: notebook title */
  column = gtk_tree_view_column_new ();
  gtk_tree_view_append_column (view, column);

  cell_renderer = gtk_cell_renderer_text_new ();
  gtk_tree_view_column_set_expand (column, TRUE);
  gtk_tree_view_column_pack_start (column, cell_renderer, TRUE);
  gtk_tree_view_column_add_attribute (column, cell_renderer, "text", COL_TAG_NAME);

}

static void
bjb_organize_dialog_init (BjbOrganizeDialog *self)
{
  gtk_window_set_default_size (GTK_WINDOW (self),
                               BJB_ORGANIZE_DIALOG_DEFAULT_WIDTH,
                               BJB_ORGANIZE_DIALOG_DEFAULT_HEIGHT);
  gtk_window_set_title (GTK_WINDOW (self), _("Notebooks"));

  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR (gtk_dialog_get_header_bar (GTK_DIALOG (self))), TRUE);

  g_signal_connect_swapped (self, "response",
                            G_CALLBACK (gtk_widget_destroy), self);

  self->store = gtk_list_store_new (N_COLUMNS,
                                    G_TYPE_INT,      // notebook is active
                                    G_TYPE_STRING,   // notebook urn
                                    G_TYPE_STRING);  // notebook title
}


static void
bjb_organize_dialog_constructed (GObject *obj)
{
  BjbOrganizeDialog *self = BJB_ORGANIZE_DIALOG (obj);
  GtkWidget *hbox, *label, *new, *area, *sw;

  gtk_window_set_transient_for (GTK_WINDOW (self), self->window);

  area = gtk_dialog_get_content_area (GTK_DIALOG (self));
  gtk_container_set_border_width (GTK_CONTAINER (area), 8);

  label = gtk_label_new (_("Enter a name to create a notebook"));
  gtk_box_pack_start (GTK_BOX (area), label, FALSE, FALSE, 2);

  /* New Tag */
  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_pack_start (GTK_BOX (area), hbox, FALSE, FALSE, 2);

  self->entry = gtk_entry_new();
  gtk_box_pack_start (GTK_BOX (hbox), self->entry, TRUE, TRUE, 0);

  new = gtk_button_new_with_label (_("New notebook"));
  g_signal_connect_swapped (new, "clicked", G_CALLBACK (add_new_tag), self);

  gtk_box_pack_start (GTK_BOX (hbox), new, FALSE, FALSE, 2);

  /* List of notebooks */
  sw = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw),
                                       GTK_SHADOW_ETCHED_IN);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
                                  GTK_POLICY_AUTOMATIC,
                                  GTK_POLICY_AUTOMATIC);

  update_notebooks_model_async (self);
  self->view = GTK_TREE_VIEW (gtk_tree_view_new_with_model (GTK_TREE_MODEL (self->store)));
  gtk_tree_view_set_headers_visible (self->view, FALSE);
  g_object_unref (self->store);

  gtk_tree_selection_set_mode (gtk_tree_view_get_selection (self->view),
                               GTK_SELECTION_MULTIPLE);

  add_columns (self->view, self);
  gtk_container_add (GTK_CONTAINER (sw), GTK_WIDGET (self->view));

  gtk_box_pack_start (GTK_BOX (area), sw, TRUE, TRUE,2);
  gtk_widget_show_all (area);
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
}

void
bjb_organize_dialog_new (GtkWindow *parent,
                         GList     *biji_items)
{
  BjbOrganizeDialog *self = g_object_new (BJB_TYPE_ORGANIZE_DIALOG,
					  "use-header-bar", TRUE,
                                          "window", parent,
                                          "items", biji_items,
                                          NULL);

  gtk_dialog_run (GTK_DIALOG (self));
}
