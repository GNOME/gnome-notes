/* biji-webkit-editor
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

#include <gtk/gtk.h>
#include <webkit2/webkit2.h>

#include "../biji-note-obj.h"
#include "../bjb-settings.h"

G_BEGIN_DECLS

#define BIJI_TYPE_WEBKIT_EDITOR (biji_webkit_editor_get_type ())

G_DECLARE_FINAL_TYPE (BijiWebkitEditor, biji_webkit_editor, BIJI, WEBKIT_EDITOR, WebKitWebView)


BijiWebkitEditor * biji_webkit_editor_new (BijiNoteObj *note);

void biji_webkit_editor_apply_format (BijiWebkitEditor *self, gint format);

const gchar * biji_webkit_editor_get_selection (BijiWebkitEditor *self);

void biji_webkit_editor_paste (BijiWebkitEditor *self);

void biji_webkit_editor_undo (BijiWebkitEditor *self);
void biji_webkit_editor_redo (BijiWebkitEditor *self);

void biji_webkit_editor_set_font (BijiWebkitEditor *self, gchar *font);

void biji_webkit_editor_set_text_size (BijiWebkitEditor *self,
                                       BjbTextSizeType   text_size);

G_END_DECLS
