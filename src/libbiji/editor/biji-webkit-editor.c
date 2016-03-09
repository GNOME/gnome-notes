/* biji-webkit-editor.c
 * Copyright (C) Pierre-Yves LUYTEN 2012 <py@luyten.fr>
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

#include <libxml/xmlwriter.h>

#include "config.h"
#include "../biji-string.h"
#include "../biji-manager.h"
#include "biji-webkit-editor.h"
#include "biji-editor-selection.h"
#include <JavaScriptCore/JavaScript.h>

/* Prop */
enum {
  PROP_0,
  PROP_NOTE,
  NUM_PROP
};

/* Signals */
enum {
  EDITOR_CLOSED,
  CONTENT_CHANGED,
  EDITOR_SIGNALS
};

/* Block Format */
typedef enum {
  BLOCK_FORMAT_NONE,
  BLOCK_FORMAT_UNORDERED_LIST,
  BLOCK_FORMAT_ORDERED_LIST
} BlockFormat;

static guint biji_editor_signals [EDITOR_SIGNALS] = { 0 };

static GParamSpec *properties[NUM_PROP] = { NULL, };

struct _BijiWebkitEditorPrivate
{
  BijiNoteObj *note;
  gulong content_changed;
  gulong color_changed;
  gboolean has_text;
  gchar *selected_text;
  BlockFormat block_format;

  EEditorSelection *sel;
};

G_DEFINE_TYPE (BijiWebkitEditor, biji_webkit_editor, WEBKIT_TYPE_WEB_VIEW);

gboolean
biji_webkit_editor_has_selection (BijiWebkitEditor *self)
{
  BijiWebkitEditorPrivate *priv = self->priv;

  return priv->has_text && priv->selected_text && *priv->selected_text;
}

const gchar *
biji_webkit_editor_get_selection (BijiWebkitEditor *self)
{
  return self->priv->selected_text;
}

static WebKitWebContext *
biji_webkit_editor_get_web_context (void)
{
  static WebKitWebContext *web_context = NULL;

  if (!web_context)
  {
    web_context = webkit_web_context_get_default ();
    webkit_web_context_set_cache_model (web_context, WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
    webkit_web_context_set_process_model (web_context, WEBKIT_PROCESS_MODEL_SHARED_SECONDARY_PROCESS);
    webkit_web_context_set_spell_checking_enabled (web_context, TRUE);
  }

  return web_context;
}

static WebKitSettings *
biji_webkit_editor_get_web_settings (void)
{
  static WebKitSettings *settings = NULL;

  if (!settings)
  {
    settings = webkit_settings_new_with_settings (
      "enable-page-cache", FALSE,
      "enable-plugins", FALSE,
      "enable-tabs-to-links", FALSE,
      "allow-file-access-from-file-urls", TRUE,
      NULL);
  }

  return settings;
}

typedef gboolean GetFormatFunc (EEditorSelection*);
typedef void     SetFormatFunc (EEditorSelection*, gboolean);

static void
biji_toggle_format (EEditorSelection *sel,
                    GetFormatFunc get_format,
                    SetFormatFunc set_format)
{
  set_format (sel, !get_format (sel));
}

static void
biji_toggle_block_format (BijiWebkitEditor *self,
                          BlockFormat block_format)
{
  const char *command = NULL;

  /* insert commands toggle the formatting */
  switch (block_format) {
  case BLOCK_FORMAT_UNORDERED_LIST:
    command = "insertUnorderedList";
    break;
  case BLOCK_FORMAT_ORDERED_LIST:
    command = "insertOrderedList";
    break;
  default:
    g_assert_not_reached ();
  }

  webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), command);
}

void
biji_webkit_editor_apply_format (BijiWebkitEditor *self, gint format)
{
  BijiWebkitEditorPrivate *priv = self->priv;

  if (priv->has_text)
  {
    switch (format)
    {
      case BIJI_BOLD:
        biji_toggle_format (priv->sel, e_editor_selection_get_bold,
                                       e_editor_selection_set_bold);
        break;

      case BIJI_ITALIC:
        biji_toggle_format (priv->sel, e_editor_selection_get_italic,
                                       e_editor_selection_set_italic);
        break;

      case BIJI_STRIKE:
        biji_toggle_format (priv->sel, e_editor_selection_get_strike_through,
                                       e_editor_selection_set_strike_through);
        break;

      case BIJI_BULLET_LIST:
        biji_toggle_block_format (self, BLOCK_FORMAT_UNORDERED_LIST);
        break;

      case BIJI_ORDER_LIST:
        biji_toggle_block_format (self, BLOCK_FORMAT_ORDERED_LIST);
        break;

      default:
        g_warning ("biji_webkit_editor_apply_format : Invalid format");
    }
  }
}

