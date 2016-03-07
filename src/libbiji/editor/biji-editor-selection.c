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
#include "biji-editor-utils.h"
 
#include <webkit/webkit.h>
#include <webkit/webkitdom.h>

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define E_EDITOR_SELECTION_GET_PRIVATE(obj) \
	(G_TYPE_INSTANCE_GET_PRIVATE \
	((obj), E_TYPE_EDITOR_SELECTION, EEditorSelectionPrivate))

struct _EEditorSelectionPrivate {

	WebKitWebView *webview;

	gchar *text;
};

G_DEFINE_TYPE (
	EEditorSelection,
	e_editor_selection,
	G_TYPE_OBJECT
);

enum {
	PROP_0,
	PROP_WEBVIEW,
	PROP_BOLD,
	PROP_BLOCK_FORMAT,
	PROP_ITALIC,
	PROP_STRIKE_THROUGH,
	PROP_TEXT,
};

static WebKitDOMRange *
editor_selection_get_current_range (EEditorSelection *selection)
{
  WebKitDOMDocument *document;
  WebKitDOMDOMWindow *window;
  WebKitDOMDOMSelection *dom_selection;

  document = webkit_web_view_get_dom_document (selection->priv->webview);
  window = webkit_dom_document_get_default_view (document);

  if (!window)
    return NULL;

  dom_selection = webkit_dom_dom_window_get_selection (window);
  
  if (webkit_dom_dom_selection_get_range_count (dom_selection) < 1)
    return NULL;

  return webkit_dom_dom_selection_get_range_at (dom_selection, 0, NULL);
}

static gboolean
get_has_style (EEditorSelection *selection, const gchar *style_tag)
{
  WebKitDOMNode *node;
  WebKitDOMElement *element;
  WebKitDOMRange *range;
  gboolean result;
  gint tag_len;

  range = editor_selection_get_current_range (selection);

  if (!range)
    return FALSE;

  node = webkit_dom_range_get_start_container (range, NULL);

  if (!WEBKIT_DOM_IS_ELEMENT (node))
    element = webkit_dom_node_get_parent_element (node);

  else
    element = WEBKIT_DOM_ELEMENT (node);

  tag_len = strlen (style_tag);
  result = FALSE;
  while (!result && element) {
    gchar *element_tag;

    element_tag = webkit_dom_element_get_tag_name (element);

		result = ((tag_len == strlen (element_tag)) &&
				(g_ascii_strncasecmp (element_tag, style_tag, tag_len) == 0));

		/* Special case: <blockquote type=cite> marks quotation, while
		 * just <blockquote> is used for indentation. If the <blockquote>
		 * has type=cite, then ignore it */
		if (result && g_ascii_strncasecmp (element_tag, "blockquote", 10) == 0) {
			if (webkit_dom_element_has_attribute (element, "type")) {
				gchar *type;
				type = webkit_dom_element_get_attribute (
						element, "type");
				if (g_ascii_strncasecmp (type, "cite", 4) == 0) {
					result = FALSE;
				}
				g_free (type);
			}
		}

		g_free (element_tag);

		if (result) {
			break;
		}

		element = webkit_dom_node_get_parent_element (
				WEBKIT_DOM_NODE (element));
	}

	return result;
}

static void
webview_selection_changed (WebKitWebView *webview,
			   EEditorSelection *selection)
{
	g_object_notify (G_OBJECT (selection), "bold");
	g_object_notify (G_OBJECT (selection), "block-format");
	g_object_notify (G_OBJECT (selection), "italic");
	g_object_notify (G_OBJECT (selection), "strike-through");
	g_object_notify (G_OBJECT (selection), "text");
}

