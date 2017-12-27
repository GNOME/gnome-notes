/* bjb-note-tag-dialog.h
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

#pragma once

#include <libbiji/libbiji.h>

G_BEGIN_DECLS

#define BJB_TYPE_ORGANIZE_DIALOG (bjb_organize_dialog_get_type ())

G_DECLARE_FINAL_TYPE (BjbOrganizeDialog, bjb_organize_dialog, BJB, ORGANIZE_DIALOG, GtkDialog)

/* Currently only works with one single note
 * the tree view has to be improved to work with several */
void bjb_organize_dialog_new (GtkWindow *parent,
                              GList     *biji_note_obj);

G_END_DECLS
