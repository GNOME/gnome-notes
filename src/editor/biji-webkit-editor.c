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

#include "bjb-utils.h"
#include "biji-string.h"
#include "biji-webkit-editor.h"
#include "biji-editor-selection.h"
#include <jsc/jsc.h>

#define ZOOM_LARGE  1.5f;
#define ZOOM_MEDIUM 1.0f;
#define ZOOM_SMALL  0.8f;

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
  BLOCK_FORMAT_ORDERED_LIST,
  BLOCK_FORMAT_INDENT,
  BLOCK_FORMAT_OUTDENT
} BlockFormat;

static guint biji_editor_signals [EDITOR_SIGNALS] = { 0 };

static GParamSpec *properties[NUM_PROP] = { NULL, };

struct _BijiWebkitEditor
{
  WebKitWebView     parent_instance;
  BjbNote          *note;
  gulong            content_changed;
  gboolean          has_text;
  char             *selected_text;
  BlockFormat       block_format;
  gboolean          first_load;
  EEditorSelection *sel;
  GtkEventController *shortcut_controller;
};

G_DEFINE_TYPE (BijiWebkitEditor, biji_webkit_editor, WEBKIT_TYPE_WEB_VIEW)

const gchar *
biji_webkit_editor_get_selection (BijiWebkitEditor *self)
{
  return self->selected_text;
}

static WebKitWebContext *
biji_webkit_editor_get_web_context (void)
{
  static WebKitWebContext *web_context = NULL;

  if (!web_context)
  {
    web_context = webkit_web_context_get_default ();
    webkit_web_context_set_cache_model (web_context, WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER);
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
  /* insert commands toggle the formatting */
  switch (block_format)
  {
    case BLOCK_FORMAT_NONE:
      break;
    case BLOCK_FORMAT_UNORDERED_LIST:
      webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), "insertUnorderedList");
      break;
    case BLOCK_FORMAT_ORDERED_LIST:
      webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), "insertOrderedList");
      break;
    case BLOCK_FORMAT_INDENT:
      webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), "indent");
      break;
    case BLOCK_FORMAT_OUTDENT:
      webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), "outdent");
      break;
    default:
      g_assert_not_reached ();
  }
}

void
biji_webkit_editor_apply_format (BijiWebkitEditor *self, gint format)
{
  gboolean has_list = self->block_format == BLOCK_FORMAT_UNORDERED_LIST
                      || self-> block_format == BLOCK_FORMAT_ORDERED_LIST;

  switch (format)
  {
    case BIJI_BOLD:
      biji_toggle_format (self->sel, e_editor_selection_get_bold,
                                      e_editor_selection_set_bold);
      break;

    case BIJI_ITALIC:
      biji_toggle_format (self->sel, e_editor_selection_get_italic,
                                      e_editor_selection_set_italic);
      break;

    case BIJI_STRIKE:
      biji_toggle_format (self->sel, e_editor_selection_get_strike_through,
                                      e_editor_selection_set_strike_through);
      break;

    case BIJI_BULLET_LIST:
      biji_toggle_block_format (self, BLOCK_FORMAT_UNORDERED_LIST);
      break;

    case BIJI_ORDER_LIST:
      biji_toggle_block_format (self, BLOCK_FORMAT_ORDERED_LIST);
      break;

    case BIJI_INDENT:
      if (has_list)
        biji_toggle_block_format (self, BLOCK_FORMAT_INDENT);
      break;

    case BIJI_OUTDENT:
      if (has_list)
        biji_toggle_block_format (self, BLOCK_FORMAT_OUTDENT);
      break;

    default:
      g_warning ("biji_webkit_editor_apply_format : Invalid format");
  }
}

static gboolean
biji_webkit_editor_undo_cb (BijiWebkitEditor *self)
{
  webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), WEBKIT_EDITING_COMMAND_UNDO);

  return GDK_EVENT_STOP;
}

static gboolean
biji_webkit_editor_redo_cb (BijiWebkitEditor *self)
{
  webkit_web_view_execute_editing_command (WEBKIT_WEB_VIEW (self), WEBKIT_EDITING_COMMAND_REDO);

  return GDK_EVENT_STOP;
}

