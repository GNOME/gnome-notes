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

#pragma once

#include <gtk/gtk.h>

#include "items/bjb-note.h"

G_BEGIN_DECLS

#define BJB_TYPE_NOTEBOOKS_DIALOG (bjb_notebooks_dialog_get_type ())

G_DECLARE_FINAL_TYPE (BjbNotebooksDialog, bjb_notebooks_dialog, BJB, NOTEBOOKS_DIALOG, GtkDialog)

GtkWidget *bjb_notebooks_dialog_new      (GtkWindow          *parent_window);
void       bjb_notebooks_dialog_set_item (BjbNotebooksDialog *self,
                                          BjbNote            *note);

G_END_DECLS

