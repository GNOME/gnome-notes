/*
 * e-editor-selection.h
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with the program; if not, see <http://www.gnu.org/licenses/>
 *
 */

#pragma once

#include <glib-object.h>
#include <webkit/webkit.h>

/* Standard GObject macros */
#define E_TYPE_EDITOR_SELECTION \
	(e_editor_selection_get_type ())
#define E_EDITOR_SELECTION(obj) \
	(G_TYPE_CHECK_INSTANCE_CAST \
	((obj), E_TYPE_EDITOR_SELECTION, EEditorSelection))
#define E_EDITOR_SELECTION_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_CAST \
	((cls), E_TYPE_EDITOR_SELECTION, EEditorSelectionClass))
#define E_IS_EDITOR_SELECTION(obj) \
	(G_TYPE_CHECK_INSTANCE_TYPE \
	((obj), E_TYPE_EDITOR_SELECTION))
#define E_IS_EDITOR_SELECTION_CLASS(cls) \
	(G_TYPE_CHECK_CLASS_TYPE \
	((cls), E_TYPE_EDITOR_SELECTION))
#define E_EDITOR_SELECTION_GET_CLASS(obj) \
	(G_TYPE_INSTANCE_GET_CLASS \
	((obj), E_TYPE_EDITOR_SELECTION, EEditorSelectionClass))

G_BEGIN_DECLS

typedef struct _EEditorSelection EEditorSelection;
typedef struct _EEditorSelectionClass EEditorSelectionClass;
typedef struct _EEditorSelectionPrivate EEditorSelectionPrivate;

struct _EEditorSelection {
	GObject parent;

	EEditorSelectionPrivate *priv;
};

struct _EEditorSelectionClass {
	GObjectClass parent_class;
};

GType			e_editor_selection_get_type 	(void);

EEditorSelection *	e_editor_selection_new		(WebKitWebView *parent_view);

void			e_editor_selection_set_bold	(EEditorSelection *selection,
							 gboolean bold);
gboolean		e_editor_selection_get_bold	(EEditorSelection *selection);

void			e_editor_selection_set_italic	(EEditorSelection *selection,
							 gboolean italic);
gboolean		e_editor_selection_get_italic	(EEditorSelection *selection);

void			e_editor_selection_set_strike_through
							(EEditorSelection *selection,
							 gboolean strike_through);
gboolean		e_editor_selection_get_strike_through
							(EEditorSelection *selection);

G_END_DECLS