static void
set_editor_color (WebKitWebView *w, GdkRGBA *col)
{
  g_autofree gchar *script = NULL;

  webkit_web_view_set_background_color (w, col);
  script = g_strdup_printf ("document.getElementById('editable').style.color = '%s';",
                            BJB_UTILS_COLOR_INTENSITY (col) < 0.5 ? "white" : "black");
  webkit_web_view_evaluate_javascript (w, script, -1, NULL, NULL, NULL, NULL, NULL);
}

void
biji_webkit_editor_set_font (BijiWebkitEditor *self, gchar *font)
{
  PangoFontDescription *font_desc;
  const gchar *family;

  /* parse : but we only parse font properties we'll be able
   * to transfer to webkit editor
   * Maybe is there a better way than webkitSettings,
   * eg applying format to the whole body */
  font_desc = pango_font_description_from_string (font);
  family = pango_font_description_get_family (font_desc);

  /* Set */
  g_object_set (biji_webkit_editor_get_web_settings (),
                "default-font-family", family,
                NULL);

  pango_font_description_free (font_desc);
}

void
biji_webkit_editor_set_text_size (BijiWebkitEditor *self,
                                  BjbTextSizeType   text_size)
{
  double zoom_level = ZOOM_MEDIUM;

  if (text_size == BJB_TEXT_SIZE_LARGE)
    {
      zoom_level = ZOOM_LARGE;
    }
  else if (text_size == BJB_TEXT_SIZE_SMALL)
    {
      zoom_level = ZOOM_SMALL;
    }

  webkit_web_view_set_zoom_level (WEBKIT_WEB_VIEW (self), zoom_level);
}

static void
biji_webkit_editor_init (BijiWebkitEditor *self)
{
}

static void
biji_webkit_editor_finalize (GObject *object)
{
  BijiWebkitEditor *self = BIJI_WEBKIT_EDITOR (object);

  g_clear_object (&self->note);
  g_free (self->selected_text);

  self->shortcut_controller = NULL;

  if (self->note != NULL) {
    g_object_remove_weak_pointer (G_OBJECT (self->note), (gpointer*) &self->note);
  }

  G_OBJECT_CLASS (biji_webkit_editor_parent_class)->finalize (object);
}

static void
biji_webkit_editor_content_changed (BijiWebkitEditor *self,
                                    const char *html,
                                    const char *text)
{
  BjbNote *note = self->note;

  bjb_note_set_html (note, (char *)html);
  bjb_note_set_raw_content (note, (char *)text);

  g_signal_emit (self, biji_editor_signals[CONTENT_CHANGED], 0, NULL);

  bjb_item_set_mtime (BJB_ITEM (note), g_get_real_time () / G_USEC_PER_SEC);
}


static void
on_note_color_changed (BijiWebkitEditor *self)
{
  GdkRGBA color;

  g_assert (BIJI_IS_WEBKIT_EDITOR (self));

  if (bjb_item_get_rgba (BJB_ITEM (self->note), &color))
    set_editor_color (WEBKIT_WEB_VIEW (self), &color);
}

static gboolean
on_navigation_request (WebKitWebView           *web_view,
                       WebKitPolicyDecision    *decision,
                       WebKitPolicyDecisionType decision_type,
                       gpointer                 user_data)
{
  WebKitNavigationPolicyDecision *navigation_decision;
  g_autoptr(GtkUriLauncher) launcher = NULL;
  WebKitNavigationAction *action;
  const char *requested_uri;
  GtkWidget *toplevel;

  if (decision_type != WEBKIT_POLICY_DECISION_TYPE_NAVIGATION_ACTION)
    return FALSE;

  navigation_decision = WEBKIT_NAVIGATION_POLICY_DECISION (decision);
  action = webkit_navigation_policy_decision_get_navigation_action (navigation_decision);
  requested_uri = webkit_uri_request_get_uri (webkit_navigation_action_get_request (action));
  if (g_strcmp0 (webkit_web_view_get_uri (web_view), requested_uri) == 0)
    return FALSE;

  toplevel = gtk_widget_get_ancestor (GTK_WIDGET (web_view), GTK_TYPE_WINDOW);
  g_return_val_if_fail (GTK_IS_WINDOW (toplevel), FALSE);

  launcher = gtk_uri_launcher_new (requested_uri);
  gtk_uri_launcher_launch (launcher, GTK_WINDOW (toplevel), NULL, NULL, NULL);

  webkit_policy_decision_ignore (decision);
  return TRUE;
}

