/* -*- mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* bjb-notebook-row.c
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
 *
 * Author(s):
 *   Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "bjb-notebook-row"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdbool.h>

#include "bjb-notebook-row.h"
#include "bjb-log.h"

struct _BjbNotebookRow
{
  GtkListBoxRow   parent_instance;

  BjbItem        *item;

  GtkWidget      *select_image;
  GtkWidget      *tag_label;

  gboolean        selected;
};

G_DEFINE_TYPE (BjbNotebookRow, bjb_notebook_row, GTK_TYPE_LIST_BOX_ROW)


static void
bjb_notebook_row_finalize (GObject *object)
{
  BjbNotebookRow *self = (BjbNotebookRow *)object;

  g_clear_object (&self->item);

  G_OBJECT_CLASS (bjb_notebook_row_parent_class)->finalize (object);
}

static void
bjb_notebook_row_class_init (BjbNotebookRowClass *klass)
{
  GObjectClass   *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bjb_notebook_row_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Notes/"
                                               "ui/bjb-notebook-row.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbNotebookRow, select_image);
  gtk_widget_class_bind_template_child (widget_class, BjbNotebookRow, tag_label);
}

static void
bjb_notebook_row_init (BjbNotebookRow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

GtkWidget *
bjb_notebook_row_new (BjbItem *notebook)
{
  BjbNotebookRow *self;

  g_return_val_if_fail (BJB_IS_NOTEBOOK (notebook) || BJB_IS_TAG (notebook), NULL);

  self = g_object_new (BJB_TYPE_NOTEBOOK_ROW, NULL);
  self->item = g_object_ref (notebook);

  gtk_label_set_text (GTK_LABEL (self->tag_label),
                      bjb_item_get_title (notebook));

  return GTK_WIDGET (self);
}

BjbItem *
bjb_notebook_row_get_item (BjbNotebookRow *self)
{
  g_return_val_if_fail (BJB_IS_NOTEBOOK_ROW (self), NULL);

  return self->item;
}

gboolean
bjb_notebook_row_get_active (BjbNotebookRow *self)
{
  g_return_val_if_fail (BJB_IS_NOTEBOOK_ROW (self), false);

  return gtk_widget_get_opacity (self->select_image) == 1.0;
}

void
bjb_notebook_row_set_active (BjbNotebookRow *self,
                             gboolean        is_active)
{
  g_return_if_fail (BJB_IS_NOTEBOOK_ROW (self));

  gtk_widget_set_opacity (self->select_image, is_active ? 1.0 : 0.0);
}
