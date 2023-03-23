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

#pragma once

#include <gtk/gtk.h>

#include "items/bjb-tag.h"

G_BEGIN_DECLS

#define BJB_TYPE_NOTEBOOK_ROW (bjb_notebook_row_get_type ())

G_DECLARE_FINAL_TYPE (BjbNotebookRow, bjb_notebook_row, BJB, NOTEBOOK_ROW, GtkListBoxRow)

GtkWidget    *bjb_notebook_row_new        (BjbItem        *notebook);
BjbItem      *bjb_notebook_row_get_item   (BjbNotebookRow *self);
gboolean      bjb_notebook_row_get_active (BjbNotebookRow *self);
void          bjb_notebook_row_set_active (BjbNotebookRow *self,
                                           gboolean        is_active);

G_END_DECLS
