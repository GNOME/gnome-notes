/* bjb-local-note.h
 * Copyright (C) Pierre-Yves LUYTEN 2013 <py@luyten.fr>
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


#ifndef BIJI_LOCAL_NOTE_H_
#define BIJI_LOCAL_NOTE_H_ 1

#include "biji-note-id.h"
#include "biji-note-obj.h"

G_BEGIN_DECLS


#define BIJI_TYPE_LOCAL_NOTE             (biji_local_note_get_type ())
#define BIJI_LOCAL_NOTE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_LOCAL_NOTE, BijiLocalNote))
#define BIJI_LOCAL_NOTE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_LOCAL_NOTE, BijiLocalNoteClass))
#define BIJI_IS_LOCAL_NOTE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_LOCAL_NOTE))
#define BIJI_IS_LOCAL_NOTE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_LOCAL_NOTE))
#define BIJI_LOCAL_NOTE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_LOCAL_NOTE, BijiLocalNoteClass))

typedef struct BijiLocalNote_         BijiLocalNote;
typedef struct BijiLocalNoteClass_    BijiLocalNoteClass;
typedef struct BijiLocalNotePrivate_  BijiLocalNotePrivate;

struct BijiLocalNote_
{
  BijiNoteObj parent;
  BijiLocalNotePrivate *priv;
};

struct BijiLocalNoteClass_
{
  BijiNoteObjClass parent_class;
};


GType                 biji_local_note_get_type        (void);


BijiNoteObj          *biji_local_note_new_from_info   (BijiProvider *provider,
                                                       BijiManager *manager,
                                                       BijiInfoSet *set);

G_END_DECLS

#endif /* BIJI_LOCAL_NOTE_H_ */
