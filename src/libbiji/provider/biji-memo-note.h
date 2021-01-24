/* bjb-memo-note.h
 * Copyright (C) Pierre-Yves LUYTEN 2014 <py@luyten.fr>
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

#include <libecal/libecal.h>
#include "biji-note-obj.h"
#include "biji-memo-provider.h"

G_BEGIN_DECLS

#define BIJI_TYPE_MEMO_NOTE (biji_memo_note_get_type())

G_DECLARE_FINAL_TYPE (BijiMemoNote, biji_memo_note, BIJI, MEMO_NOTE, BijiNoteObj)

BijiNoteObj           *biji_memo_note_new_from_info          (BijiMemoProvider *provider,
                                                              BijiManager      *manager,
                                                              BijiInfoSet      *info,
                                                              ECalComponent    *comp,
                                                              const char       *description,
                                                              ECalClient       *client);

G_END_DECLS