void
biji_webkit_editor_cut (BijiWebkitEditor *self)
{
  webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), WEBKIT_EDITING_COMMAND_CUT);
}

void
biji_webkit_editor_copy (BijiWebkitEditor *self)
{
  webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), WEBKIT_EDITING_COMMAND_COPY);
}

void
biji_webkit_editor_paste (BijiWebkitEditor *self)
{
  webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), WEBKIT_EDITING_COMMAND_PASTE);
}

void
biji_webkit_editor_undo (BijiWebkitEditor *self)
{
  webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), WEBKIT_EDITING_COMMAND_UNDO);
}

void
biji_webkit_editor_redo (BijiWebkitEditor *self)
{
  webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), WEBKIT_EDITING_COMMAND_REDO);
}

static void
set_editor_color (WebKitWebView *w, GdkRGBA *col)
{
  gchar *script;

  webkit_web_view_set_background_color (w, col);
  script = g_strdup_printf ("document.getElementById('editable').style.color = '%s';",
                            col->red < 0.5 ? "white" : "black");
  webkit_web_view_run_javascript (w, script, NULL, NULL, NULL);
  g_free (script);
}

void
biji_webkit_editor_set_font (BijiWebkitEditor *self, gchar *font)
{
  PangoFontDescription *font_desc;

  /* parse : but we only parse font properties we'll be able
   * to transfer to webkit editor
   * Maybe is there a better way than webkitSettings,
   * eg applying format to the whole body */
  font_desc = pango_font_description_from_string (font);
  const gchar * family = pango_font_description_get_family (font_desc);
  gint size = pango_font_description_get_size (font_desc);

  if (!pango_font_description_get_size_is_absolute (font_desc))
    size /= PANGO_SCALE;

  GdkScreen *screen = gtk_widget_get_screen (GTK_WIDGET (self));
  double dpi = screen ? gdk_screen_get_resolution (screen) : 96.0;
  guint font_size = size / 72. * dpi;

  /* Set */
  g_object_set (biji_webkit_editor_get_web_settings (),
                "default-font-family", family,
                "default-font-size", font_size,
                NULL);

  pango_font_description_free (font_desc);
}


static void
biji_webkit_editor_init (BijiWebkitEditor *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_WEBKIT_EDITOR, BijiWebkitEditorPrivate);
}

static void
biji_webkit_editor_finalize (GObject *object)
{
  BijiWebkitEditor *self = BIJI_WEBKIT_EDITOR (object);
  BijiWebkitEditorPrivate *priv = self->priv;

  g_free (priv->selected_text);

  if (priv->note != NULL) {
    g_object_remove_weak_pointer (G_OBJECT (priv->note), (gpointer*) &priv->note);
    g_signal_handler_disconnect (priv->note, priv->color_changed);
  }

  G_OBJECT_CLASS (biji_webkit_editor_parent_class)->finalize (object);
}

static void
biji_webkit_editor_content_changed (BijiWebkitEditor *self,
                                    const char *html,
                                    const char *text)
{
  BijiNoteObj *note = self->priv->note;
  gchar **rows;

  biji_note_obj_set_html (note, (char *)html);
  biji_note_obj_set_raw_text (note, (char *)text);

  g_signal_emit (self, biji_editor_signals[CONTENT_CHANGED], 0, NULL);

  /* Now tries to update title */

  rows = g_strsplit (text, "\n", 2);

  /* if we have a line feed, we have a proper title */
  /* this is equivalent to g_strv_length (rows) > 1 */

  if (rows && rows[0] && rows[1])
  {
    gchar *title;
    gchar *unique_title;

    title = rows[0];

    if (g_strcmp0 (title, biji_item_get_title (BIJI_ITEM (note))) != 0)
    {
      unique_title = biji_manager_get_unique_title (biji_item_get_manager (BIJI_ITEM (note)),
                                                      title);

      biji_note_obj_set_title (note, unique_title);
      g_free (unique_title);
    }
  }

  g_strfreev (rows);

  biji_note_obj_set_mtime (note, g_get_real_time () / G_USEC_PER_SEC);
  biji_note_obj_save_note (note);
}


static void
on_note_color_changed (BijiNoteObj *note, BijiWebkitEditor *self)
{
  GdkRGBA color;

  if (biji_note_obj_get_rgba(note,&color))
    set_editor_color (WEBKIT_WEB_VIEW (self), &color);
}


static void
open_url ( const char *uri)
{
  gtk_show_uri (gdk_screen_get_default (),
                uri,
                gtk_get_current_event_time (),
                NULL);
}