static void
editor_selection_set_webview (EEditorSelection *selection,
			      WebKitWebView *webview)
{
	selection->priv->webview = g_object_ref (webview);
	g_signal_connect (
		webview, "selection-changed",
		G_CALLBACK (webview_selection_changed), selection);
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
			return;

		case PROP_BLOCK_FORMAT:
			g_value_set_int (value,
				e_editor_selection_get_block_format (selection));
			return;

		case PROP_ITALIC:
			g_value_set_boolean (value,
				e_editor_selection_get_italic (selection));
			return;

		case PROP_STRIKE_THROUGH:
			g_value_set_boolean (value,
				e_editor_selection_get_strike_through (selection));
			return;

		case PROP_TEXT:
			g_value_set_string (value,
				e_editor_selection_get_string (selection));
			break;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
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
			return;

		case PROP_BOLD:
			e_editor_selection_set_bold (
				selection, g_value_get_boolean (value));
			return;

		case PROP_BLOCK_FORMAT:
			e_editor_selection_set_block_format (
				selection, g_value_get_int (value));
			return;

		case PROP_ITALIC:
			e_editor_selection_set_italic (
				selection, g_value_get_boolean (value));
			return;

		case PROP_STRIKE_THROUGH:
			e_editor_selection_set_strike_through (
				selection, g_value_get_boolean (value));
			return;
	}

	G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
}

static void
e_editor_selection_finalize (GObject *object)
{
	EEditorSelection *selection = E_EDITOR_SELECTION (object);

	g_free (selection->priv->text);
	selection->priv->text = NULL;
}

