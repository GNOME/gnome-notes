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

#include "biji-info-set.h"
#include "biji-item.h"

G_BEGIN_DECLS

/* Available formating for biji_note_obj_editor_apply_format
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

G_DECLARE_DERIVABLE_TYPE (BijiNoteObj, biji_note_obj, BIJI, NOTE_OBJ, BijiItem)

struct _BijiNoteObjClass
{
  BijiItemClass parent_class;

  char         *(*get_basename)      (BijiNoteObj *note);

  /*
   * Mandatory. Provide the latest note html.
   * Use html_from_plain_text if needed.
   * This sring must be allocated. use g_strdup if needed. */
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
   * Return TRUE if the note is marked as trashed in any way
   * (depending on the provider)
   *
   * Return FALSE if the note is note trashed.
   *
   * If the provider does not suport this, return FALSE */
  gboolean      (*is_trashed)        (BijiNoteObj *note);

  /*
   * Mandatory
   * Return TRUE if the note can store rich text:
   * - bold, italic, srike
   */
  gboolean      (*can_format)        (BijiNoteObj *note);
};

gboolean         biji_note_obj_are_same                      (BijiNoteObj *note_a,
                                                              BijiNoteObj *note_b);

gboolean         biji_note_obj_set_mtime                     (BijiNoteObj *self,
                                                              gint64       time);

char            *biji_note_obj_get_last_change_date_string   (BijiNoteObj *self);

gint64           biji_note_obj_get_last_metadata_change_date (BijiNoteObj *self);

gboolean         biji_note_obj_set_last_metadata_change_date (BijiNoteObj *self,
                                                              gint64       time);

gint64           biji_note_obj_get_create_date               (BijiNoteObj *self);

gboolean         biji_note_obj_set_create_date               (BijiNoteObj *self,
                                                              gint64       time);

void             biji_note_obj_set_all_dates_now             (BijiNoteObj *self);

gboolean         biji_note_obj_get_rgba                      (BijiNoteObj *self,
                                                              GdkRGBA     *rgba);

void             biji_note_obj_set_rgba                      (BijiNoteObj   *self,
                                                              const GdkRGBA *rgba);

GList           *biji_note_obj_get_notebooks                 (BijiNoteObj *self);

gboolean         biji_note_obj_is_trashed                    (BijiNoteObj *self);

void             biji_note_obj_save_note                     (BijiNoteObj *self);

void             biji_note_obj_set_icon                      (BijiNoteObj *self,
                                                              GdkPixbuf   *pix);

char            *biji_note_obj_get_icon_file                 (BijiNoteObj *self);

const char      *biji_note_obj_get_raw_text                  (BijiNoteObj *self);

void             biji_note_obj_set_raw_text                  (BijiNoteObj *self,
                                                              const char  *plain_text);

const char      *biji_note_obj_get_path                      (BijiNoteObj *self);

void             biji_note_obj_set_path                      (BijiNoteObj *self,
                                                              const char  *path);

const char      *biji_note_obj_get_title                     (BijiNoteObj *self);

gboolean         biji_note_obj_set_title                     (BijiNoteObj *self,
                                                              const char  *title);

gboolean         biji_note_obj_is_template                   (BijiNoteObj *self);

void             biji_note_obj_set_is_template               (BijiNoteObj *self,
                                                              gboolean     is_template);

GtkWidget       *biji_note_obj_open                          (BijiNoteObj *self);

gboolean         biji_note_obj_is_opened                     (BijiNoteObj *self);

GtkWidget       *biji_note_obj_get_editor                    (BijiNoteObj *self);

gboolean         biji_note_obj_can_format                    (BijiNoteObj *self);

char            *html_from_plain_text                        (const char *content);

void             biji_note_obj_set_html                      (BijiNoteObj *self,
                                                              const char  *html);

char            *biji_note_obj_get_html                      (BijiNoteObj *self);

void             biji_note_obj_editor_apply_format           (BijiNoteObj *self,
                                                              int          format);

gboolean         biji_note_obj_editor_has_selection          (BijiNoteObj *self);

const char      *biji_note_obj_editor_get_selection          (BijiNoteObj *self);

void             biji_note_obj_editor_cut                    (BijiNoteObj *self);

void             biji_note_obj_editor_copy                   (BijiNoteObj *self);

void             biji_note_obj_editor_paste                  (BijiNoteObj *self);

G_END_DECLS
