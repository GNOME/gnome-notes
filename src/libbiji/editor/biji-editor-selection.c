/*
 * e-editor-selection.c
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "biji-editor-selection.h"

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

struct _EEditorSelectionPrivate {
	WebKitWebView *webview;
        WebKitEditorTypingAttributes attrs;
};

G_DEFINE_TYPE_WITH_PRIVATE (
	EEditorSelection,
	e_editor_selection,
	G_TYPE_OBJECT
);

enum {
	PROP_0,
	PROP_WEBVIEW,
	PROP_BOLD,
	PROP_ITALIC,
	PROP_STRIKE_THROUGH,
};

static void
check_and_update_typing_attr (EEditorSelection *selection,
                              WebKitEditorTypingAttributes attrs,
                              unsigned attr,
                              const char *poperty_name)
{
        if (attrs & attr) {
                if (!(selection->priv->attrs & attr)) {
                        selection->priv->attrs |= attr;
                        g_object_notify (G_OBJECT (selection), poperty_name);
                }
        } else if (!(attrs & attr)) {
                if (selection->priv->attrs & attr) {
                        selection->priv->attrs &= ~attr;
                        g_object_notify (G_OBJECT (selection), poperty_name);
                }
        }
}

static void
webview_typing_attributes_changed (WebKitEditorState *editor,
                                   GParamSpec *spec,
                                   EEditorSelection *selection)
{
        WebKitEditorTypingAttributes attrs;

        attrs = webkit_editor_state_get_typing_attributes (editor);

        g_object_freeze_notify (G_OBJECT (selection));
        check_and_update_typing_attr (selection, attrs, WEBKIT_EDITOR_TYPING_ATTRIBUTE_BOLD, "bold");
        check_and_update_typing_attr (selection, attrs, WEBKIT_EDITOR_TYPING_ATTRIBUTE_ITALIC, "italic");
        check_and_update_typing_attr (selection, attrs, WEBKIT_EDITOR_TYPING_ATTRIBUTE_STRIKETHROUGH, "strike-through");
        g_object_thaw_notify (G_OBJECT (selection));
}

static void
editor_selection_set_webview (EEditorSelection *selection,
			      WebKitWebView *webview)
{
        WebKitEditorState *editor;

        g_clear_object (&selection->priv->webview);
	selection->priv->webview = g_object_ref (webview);

        editor = webkit_web_view_get_editor_state (webview);
        selection->priv->attrs = webkit_editor_state_get_typing_attributes (editor);
        g_signal_connect (
                editor, "notify::typing-attributes",
                G_CALLBACK (webview_typing_attributes_changed), selection);
}


static void
e_editor_selection_get_property (GObject *object,
				 guint property_id,
				 GValue *value,
				 GParamSpec *pspec)
{
	EEditorSelection *selection = E_EDITOR_SELECTION (object);

	switch (property_id) {
		case PROP_BOLD:
			g_value_set_boolean (value,
				e_editor_selection_get_bold (selection));
			break;

		case PROP_ITALIC:
			g_value_set_boolean (value,
				e_editor_selection_get_italic (selection));
			break;

		case PROP_STRIKE_THROUGH:
			g_value_set_boolean (value,
				e_editor_selection_get_strike_through (selection));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
			break;
	}
}

static void
e_editor_selection_set_property (GObject *object,
				 guint property_id,
				 const GValue *value,
				 GParamSpec *pspec)
{
	EEditorSelection *selection = E_EDITOR_SELECTION (object);

	switch (property_id) {
		case PROP_WEBVIEW:
			editor_selection_set_webview (
				selection, g_value_get_object (value));
			break;

		case PROP_BOLD:
			e_editor_selection_set_bold (
				selection, g_value_get_boolean (value));
			break;

		case PROP_ITALIC:
			e_editor_selection_set_italic (
				selection, g_value_get_boolean (value));
			break;

		case PROP_STRIKE_THROUGH:
			e_editor_selection_set_strike_through (
				selection, g_value_get_boolean (value));
			break;

		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
e_editor_selection_finalize (GObject *object)
{
	EEditorSelection *selection = E_EDITOR_SELECTION (object);

        g_object_unref (selection->priv->webview);

        G_OBJECT_CLASS (e_editor_selection_parent_class)->finalize (object);
}

static void
e_editor_selection_class_init (EEditorSelectionClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->get_property = e_editor_selection_get_property;
	object_class->set_property = e_editor_selection_set_property;
	object_class->finalize = e_editor_selection_finalize;

	g_object_class_install_property (
		object_class,
		PROP_WEBVIEW,
		g_param_spec_object (
			"webview",
			NULL,
			NULL,
		        WEBKIT_TYPE_WEB_VIEW,
		        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));

	g_object_class_install_property (
		object_class,
		PROP_BOLD,
		g_param_spec_boolean (
			"bold",
			NULL,
			NULL,
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_ITALIC,
		g_param_spec_boolean (
			"italic",
			NULL,
			NULL,
			FALSE,
			G_PARAM_READWRITE));

	g_object_class_install_property (
		object_class,
		PROP_STRIKE_THROUGH,
		g_param_spec_boolean (
			"strike-through",
			NULL,
			NULL,
			FALSE,
			G_PARAM_READWRITE));
}


static void
e_editor_selection_init (EEditorSelection *selection)
{
	selection->priv = e_editor_selection_get_instance_private (selection);
}

EEditorSelection *
e_editor_selection_new (WebKitWebView *parent_view)
{
	g_return_val_if_fail (WEBKIT_IS_WEB_VIEW (parent_view), NULL);

	return g_object_new (
			E_TYPE_EDITOR_SELECTION,
			"webview", parent_view, NULL);
}

gboolean
e_editor_selection_get_bold (EEditorSelection *selection)
{
	g_return_val_if_fail (E_IS_EDITOR_SELECTION (selection), FALSE);

	return selection->priv->attrs & WEBKIT_EDITOR_TYPING_ATTRIBUTE_BOLD;
}

void
e_editor_selection_set_bold (EEditorSelection *selection,
			     gboolean bold)
{
	g_return_if_fail (E_IS_EDITOR_SELECTION (selection));

	if ((e_editor_selection_get_bold (selection) ? TRUE : FALSE)
				== (bold ? TRUE : FALSE)) {
		return;
	}

        webkit_web_view_execute_editing_command (selection->priv->webview, "Bold");
}

gboolean
e_editor_selection_get_italic (EEditorSelection *selection)
{
	g_return_val_if_fail (E_IS_EDITOR_SELECTION (selection), FALSE);

	return selection->priv->attrs & WEBKIT_EDITOR_TYPING_ATTRIBUTE_ITALIC;
}

void
e_editor_selection_set_italic (EEditorSelection *selection,
			       gboolean italic)
{
	g_return_if_fail (E_IS_EDITOR_SELECTION (selection));

	if ((e_editor_selection_get_italic (selection) ? TRUE : FALSE)
				== (italic ? TRUE : FALSE)) {
		return;
	}

        webkit_web_view_execute_editing_command (selection->priv->webview, "Italic");
}

gboolean
e_editor_selection_get_strike_through (EEditorSelection *selection)
{
	g_return_val_if_fail (E_IS_EDITOR_SELECTION (selection), FALSE);

	return selection->priv->attrs & WEBKIT_EDITOR_TYPING_ATTRIBUTE_STRIKETHROUGH;
}

void
e_editor_selection_set_strike_through (EEditorSelection *selection,
				       gboolean strike_through)
{
	g_return_if_fail (E_IS_EDITOR_SELECTION (selection));

	if ((e_editor_selection_get_strike_through (selection) ? TRUE : FALSE)
				== (strike_through? TRUE : FALSE)) {
		return;
	}

        webkit_web_view_execute_editing_command (selection->priv->webview, "Strikethrough");
}
