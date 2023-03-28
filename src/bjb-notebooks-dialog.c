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
#include "providers/bjb-provider.h"
#include "bjb-notebook-row.h"
#include "bjb-notebooks-dialog.h"
#include "bjb-log.h"

struct _BjbNotebooksDialog
{
  GtkDialog     parent_instance;

  BjbNote      *item;

  GtkWidget    *notebook_entry;
  GtkWidget    *add_notebook_button;
  GtkWidget    *notebooks_list;
};

G_DEFINE_TYPE (BjbNotebooksDialog, bjb_notebooks_dialog, GTK_TYPE_DIALOG)

static void
on_notebook_entry_changed_cb (BjbNotebooksDialog *self)
{
  BjbProvider *provider;
  BjbTagStore *tag_store;
  GListModel *tags;
  const char *tag;
  guint n_items;

  g_assert (BJB_IS_NOTEBOOKS_DIALOG (self));
  g_assert (self->item);

  tag = gtk_editable_get_text (GTK_EDITABLE (self->notebook_entry));

  if (!tag || !*tag)
    {
      gtk_widget_set_sensitive (self->add_notebook_button, FALSE);
      return;
    }

  provider = g_object_get_data (G_OBJECT (self->item), "provider");
  g_assert (BJB_IS_PROVIDER (provider));

  tag_store = bjb_provider_get_tag_store (provider);
  tags = bjb_tag_store_get_tags (tag_store);
  n_items = g_list_model_get_n_items (tags);

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(BjbItem) item = NULL;

      item = g_list_model_get_item (tags, i);

      if (g_strcmp0 (bjb_item_get_title (item), tag) == 0)
        {
          gtk_widget_set_sensitive (self->add_notebook_button, FALSE);
          return;
        }
    }

  gtk_widget_set_sensitive (self->add_notebook_button, TRUE);
}

static void
on_add_notebook_button_clicked_cb (BjbNotebooksDialog *self)
{
  const char *tag;

  tag = gtk_editable_get_text (GTK_EDITABLE (self->notebook_entry));
  bjb_note_add_tag (self->item, tag);
}

static void
on_notebooks_row_activated_cb (BjbNotebooksDialog *self,
                               BjbNotebookRow     *row,
                               GtkListBox         *box)
{
  BjbItem *notebook;

  g_assert (BJB_IS_NOTEBOOKS_DIALOG (self));
  g_assert (GTK_IS_LIST_BOX (box));
  g_assert (BJB_IS_NOTEBOOK_ROW (row));
  g_assert (BJB_IS_NOTE (self->item));

  notebook = bjb_notebook_row_get_item (row);

  BJB_TRACE_MSG ("Notebook '%s' %s",
                 bjb_item_get_title (notebook),
                 bjb_notebook_row_get_active (row) ? "selected" : "deselected");

  /* add tag if it's not already, remove otherwise */
  if (!bjb_notebook_row_get_active (row))
    bjb_note_add_tag (self->item, bjb_item_get_title (notebook));
  else
    bjb_note_remove_tag (self->item, notebook);
}

static GtkWidget *
notebooks_row_new (BjbItem            *notebook,
                   BjbNotebooksDialog *self)
{
  GtkWidget *row;

  g_assert (BJB_IS_NOTEBOOKS_DIALOG (self));
  g_assert (BJB_IS_NOTEBOOK (notebook) || BJB_IS_TAG (notebook));

  row = bjb_notebook_row_new (notebook);
  gtk_widget_set_visible (row, TRUE);

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
  gtk_widget_init_template (GTK_WIDGET (self));
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

static void
notebooks_note_tag_changed_cb (BjbNotebooksDialog *self)
{
  BjbNotebookRow *row;
  GListModel *tags;
  int index;

  g_assert (BJB_IS_NOTEBOOKS_DIALOG (self));

  tags = bjb_note_get_tags (self->item);

  index = 0;
  do
    {
      row = (BjbNotebookRow *)gtk_list_box_get_row_at_index (GTK_LIST_BOX (self->notebooks_list), index);

      if (row)
        {
          BjbItem *notebook;

          notebook = bjb_notebook_row_get_item (row);
          if (g_list_store_find (G_LIST_STORE (tags), notebook, NULL))
            bjb_notebook_row_set_active (row, TRUE);
          else
            bjb_notebook_row_set_active (row, FALSE);
        }
      index++;
    } while (row);
}

void
bjb_notebooks_dialog_set_item (BjbNotebooksDialog *self,
                               BjbNote            *note)
{
  BjbProvider *provider;
  BjbTagStore *tag_store;
  GListModel *tags;

  g_return_if_fail (BJB_IS_NOTEBOOKS_DIALOG (self));
  g_return_if_fail (BJB_IS_NOTE (note));

  if (!g_set_object (&self->item, note))
    return;

  BJB_DEBUG_MSG ("Setting note '%s'", bjb_item_get_title (BJB_ITEM (note)));

  provider = g_object_get_data (G_OBJECT (note), "provider");
  g_assert (BJB_IS_PROVIDER (provider));

  tag_store = bjb_provider_get_tag_store (provider);
  gtk_list_box_bind_model (GTK_LIST_BOX (self->notebooks_list),
                           bjb_tag_store_get_tags (tag_store),
                           (GtkListBoxCreateWidgetFunc)notebooks_row_new,
                           g_object_ref (self), g_object_unref);

  tags = bjb_note_get_tags (note);
  g_signal_connect_object (tags, "items-changed",
                           G_CALLBACK (notebooks_note_tag_changed_cb),
                           self, G_CONNECT_SWAPPED | G_CONNECT_AFTER);
  notebooks_note_tag_changed_cb (self);
}
