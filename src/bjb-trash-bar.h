/* bjb-trash-bar.h
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


#ifndef BJB_TRASH_BAR_H_
#define BJB_TRASH_BAR_H_ 1

#include "config.h"

#include <gtk/gtk.h>
#include <libgd/gd.h>

#include "bjb-controller.h"

G_BEGIN_DECLS


#define BJB_TYPE_TRASH_BAR             (bjb_trash_bar_get_type ())
#define BJB_TRASH_BAR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BJB_TYPE_TRASH_BAR, BjbTrashBar))
#define BJB_TRASH_BAR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BJB_TYPE_TRASH_BAR, BjbTrashBarClass))
#define BJB_IS_TRASH_BAR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BJB_TYPE_TRASH_BAR))
#define BJB_IS_TRASH_BAR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BJB_TYPE_TRASH_BAR))
#define BJB_TRASH_BAR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BJB_TYPE_TRASH_BAR, BjbTrashBarClass))

typedef struct BjbTrashBar_         BjbTrashBar;
typedef struct BjbTrashBarClass_    BjbTrashBarClass;
typedef struct BjbTrashBarPrivate_  BjbTrashBarPrivate;

struct BjbTrashBar_
{
  GtkBox parent;
  BjbTrashBarPrivate *priv;
};

struct BjbTrashBarClass_
{
  GtkBoxClass parent_class;
};


GType                 bjb_trash_bar_get_type            (void);


BjbTrashBar *         bjb_trash_bar_new                 (BjbController *controller,
                                                         BjbMainView   *view,
                                                         GdMainView    *selection);


void                  bjb_trash_bar_set_visibility      (BjbTrashBar *self);


void                  bjb_trash_bar_fade_in             (BjbTrashBar *self);


void                  bjb_trash_bar_fade_out            (BjbTrashBar *self);


G_END_DECLS

#endif /* BJB_TRASH_BAR_H_ */
