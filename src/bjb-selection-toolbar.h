/* bjb-selection-toolbar.h
 * Copyright © 2012, 2013 Red Hat, Inc.
 * Copyright © 2013 Pierre-Yves LUYTEN <py@luyten.fr>
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

#ifndef BJB_SELECTION_TOOLBAR_H
#define BJB_SELECTION_TOOLBAR_H

#include <libgd/gd.h>

G_BEGIN_DECLS

#define BJB_TYPE_SELECTION_TOOLBAR (bjb_selection_toolbar_get_type ())

G_DECLARE_FINAL_TYPE (BjbSelectionToolbar, bjb_selection_toolbar, BJB, SELECTION_TOOLBAR, GtkRevealer)

BjbSelectionToolbar * bjb_selection_toolbar_new (GdMainView   *selection,
                                                 BjbMainView  *bjb_main_view);

G_END_DECLS

#endif /* BJB_SELECTION_TOOLBAR_H */
