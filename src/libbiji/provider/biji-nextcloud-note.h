/* biji-nextcloud-note.h
 *
 * Copyright 2020 Isaque Galdino <igaldino@gmail.com>
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
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "biji-note-obj.h"
#include "biji-nextcloud-provider.h"

G_BEGIN_DECLS

#define BIJI_TYPE_NEXTCLOUD_NOTE (biji_nextcloud_note_get_type ())

G_DECLARE_FINAL_TYPE (BijiNextcloudNote, biji_nextcloud_note, BIJI, NEXTCLOUD_NOTE, BijiNoteObj)

BijiNoteObj *biji_nextcloud_note_new (BijiNextcloudProvider *provider,
                                      BijiManager           *manager,
                                      gint64                 id,
                                      const char            *title,
                                      gint64                 mtime,
                                      const char            *content);

G_END_DECLS
