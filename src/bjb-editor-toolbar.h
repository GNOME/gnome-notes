/* bjb-editor-toolbar.h
 * Copyright © 2012, 2013 Red Hat, Inc.
 * Copyright © 2013, 2014 Pierre-Yves LUYTEN <py@luyten.fr>
 * Copyright © 2017 Iñigo Martínez <inigomartinez@gmail.com>
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

#include "bjb-note-view.h"

G_BEGIN_DECLS

#define BJB_TYPE_EDITOR_TOOLBAR (bjb_editor_toolbar_get_type ())

G_DECLARE_FINAL_TYPE (BjbEditorToolbar, bjb_editor_toolbar, BJB, EDITOR_TOOLBAR, GtkBox)

void     bjb_editor_toolbar_set_can_format (BjbEditorToolbar *self,
                                            gboolean          can_format);

G_END_DECLS
