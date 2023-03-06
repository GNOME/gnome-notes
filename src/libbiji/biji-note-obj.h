/* biji-note-obj.h
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

#include <glib-object.h>
#include <gtk/gtk.h>

#include "../items/bjb-item.h"
#include "../items/bjb-note.h"
#include "biji-info-set.h"

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

#define BIJI_TYPE_NOTE_OBJ (biji_note_obj_get_type ())

G_DECLARE_DERIVABLE_TYPE (BijiNoteObj, biji_note_obj, BIJI, NOTE_OBJ, BjbNote)

struct _BijiNoteObjClass
{
  BjbNoteClass  parent_class;

  char         *(*get_basename)      (BijiNoteObj *note);

  /*
   * Mandatory. Provide the latest note html.
   * Use html_from_plain_text if needed.
   * This string must be allocated. use g_strdup if needed. */
  char         *(*get_html)          (BijiNoteObj *note);

  /*
   * Mandatory. When editor amends html, assign it */
  void          (*set_html)          (BijiNoteObj *note,
                                      const char  *html);

  /*
   * Mandatory. Store the note. This might be async. */
  void          (*save_note)         (BijiNoteObj *note);

  /*
   * Mandatory
   * Move the note to trash bin. What to do depends on the provider
   * Return FALSE is this fails.
   * If the provider does not support trash,
   * you should delete the note */
  gboolean      (*archive)           (BijiNoteObj *note);

  /*
   * Mandatory
   * Return TRUE if the note can store rich text:
   * - bold, italic, srike
   */
  gboolean      (*can_format)        (BijiNoteObj *note);
  gboolean      (*restore)           (BijiNoteObj *note,
                                      char       **old_uuid);
  gboolean      (*delete)            (BijiNoteObj *note);
};

gpointer         biji_note_obj_get_manager                   (BijiNoteObj *self);
gboolean         biji_note_obj_trash                         (BijiNoteObj *self);
gboolean         biji_note_obj_has_notebook                  (BijiNoteObj *self,
                                                              const char  *label);
GList           *biji_note_obj_get_notebooks                 (BijiNoteObj *self);

void             biji_note_obj_save_note                     (BijiNoteObj *self);

char            *html_from_plain_text                        (const char *content);

G_END_DECLS
