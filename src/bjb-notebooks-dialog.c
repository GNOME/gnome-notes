/* -*- mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* bjb-notebooks-dialog.c
 *
 * Copyright 2021 Mohammed Sadiq <sadiq@sadiqpk.org>
 * Copyright 2021 Purism SPC
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

#define G_LOG_DOMAIN "bjb-notebooks-dialog"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "bjb-application.h"
#include "bjb-notebook-row.h"
#include "bjb-notebooks-dialog.h"
#include "bjb-log.h"

struct _BjbNotebooksDialog
{
  GtkDialog     parent_instance;

  BijiNoteObj  *item;

  GtkWidget    *notebook_entry;
  GtkWidget    *add_notebook_button;
  GtkWidget    *notebooks_list;
};

G_DEFINE_TYPE (BjbNotebooksDialog, bjb_notebooks_dialog, GTK_TYPE_DIALOG)

static void
on_notebook_entry_changed_cb (BjbNotebooksDialog *self)
{
  GListModel *notebooks;
  const char *notebook;
  guint n_items;

  g_assert (BJB_IS_NOTEBOOKS_DIALOG (self));
  g_assert (self->item);

  notebook = gtk_entry_get_text (GTK_ENTRY (self->notebook_entry));

  if (!notebook || !*notebook)
    {
      gtk_widget_set_sensitive (self->add_notebook_button, FALSE);
      return;
    }

  notebooks = biji_manager_get_notebooks (biji_item_get_manager (BIJI_ITEM (self->item)));
  n_items = g_list_model_get_n_items (notebooks);

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(BijiItem) item = NULL;

      item = g_list_model_get_item (notebooks, i);

      if (g_strcmp0 (biji_item_get_title (item), notebook) == 0)
        {
          gtk_widget_set_sensitive (self->add_notebook_button, FALSE);
          return;
        }
    }

  gtk_widget_set_sensitive (self->add_notebook_button, TRUE);
}

static void
on_new_notebook_created_cb (GObject      *object,
                            GAsyncResult *result,
                            gpointer      user_data)
{
  BjbNotebooksDialog *self = user_data;
  g_autoptr(BijiItem) notebook = NULL;
  g_autoptr(GList) rows = NULL;

  notebook = biji_tracker_add_notebook_finish (BIJI_TRACKER (object), result, NULL);
  biji_item_add_notebook (BIJI_ITEM (self->item), notebook, NULL);
  gtk_entry_set_text (GTK_ENTRY (self->notebook_entry), "");

  rows = gtk_container_get_children (GTK_CONTAINER (self->notebooks_list));

  for (GList *row = rows; row; row = row->next)
    if (notebook == bjb_notebook_row_get_item (row->data))
      {
        bjb_notebook_row_set_active (row->data, TRUE);
        break;
      }
}

static void
on_add_notebook_button_clicked_cb (BjbNotebooksDialog *self)
{
  BijiManager *manager;
  const char *notebook;

  notebook = gtk_entry_get_text (GTK_ENTRY (self->notebook_entry));

  manager = biji_item_get_manager (BIJI_ITEM (self->item));
  biji_tracker_add_notebook_async (biji_manager_get_tracker (manager),
                                   notebook, on_new_notebook_created_cb, self);
}

static void
on_notebooks_row_activated_cb (BjbNotebooksDialog *self,
                               BjbNotebookRow     *row,
                               GtkListBox         *box)
{
  BijiItem *notebook;

  g_assert (BJB_IS_NOTEBOOKS_DIALOG (self));
  g_assert (GTK_IS_LIST_BOX (box));
  g_assert (BJB_IS_NOTEBOOK_ROW (row));
  g_assert (BIJI_IS_NOTE_OBJ (self->item));

  bjb_notebook_row_set_active (row, !bjb_notebook_row_get_active (row));
  notebook = bjb_notebook_row_get_item (row);

  BJB_TRACE_MSG ("Notebook '%s' %s",
                 biji_item_get_title (notebook),
                 bjb_notebook_row_get_active (row) ? "selected" : "deselected");

  if (bjb_notebook_row_get_active (row))
    biji_item_add_notebook (BIJI_ITEM (self->item), notebook, NULL);
  else
    biji_item_remove_notebook (BIJI_ITEM (self->item), notebook);
}

static GtkWidget *
notebooks_row_new (BijiNotebook       *notebook,
                   BjbNotebooksDialog *self)
{
  GtkWidget *row;

  g_assert (BJB_IS_NOTEBOOKS_DIALOG (self));
  g_assert (BIJI_IS_NOTEBOOK (notebook));

  row = bjb_notebook_row_new (BIJI_ITEM (notebook));

  return row;
}

static void
bjb_notebooks_dialog_finalize (GObject *object)
{
  BjbNotebooksDialog *self = BJB_NOTEBOOKS_DIALOG (object);

  g_clear_object (&self->item);

  G_OBJECT_CLASS (bjb_notebooks_dialog_parent_class)->finalize (object);
}

static void
bjb_notebooks_dialog_class_init (BjbNotebooksDialogClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bjb_notebooks_dialog_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Notes/"
                                               "ui/bjb-notebooks-dialog.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbNotebooksDialog, notebook_entry);
  gtk_widget_class_bind_template_child (widget_class, BjbNotebooksDialog, add_notebook_button);
  gtk_widget_class_bind_template_child (widget_class, BjbNotebooksDialog, notebooks_list);

  gtk_widget_class_bind_template_callback (widget_class, on_notebook_entry_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_add_notebook_button_clicked_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_notebooks_row_activated_cb);
}

static void
bjb_notebooks_dialog_init (BjbNotebooksDialog *self)
{
  BijiManager *manager;

  gtk_widget_init_template (GTK_WIDGET (self));

  manager = bijiben_get_manager (BJB_APPLICATION (g_application_get_default ()));
  g_return_if_fail (manager);

  gtk_list_box_bind_model (GTK_LIST_BOX (self->notebooks_list),
                           biji_manager_get_notebooks (manager),
                           (GtkListBoxCreateWidgetFunc)notebooks_row_new,
                           g_object_ref (self), g_object_unref);
}

GtkWidget *
bjb_notebooks_dialog_new (GtkWindow *parent_window)
{
  g_return_val_if_fail (GTK_IS_WINDOW (parent_window), NULL);

  return g_object_new (BJB_TYPE_NOTEBOOKS_DIALOG,
                       "use-header-bar", TRUE,
                       "transient-for", parent_window,
                       NULL);
}

void
bjb_notebooks_dialog_set_item (BjbNotebooksDialog *self,
                               BijiNoteObj        *note)
{
  g_autoptr(GList) notebooks = NULL;
  g_autoptr(GList) rows = NULL;

  g_return_if_fail (BJB_IS_NOTEBOOKS_DIALOG (self));
  g_return_if_fail (BIJI_IS_NOTE_OBJ (note));

  if (!g_set_object (&self->item, note))
    return;

  BJB_DEBUG_MSG ("Setting note '%s'", biji_item_get_title (BIJI_ITEM (note)));

  notebooks = biji_note_obj_get_notebooks (self->item);
  rows = gtk_container_get_children (GTK_CONTAINER (self->notebooks_list));

  /* Deselect all rows first */
  for (GList *row = rows; row; row = row->next)
    bjb_notebook_row_set_active (row->data, FALSE);

  for (GList *row = rows; row; row = row->next)
    {
      BijiItem *notebook;
      gboolean selected;

      notebook = bjb_notebook_row_get_item (row->data);
      selected = biji_item_has_notebook (BIJI_ITEM (self->item),
                                         (char *)biji_item_get_title (notebook));
      bjb_notebook_row_set_active (row->data, selected);
    }
}