static void
on_load_change (WebKitWebView  *web_view,
                WebKitLoadEvent event)
{
  BijiWebkitEditor *self = BIJI_WEBKIT_EDITOR (web_view);
  GdkRGBA color;

  if (event != WEBKIT_LOAD_FINISHED)
    return;

  /* Apply color */
  if (bjb_item_get_rgba (BJB_ITEM (self->note), &color))
    set_editor_color (web_view, &color);

  g_signal_connect_object (self->note,
                           "notify::color",
                           G_CALLBACK (on_note_color_changed),
                           web_view,
                           G_CONNECT_SWAPPED);
}

static gboolean
on_context_menu (WebKitWebView       *web_view,
                 WebKitContextMenu   *context_menu,
                 GdkEvent            *event,
                 WebKitHitTestResult *hit_test_result,
                 gpointer             user_data)
{
  return TRUE;
}

static void
biji_webkit_editor_handle_contents_update (BijiWebkitEditor *self,
                                           JSCValue         *js_value)
{
  g_autoptr (JSCValue) js_outer_html = NULL;
  g_autoptr (JSCValue) js_inner_text = NULL;
  g_autofree gchar *html = NULL;
  g_autofree gchar *text = NULL;

  js_outer_html = jsc_value_object_get_property (js_value, "outerHTML");
  html = jsc_value_to_string (js_outer_html);
  if (!html)
    return;

  js_inner_text = jsc_value_object_get_property (js_value, "innerText");
  text = jsc_value_to_string (js_inner_text);
  if (!text)
    return;

  biji_webkit_editor_content_changed (self, html, text);
}

static void
biji_webkit_editor_handle_selection_change (BijiWebkitEditor *self,
                                            JSCValue         *js_value)
{
  g_autoptr (JSCValue) js_has_text     = NULL;
  g_autoptr (JSCValue) js_text         = NULL;
  g_autoptr (JSCValue) js_block_format = NULL;
  g_autofree char *block_format_str = NULL;

  js_has_text = jsc_value_object_get_property (js_value, "hasText");
  self->has_text = jsc_value_to_boolean (js_has_text);

  js_text = jsc_value_object_get_property (js_value, "text");
  g_free (self->selected_text);
  self->selected_text = jsc_value_to_string (js_text);

  js_block_format = jsc_value_object_get_property (js_value, "blockFormat");
  block_format_str = jsc_value_to_string (js_block_format);
  if (g_strcmp0 (block_format_str, "UL") == 0)
    self->block_format = BLOCK_FORMAT_UNORDERED_LIST;
  else if (g_strcmp0 (block_format_str, "OL") == 0)
    self->block_format = BLOCK_FORMAT_ORDERED_LIST;
  else
    self->block_format = BLOCK_FORMAT_NONE;
}

static void
on_script_message (WebKitUserContentManager *user_content,
                   JSCValue *js_value,
                   BijiWebkitEditor *self)
{
  g_autoptr (JSCValue)  js_message_name = NULL;
  g_autofree char *message_name = NULL;

  g_assert (jsc_value_is_object (js_value));

  js_message_name = jsc_value_object_get_property (js_value, "messageName");
  message_name = jsc_value_to_string (js_message_name);
  if (g_strcmp0 (message_name, "ContentsUpdate") == 0)
    {
      if (self->first_load)
        self->first_load = FALSE;
      else
        biji_webkit_editor_handle_contents_update (self, js_value);
    }
  else if (g_strcmp0 (message_name, "SelectionChange") == 0)
    biji_webkit_editor_handle_selection_change (self, js_value);
}

