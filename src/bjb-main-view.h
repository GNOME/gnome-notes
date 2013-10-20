/* bjb-main-view.h
 * Copyright © 2012, 2013 Pierre-Yves LUYTEN <py@luyten.fr>
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

#ifndef _MAIN_VIEW_H_
#define _MAIN_VIEW_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libbiji/libbiji.h>

#include "bjb-controller.h"
#include "bjb-search-toolbar.h"

G_BEGIN_DECLS

#define BJB_TYPE_MAIN_VIEW             (bjb_main_view_get_type ())
#define BJB_MAIN_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BJB_TYPE_MAIN_VIEW, BjbMainView))
#define BJB_MAIN_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BJB_TYPE_MAIN_VIEW, BjbMainViewClass))
#define BJB_IS_MAIN_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BJB_TYPE_MAIN_VIEW))
#define BJB_IS_MAIN_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BJB_TYPE_MAIN_VIEW))
#define BJB_MAIN_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BJB_TYPE_MAIN_VIEW, BjbMainViewClass))

typedef struct _BjbMainViewClass BjbMainViewClass;
typedef struct _BjbMainView BjbMainView;

typedef struct _BjbMainViewPriv BjbMainViewPriv;

struct _BjbMainViewClass
{
  GtkGridClass parent_class;
};

struct _BjbMainView
{
  GtkGrid parent_instance;
  BjbMainViewPriv *priv;
};

GType bjb_main_view_get_type (void) G_GNUC_CONST;

BjbMainView * bjb_main_view_new(GtkWidget *win, BjbController *controller);

void bjb_main_view_connect_signals (BjbMainView *self);

GtkWidget *bjb_main_view_get_window(BjbMainView *view);

void action_new_window_callback(GtkAction *action, gpointer bjb_main_view);



GList *bjb_main_view_get_selected_items (BjbMainView *view);


gboolean bjb_main_view_get_iter_at_note (BjbMainView  *view,
                                         BijiNoteObj  *note,
                                         GtkTreeIter  *retval);

void update_notes_with_tag_search(BjbMainView *view, gchar *tag);

void update_notes_with_string_search(BjbMainView *view, gchar *needle);

void switch_to_note_view(BjbMainView *view,BijiNoteObj *note) ;

void bjb_main_view_update_model (BjbMainView *view);

/* bridge for notes view (GdMainView)
 * TODO : get rid of this it's a bit idiot */

gboolean bjb_main_view_get_selection_mode (BjbMainView *view);

void bjb_main_view_set_selection_mode (BjbMainView *view, gboolean mode);

GdMainViewType bjb_main_view_get_view_type (BjbMainView *view);

void bjb_main_view_set_view_type (BjbMainView *view, GdMainViewType type);



G_END_DECLS

#endif /* _MAIN_VIEW_H_ */
