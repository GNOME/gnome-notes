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
#include <webkit/webkit.h>

#include "../items/bjb-note.h"
#include "../bjb-settings.h"

G_BEGIN_DECLS

/* Available formatting for biji_note_obj_editor_apply_format
 * If note is opened, and if text is opened
 * This toggle the format
 * eg bold text will become normal and normal text becomes bold */
typedef enum
{
  BIJI_NO_FORMAT,
  BIJI_BOLD,
  BIJI_ITALIC,
  BIJI_STRIKE,
  BIJI_BULLET_LIST,
  BIJI_ORDER_LIST,
  BIJI_INDENT,
  BIJI_OUTDENT
} BijiEditorFormat;

#define BIJI_TYPE_WEBKIT_EDITOR (biji_webkit_editor_get_type ())

G_DECLARE_FINAL_TYPE (BijiWebkitEditor, biji_webkit_editor, BIJI, WEBKIT_EDITOR, WebKitWebView)


BijiWebkitEditor * biji_webkit_editor_new (BjbNote *note);

void biji_webkit_editor_apply_format (BijiWebkitEditor *self, gint format);

const gchar * biji_webkit_editor_get_selection (BijiWebkitEditor *self);

void biji_webkit_editor_set_font (BijiWebkitEditor *self, gchar *font);

void biji_webkit_editor_set_text_size (BijiWebkitEditor *self,
                                       BjbTextSizeType   text_size);

G_END_DECLS