static char *
html_from_plain_content (const char *content)
{
  g_autofree char *escaped = NULL;

  if (content == NULL)
    escaped = g_strdup ("");
  else
    escaped = biji_str_mass_replace (content,
                                     "&", "&amp;",
                                     "<", "&lt;",
                                     ">", "&gt;",
                                     "\n", "<br/>",
                                     NULL);

  return g_strconcat ("<html xmlns=\"http://www.w3.org/1999/xhtml\">",
                      "<head>",
                      "<link rel=\"stylesheet\" href=\"Default.css\" type=\"text/css\"/>",
                      "<script language=\"javascript\" src=\"bijiben.js\"></script>"
                      "</head>",
                      "<body id=\"editable\">",
                      escaped,
                      "</body></html>",
                      NULL);
}

static void
biji_webkit_editor_constructed (GObject *obj)
{
  BijiWebkitEditor *self;
  GtkShortcut *redo_shortcut;
  GtkShortcut *undo_shortcut;
  WebKitWebView *view;
  WebKitUserContentManager *user_content;
  g_autoptr(GBytes) html_data = NULL;
  gchar *body;

  self = BIJI_WEBKIT_EDITOR (obj);
  view = WEBKIT_WEB_VIEW (self);
  self->first_load = TRUE;

  G_OBJECT_CLASS (biji_webkit_editor_parent_class)->constructed (obj);

  user_content = webkit_web_view_get_user_content_manager (view);
  webkit_user_content_manager_register_script_message_handler (user_content, "bijiben", NULL);
  g_signal_connect (user_content, "script-message-received::bijiben",
                    G_CALLBACK (on_script_message), self);

  self->sel = e_editor_selection_new (view);

  webkit_web_view_set_editable (view, !bjb_item_is_trashed (BJB_ITEM (self->note)));

  /* Do not segfault at finalize
   * if the note died */
  g_object_add_weak_pointer (G_OBJECT (self->note), (gpointer*) &self->note);

  body = bjb_note_get_html (self->note);

  if (!body)
    body = html_from_plain_content ("");

  html_data = g_bytes_new_take (body, strlen (body));
  webkit_web_view_load_bytes (view, html_data, "application/xhtml+xml", NULL,
                              "file://" DATADIR G_DIR_SEPARATOR_S "bijiben" G_DIR_SEPARATOR_S);

  redo_shortcut = gtk_shortcut_new (gtk_keyval_trigger_new (GDK_KEY_Z, GDK_CONTROL_MASK | GDK_SHIFT_MASK),
                                    gtk_callback_action_new ((GtkShortcutFunc) biji_webkit_editor_redo_cb, self, NULL));
  undo_shortcut = gtk_shortcut_new (gtk_keyval_trigger_new (GDK_KEY_Z, GDK_CONTROL_MASK),
                                    gtk_callback_action_new ((GtkShortcutFunc) biji_webkit_editor_undo_cb, self, NULL));

  self->shortcut_controller = gtk_shortcut_controller_new ();
  gtk_shortcut_controller_set_scope (GTK_SHORTCUT_CONTROLLER (self->shortcut_controller), GTK_SHORTCUT_SCOPE_LOCAL);
  gtk_shortcut_controller_add_shortcut (GTK_SHORTCUT_CONTROLLER (self->shortcut_controller),
                                        redo_shortcut);
  gtk_shortcut_controller_add_shortcut (GTK_SHORTCUT_CONTROLLER (self->shortcut_controller),
                                        undo_shortcut);

  gtk_widget_add_controller (GTK_WIDGET (self), self->shortcut_controller);

  /* Do not be a browser */
  g_signal_connect (view, "decide-policy",
                    G_CALLBACK (on_navigation_request), NULL);
  g_signal_connect (view, "load-changed",
                    G_CALLBACK (on_load_change), NULL);
  g_signal_connect (view, "context-menu",
                    G_CALLBACK (on_context_menu), NULL);
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
      g_value_set_object (value, self->note);
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
      self->note = g_value_dup_object (value);
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
                                               "Bjb Note",
                                                BJB_TYPE_NOTE,
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
}

BijiWebkitEditor *
biji_webkit_editor_new (BjbNote *note)
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