static gboolean
on_navigation_request (WebKitWebView           *web_view,
                       WebKitPolicyDecision    *decision,
                       WebKitPolicyDecisionType decision_type,
                       gpointer                 user_data)
{
  WebKitNavigationPolicyDecision *navigation_decision;
  WebKitNavigationAction *action;
  const char *requested_uri;

  if (decision_type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
    return FALSE;

  navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
  action = webkit_navigation_policy_decision_get_navigation_action (navigation_decision);
  requested_uri = webkit_uri_request_get_uri (webkit_navigation_action_get_request (action));
  if (g_strcmp0 (webkit_web_view_get_uri (web_view), requested_uri) == 0)
    return FALSE;

  open_url (requested_uri);
  webkit_policy_decision_ignore (decision);
  return TRUE;
}

static void
on_load_change (WebKitWebView  *web_view,
                WebKitLoadEvent event)
{
  BijiWebkitEditorPrivate *priv;
  GdkRGBA color;

  if (event != WEBKIT_LOAD_FINISHED)
    return;

  priv = BIJI_WEBKIT_EDITOR (web_view)->priv;

  /* Apply color */
  if (biji_note_obj_get_rgba (priv->note, &color))
    set_editor_color (web_view, &color);

  if (!priv->color_changed)
  {
    priv->color_changed = g_signal_connect (priv->note,
                                            "color-changed",
                                            G_CALLBACK (on_note_color_changed),
                                            web_view);
  }
}

static char *
get_js_property_string (JSGlobalContextRef js_context,
                        JSObjectRef js_object,
                        const char *property_name)
{
  JSStringRef js_property_name;
  JSValueRef js_property_value;
  JSStringRef js_string_value;
  size_t max_size;
  char *property_value = NULL;

  js_property_name = JSStringCreateWithUTF8CString (property_name);
  js_property_value = JSObjectGetProperty (js_context, js_object, js_property_name, NULL);
  JSStringRelease (js_property_name);

  if (!js_property_value || !JSValueIsString (js_context, js_property_value))
    return NULL;

  js_string_value = JSValueToStringCopy (js_context, js_property_value, NULL);
  if (!js_string_value)
    return NULL;

  max_size = JSStringGetMaximumUTF8CStringSize (js_string_value);
  if (max_size)
  {
    property_value = g_malloc (max_size);
    JSStringGetUTF8CString (js_string_value, property_value, max_size);
  }
  JSStringRelease (js_string_value);

  return property_value;
}

static gboolean
get_js_property_boolean (JSGlobalContextRef js_context,
                         JSObjectRef js_object,
                         const char *property_name)
{
  JSStringRef js_property_name;
  JSValueRef js_property_value;

  js_property_name = JSStringCreateWithUTF8CString (property_name);
  js_property_value = JSObjectGetProperty (js_context, js_object, js_property_name, NULL);
  JSStringRelease (js_property_name);

  if (!js_property_value || !JSValueIsBoolean (js_context, js_property_value))
    return FALSE;

  return JSValueToBoolean (js_context, js_property_value);
}

static void
biji_webkit_editor_handle_contents_update (BijiWebkitEditor *self,
                                           JSGlobalContextRef js_context,
                                           JSObjectRef js_object)
{
  char *html, *text;

  html = get_js_property_string (js_context, js_object, "outerHTML");
  if (!html)
    return;

  text = get_js_property_string (js_context, js_object, "innerText");
  if (!text)
  {
    g_free (html);
    return;
  }

  biji_webkit_editor_content_changed (self, html, text);
  g_free (html);
  g_free (text);
}

static void
biji_webkit_editor_handle_selection_change (BijiWebkitEditor *self,
                                            JSGlobalContextRef js_context,
                                            JSObjectRef js_object)
{
  char *block_format_str;

  self->priv->has_text = get_js_property_boolean (js_context, js_object, "hasText");

  g_free (self->priv->selected_text);
  self->priv->selected_text = get_js_property_string (js_context, js_object, "text");

  block_format_str = get_js_property_string (js_context, js_object, "blockFormat");
  if (g_strcmp0 (block_format_str, "UL") == 0)
    self->priv->block_format = BLOCK_FORMAT_UNORDERED_LIST;
  else if (g_strcmp0 (block_format_str, "OL") == 0)
    self->priv->block_format = BLOCK_FORMAT_ORDERED_LIST;
  else
    self->priv->block_format = BLOCK_FORMAT_NONE;
  g_free (block_format_str);
}

static void
on_script_message (WebKitUserContentManager *user_content,
                   WebKitJavascriptResult *message,
                   BijiWebkitEditor *self)
{
  JSGlobalContextRef js_context;
  JSValueRef js_value;
  JSObjectRef js_object;
  char *message_name;

  js_context = webkit_javascript_result_get_global_context (message);
  js_value = webkit_javascript_result_get_value (message);
  if (!js_value || !JSValueIsObject (js_context, js_value))
  {
    g_warning ("Invalid script message received");
    return;
  }

  js_object = JSValueToObject (js_context, js_value, NULL);
  if (!js_object)
    return;

  message_name = get_js_property_string (js_context, js_object, "messageName");
  if (g_strcmp0 (message_name, "ContentsUpdate") == 0)
    biji_webkit_editor_handle_contents_update (self, js_context, js_object);
  else if (g_strcmp0 (message_name, "SelectionChange") == 0)
    biji_webkit_editor_handle_selection_change (self, js_context, js_object);
  g_free (message_name);
}

static void
biji_webkit_editor_constructed (GObject *obj)
{
  BijiWebkitEditor *self;
  BijiWebkitEditorPrivate *priv;
  WebKitWebView *view;
  WebKitUserContentManager *user_content;
  GBytes *html_data;
  gchar *body;

  self = BIJI_WEBKIT_EDITOR (obj);
  view = WEBKIT_WEB_VIEW (self);
  priv = self->priv;

  G_OBJECT_CLASS (biji_webkit_editor_parent_class)->constructed (obj);

  user_content = webkit_web_view_get_user_content_manager (view);
  webkit_user_content_manager_register_script_message_handler (user_content, "bijiben");
  g_signal_connect (user_content, "script-message-received::bijiben",
                    G_CALLBACK (on_script_message), self);

  priv->sel = e_editor_selection_new (view);

  webkit_web_view_set_editable (view, TRUE);

  /* Do not segfault at finalize
   * if the note died */
  g_object_add_weak_pointer (G_OBJECT (priv->note), (gpointer*) &priv->note);

  body = biji_note_obj_get_html (priv->note);

  if (!body)
    body = html_from_plain_text ("");

  html_data = g_bytes_new_take (body, strlen (body));
  webkit_web_view_load_bytes (view, html_data, "application/xhtml+xml", NULL,
                              "file://" DATADIR G_DIR_SEPARATOR_S "bijiben" G_DIR_SEPARATOR_S);
  g_bytes_unref (html_data);

  /* Do not be a browser */
  g_signal_connect (view, "decide-policy",
                    G_CALLBACK (on_navigation_request), NULL);
  g_signal_connect (view, "load-changed",
                    G_CALLBACK (on_load_change), NULL);
}

static void
biji_webkit_editor_get_property (GObject  *object,
                                 guint     property_id,
                                 GValue   *value,
                                 GParamSpec *pspec)
{
  BijiWebkitEditor *self = BIJI_WEBKIT_EDITOR (object);

  switch (property_id)
  {
    case PROP_NOTE:
      g_value_set_object (value, self->priv->note);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
biji_webkit_editor_set_property (GObject  *object,
                                 guint     property_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
  BijiWebkitEditor *self = BIJI_WEBKIT_EDITOR (object);

  switch (property_id)
  {
    case PROP_NOTE:
      self->priv->note = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
biji_webkit_editor_class_init (BijiWebkitEditorClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = biji_webkit_editor_constructed;
  object_class->finalize = biji_webkit_editor_finalize;
  object_class->get_property = biji_webkit_editor_get_property;
  object_class->set_property = biji_webkit_editor_set_property;

  properties[PROP_NOTE] = g_param_spec_object ("note",
                                               "Note",
                                               "Biji Note Obj",
                                                BIJI_TYPE_NOTE_OBJ,
                                                G_PARAM_READWRITE  |
                                                G_PARAM_CONSTRUCT |
                                                G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class,PROP_NOTE,properties[PROP_NOTE]);

  biji_editor_signals[EDITOR_CLOSED] = g_signal_new ("closed",
                                       G_OBJECT_CLASS_TYPE (klass),
                                       G_SIGNAL_RUN_FIRST,
                                       0,
                                       NULL,
                                       NULL,
                                       g_cclosure_marshal_VOID__VOID,
                                       G_TYPE_NONE,
                                       0);
  biji_editor_signals[CONTENT_CHANGED] = g_signal_new ("content-changed",
                                         G_OBJECT_CLASS_TYPE (klass),
                                         G_SIGNAL_RUN_LAST,
                                         0,
                                         NULL,
                                         NULL,
                                         g_cclosure_marshal_VOID__VOID,
                                         G_TYPE_NONE,
                                         0);

  g_type_class_add_private (klass, sizeof (BijiWebkitEditorPrivate));
}

BijiWebkitEditor *
biji_webkit_editor_new (BijiNoteObj *note)
{
  WebKitUserContentManager *manager = webkit_user_content_manager_new ();

  return g_object_new (BIJI_TYPE_WEBKIT_EDITOR,
                       "web-context", biji_webkit_editor_get_web_context (),
                       "settings", biji_webkit_editor_get_web_settings (),
                       "user-content-manager", manager,
                       "note", note,
                       NULL);

  g_object_unref (manager);
}
