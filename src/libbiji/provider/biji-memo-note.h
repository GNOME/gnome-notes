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

#ifndef _BIJI_MEMO_NOTE_H_
#define _BIJI_MEMO_NOTE_H_ 1

#include <libecal/libecal.h>               /* ECalClient      */

#include "biji-tracker.h"
#include "biji-note-id.h"
#include "biji-note-obj.h"


G_BEGIN_DECLS

#define BIJI_TYPE_MEMO_NOTE             (biji_memo_note_get_type ())
#define BIJI_MEMO_NOTE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_MEMO_NOTE, BijiMemoNote))
#define BIJI_MEMO_NOTE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_MEMO_NOTE, BijiMemoNoteClass))
#define BIJI_IS_MEMO_NOTE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_MEMO_NOTE))
#define BIJI_IS_MEMO_NOTE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_MEMO_NOTE))
#define BIJI_MEMO_NOTE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_MEMO_NOTE, BijiMemoNoteClass))

typedef struct _BijiMemoNoteClass BijiMemoNoteClass;
typedef struct _BijiMemoNote BijiMemoNote;
typedef struct _BijiMemoNotePrivate BijiMemoNotePrivate;


struct _BijiMemoNote
{
    BijiNoteObj parent_instance;

    BijiMemoNotePrivate *priv;
};


struct _BijiMemoNoteClass
{
    BijiNoteObjClass parent_class;
};



GType                  biji_memo_note_get_type               (void) G_GNUC_CONST;


BijiNoteObj           *biji_memo_note_new_from_info          (BijiMemoProvider *provider,
                                                              BijiManager      *manager,
                                                              BijiInfoSet      *info,
							      ECalComponent    *comp,
							      gchar            *description,
                                                              ECalClient       *client);


G_END_DECLS

#endif /* _BIJI_MEMO_NOTE_H_ */

