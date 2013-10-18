/* bjb-search-toolbar.h
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

#ifndef BJB_SEARCH_TOOLBAR_H
#define BJB_SEARCH_TOOLBAR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BJB_TYPE_SEARCH_TOOLBAR (bjb_search_toolbar_get_type ())

#define BJB_SEARCH_TOOLBAR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BJB_TYPE_SEARCH_TOOLBAR, BjbSearchToolbar))

#define BJB_SEARCH_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), BJB_TYPE_SEARCH_TOOLBAR, BjbSearchToolbarClass))

#define BJB_IS_SEARCH_TOOLBAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BJB_TYPE_SEARCH_TOOLBAR))

#define BJB_IS_SEARCH_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BJB_TYPE_SEARCH_TOOLBAR))

#define BJB_SEARCH_TOOLBAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BJB_TYPE_SEARCH_TOOLBAR, BjbSearchToolbarClass))

typedef struct _BjbSearchToolbar        BjbSearchToolbar;
typedef struct _BjbSearchToolbarClass   BjbSearchToolbarClass;
typedef struct _BjbSearchToolbarPrivate BjbSearchToolbarPrivate;

struct _BjbSearchToolbar
{
  GtkSearchBar parent_instance;
  BjbSearchToolbarPrivate *priv;
};

struct _BjbSearchToolbarClass
{
  GtkSearchBarClass parent_class;
};


GType              bjb_search_toolbar_get_type              (void) G_GNUC_CONST;


BjbSearchToolbar  *bjb_search_toolbar_new                   (GtkWidget *window,
                                                             BjbController *controller);


void               bjb_search_toolbar_fade_in               (BjbSearchToolbar *self);


void               bjb_search_toolbar_fade_out              (BjbSearchToolbar *self);


void               bjb_search_toolbar_disconnect            (BjbSearchToolbar *self);


void               bjb_search_toolbar_connect               (BjbSearchToolbar *self);



G_END_DECLS

#endif /* BJB_SEARCH_TOOLBAR_H */