static void
e_editor_selection_class_init (EEditorSelectionClass *klass)
{
	GObjectClass *object_class;

	g_type_class_add_private (klass, sizeof (EEditorSelectionPrivate));

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
		PROP_BLOCK_FORMAT,
		g_param_spec_int (
			"block-format",
			NULL,
			NULL,
			0,
			G_MAXINT,
			0,
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

	g_object_class_install_property (
		object_class,
		PROP_TEXT,
		g_param_spec_string (
			"text",
		       NULL,
		       NULL,
		       NULL,
		       G_PARAM_READABLE));
}


static void
e_editor_selection_init (EEditorSelection *selection)
{
	selection->priv = E_EDITOR_SELECTION_GET_PRIVATE (selection);
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
e_editor_selection_has_text (EEditorSelection *selection)
{
	WebKitDOMRange *range;
	WebKitDOMNode *node;

	g_return_val_if_fail (E_IS_EDITOR_SELECTION (selection), FALSE);

	range = editor_selection_get_current_range (selection);

	node = webkit_dom_range_get_start_container (range, NULL);
	if (webkit_dom_node_get_node_type (node) == 3) {
		return TRUE;
	}

	node = webkit_dom_range_get_end_container (range, NULL);
	if (webkit_dom_node_get_node_type (node) == 3) {
		return TRUE;
	}

	node = WEBKIT_DOM_NODE (webkit_dom_range_clone_contents (range, NULL));
	while (node) {
		if (webkit_dom_node_get_node_type (node) == 3) {
			return TRUE;
		}

		if (webkit_dom_node_has_child_nodes (node)) {
			node = webkit_dom_node_get_first_child (node);
		} else if (webkit_dom_node_get_next_sibling (node)) {
			node = webkit_dom_node_get_next_sibling (node);
		} else {
			node = webkit_dom_node_get_parent_node (node);
			if (node) {
				node = webkit_dom_node_get_next_sibling (node);
			}
		}
	}

	return FALSE;
}

const gchar *
e_editor_selection_get_string (EEditorSelection *selection)
{
	WebKitDOMRange *range;

	g_return_val_if_fail (E_IS_EDITOR_SELECTION (selection), NULL);

	range = editor_selection_get_current_range (selection);

	g_free (selection->priv->text);
	selection->priv->text = webkit_dom_range_get_text (range);

	return selection->priv->text;
}

EEditorSelectionBlockFormat
e_editor_selection_get_block_format (EEditorSelection *selection)
{
	WebKitDOMNode *node;
	WebKitDOMRange *range;
	WebKitDOMElement *element;
	EEditorSelectionBlockFormat result = E_EDITOR_SELECTION_BLOCK_FORMAT_NONE;

	g_return_val_if_fail (E_IS_EDITOR_SELECTION (selection),
			      E_EDITOR_SELECTION_BLOCK_FORMAT_NONE);

	range = editor_selection_get_current_range (selection);
	if (!range) {
		return result;
	}

	node = webkit_dom_range_get_start_container (range, NULL);

	if (e_editor_dom_node_find_parent_element (node, "UL")) {
		result = E_EDITOR_SELECTION_BLOCK_FORMAT_UNORDERED_LIST;
	} else if (e_editor_dom_node_find_parent_element (node, "OL")) {
                result = E_EDITOR_SELECTION_BLOCK_FORMAT_ORDERED_LIST;
	}

	return result;
}

void
e_editor_selection_set_block_format (EEditorSelection *selection,
				     EEditorSelectionBlockFormat format)
{
	EEditorSelectionBlockFormat current_format;
	WebKitDOMDocument *document;
	const gchar *command;
	const gchar *value;

	g_return_if_fail (E_IS_EDITOR_SELECTION (selection));

	current_format = e_editor_selection_get_block_format (selection);
	if (current_format == format) {
		return;
	}

	document = webkit_web_view_get_dom_document (selection->priv->webview);

	switch (format) {
		case E_EDITOR_SELECTION_BLOCK_FORMAT_ORDERED_LIST:
			command = "insertOrderedList";
			value = "";
			break;
		case E_EDITOR_SELECTION_BLOCK_FORMAT_UNORDERED_LIST:
			command = "insertUnorderedList";
			value = "";
			break;
		case E_EDITOR_SELECTION_BLOCK_FORMAT_NONE:
		default:
			command = "removeFormat";
			value = "";
			break;
	}


	/* First remove (un)ordered list before changing formatting */
	if (current_format == E_EDITOR_SELECTION_BLOCK_FORMAT_UNORDERED_LIST) {
		webkit_dom_document_exec_command (
			document, "insertUnorderedList", FALSE, "");
		/*		    ^-- not a typo, "insert" toggles the formatting
		 * 			if already present */
	} else if (current_format == E_EDITOR_SELECTION_BLOCK_FORMAT_ORDERED_LIST) {
		webkit_dom_document_exec_command (
			document, "insertOrderedList", FALSE ,"");
	}

	webkit_dom_document_exec_command (
		document, command, FALSE, value);

	g_object_notify (G_OBJECT (selection), "block-format");
}

gboolean
e_editor_selection_get_bold (EEditorSelection *selection)
{
	g_return_val_if_fail (E_IS_EDITOR_SELECTION (selection), FALSE);

	return get_has_style (selection, "b");
}

void
e_editor_selection_set_bold (EEditorSelection *selection,
			     gboolean bold)
{
	WebKitDOMDocument *document;

	g_return_if_fail (E_IS_EDITOR_SELECTION (selection));

	if ((e_editor_selection_get_bold (selection) ? TRUE : FALSE)
				== (bold ? TRUE : FALSE)) {
		return;
	}

	document = webkit_web_view_get_dom_document (selection->priv->webview);
	webkit_dom_document_exec_command (document, "bold", FALSE, "");

	g_object_notify (G_OBJECT (selection), "bold");
}

gboolean
e_editor_selection_get_italic (EEditorSelection *selection)
{
	g_return_val_if_fail (E_IS_EDITOR_SELECTION (selection), FALSE);

	return get_has_style (selection, "i");
}

void
e_editor_selection_set_italic (EEditorSelection *selection,
			       gboolean italic)
{
	WebKitDOMDocument *document;

	g_return_if_fail (E_IS_EDITOR_SELECTION (selection));

	if ((e_editor_selection_get_italic (selection) ? TRUE : FALSE)
				== (italic ? TRUE : FALSE)) {
		return;
	}

	document = webkit_web_view_get_dom_document (selection->priv->webview);
	webkit_dom_document_exec_command (document, "italic", FALSE, "");

	g_object_notify (G_OBJECT (selection), "italic");
}

gboolean
e_editor_selection_get_strike_through (EEditorSelection *selection)
{
	g_return_val_if_fail (E_IS_EDITOR_SELECTION (selection), FALSE);

	return get_has_style (selection, "strike");
}

void
e_editor_selection_set_strike_through (EEditorSelection *selection,
				       gboolean strike_through)
{
	WebKitDOMDocument *document;

	g_return_if_fail (E_IS_EDITOR_SELECTION (selection));

	if ((e_editor_selection_get_strike_through (selection) ? TRUE : FALSE)
				== (strike_through? TRUE : FALSE)) {
		return;
	}

	document = webkit_web_view_get_dom_document (selection->priv->webview);
	webkit_dom_document_exec_command (document, "strikeThrough", FALSE, "");

	g_object_notify (G_OBJECT (selection), "strike-through");
}
