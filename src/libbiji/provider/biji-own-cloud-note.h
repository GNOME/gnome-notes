/* bjb-own-cloud-note.c
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

#ifndef BIJI_OWN_CLOUD_NOTE_H_
#define BIJI_OWN_CLOUD_NOTE_H_ 1

#include "../biji-note-obj.h"
#include "../biji-note-id.h"

#include "biji-own-cloud-provider.h"

G_BEGIN_DECLS


#define BIJI_TYPE_OWN_CLOUD_NOTE             (biji_own_cloud_note_get_type ())
#define BIJI_OWN_CLOUD_NOTE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_OWN_CLOUD_NOTE, BijiOwnCloudNote))
#define BIJI_OWN_CLOUD_NOTE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_OWN_CLOUD_NOTE, BijiOwnCloudNoteClass))
#define BIJI_IS_OWN_CLOUD_NOTE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_OWN_CLOUD_NOTE))
#define BIJI_IS_OWN_CLOUD_NOTE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_OWN_CLOUD_NOTE))
#define BIJI_OWN_CLOUD_NOTE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_OWN_CLOUD_NOTE, BijiOwnCloudNoteClass))

typedef struct BijiOwnCloudNote_         BijiOwnCloudNote;
typedef struct BijiOwnCloudNoteClass_    BijiOwnCloudNoteClass;
typedef struct BijiOwnCloudNotePrivate_  BijiOwnCloudNotePrivate;

struct BijiOwnCloudNote_
{
  BijiNoteObj parent;
  BijiOwnCloudNotePrivate *priv;
};

struct BijiOwnCloudNoteClass_
{
  BijiNoteObjClass parent_class;
};


GType              biji_own_cloud_note_get_type                (void);


BijiNoteObj        *biji_own_cloud_note_new_from_info           (BijiOwnCloudProvider *prov,
                                                                 BijiManager *manager,
                                                                 BijiInfoSet *info);

G_END_DECLS

#endif /* BIJI_OWN_CLOUD_NOTE_H_ */
