/* bjb-note.h
 *
 * Copyright 2023 Mohammed Sadiq <sadiq@sadiqpk.org>
 * Copyright 2023 Purism SPC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s):
 *   Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <gtk/gtk.h>

#include "bjb-item.h"
#include "bjb-tag-store.h"

G_BEGIN_DECLS

#define BJB_TYPE_NOTE (bjb_note_get_type ())
G_DECLARE_DERIVABLE_TYPE (BjbNote, bjb_note, BJB, NOTE, BjbItem)

struct _BjbNoteClass
{
  BjbItemClass parent_class;

  char        *(*get_text_content)        (BjbNote        *self);
  void         (*set_text_content)        (BjbNote        *self,
                                          const char     *content);
  char        *(*get_raw_content)         (BjbNote        *self);
  void         (*set_raw_content)         (BjbNote        *self,
                                          const char     *content);

  BjbTagStore *(*get_tag_store)           (BjbNote        *self);
  BjbItem     *(*get_notebook)            (BjbNote        *self);
  const char  *(*get_extension)           (BjbNote        *self);
};

char         *bjb_note_get_text_content        (BjbNote         *self);
void          bjb_note_set_text_content        (BjbNote         *self,
                                                const char      *content);
char         *bjb_note_get_raw_content         (BjbNote         *self);
void          bjb_note_set_raw_content         (BjbNote         *self,
                                                const char      *content);
char         *bjb_note_get_html                (BjbNote         *self);
void          bjb_note_set_html                (BjbNote         *self,
                                                const char      *html);
void          bjb_note_add_tag                 (BjbNote         *self,
                                                const char      *tag_title);
void          bjb_note_remove_tag              (BjbNote         *self,
                                                BjbItem         *tag);
void          bjb_note_set_notebook            (BjbNote         *self,
                                                BjbItem         *notebook);
BjbItem      *bjb_note_get_notebook            (BjbNote         *self);
GListModel   *bjb_note_get_tags                (BjbNote         *self);
const char   *bjb_note_get_file_extension      (BjbNote         *self);

G_END_DECLS
