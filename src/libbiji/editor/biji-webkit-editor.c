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

/* Prop */
enum {
  PROP_0,
  PROP_NOTE,
  NUM_PROP
};

/* Signals */
enum {
  EDITOR_CLOSED,
  EDITOR_SIGNALS
};

static guint biji_editor_signals [EDITOR_SIGNALS] = { 0 };

static GParamSpec *properties[NUM_PROP] = { NULL, };

struct _BijiWebkitEditorPrivate
{
  BijiNoteObj *note;
  gulong content_changed;
  gulong color_changed;
  gchar *font_color;

  WebKitWebSettings *settings;
  EEditorSelection *sel;
  GObject *spell_check;
};

G_DEFINE_TYPE (BijiWebkitEditor, biji_webkit_editor, WEBKIT_TYPE_WEB_VIEW);

gboolean
biji_webkit_editor_has_selection (BijiWebkitEditor *self)
{
  BijiWebkitEditorPrivate *priv = self->priv;
  const gchar *text = NULL;
  gboolean retval = FALSE;

  if (e_editor_selection_has_text (priv->sel))
  {
    text = e_editor_selection_get_string (priv->sel);

    if ( g_strcmp0 (text, "") != 0)
      retval = TRUE;
  }

  return retval;
}

gchar *
biji_webkit_editor_get_selection (BijiWebkitEditor *self)
{
  gchar *retval = NULL;

  if (e_editor_selection_has_text (self->priv->sel))
    retval = (gchar*) e_editor_selection_get_string (self->priv->sel);

  return retval;
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
biji_toggle_block_format (EEditorSelection *sel,
                          EEditorSelectionBlockFormat format)
{
  if (e_editor_selection_get_block_format(sel) == format)
    e_editor_selection_set_block_format (sel,
                                  E_EDITOR_SELECTION_BLOCK_FORMAT_NONE);

  else
    e_editor_selection_set_block_format (sel, format);
}

void
biji_webkit_editor_apply_format (BijiWebkitEditor *self, gint format)
{
  BijiWebkitEditorPrivate *priv = self->priv;

  if ( e_editor_selection_has_text (priv->sel))
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
        biji_toggle_block_format (priv->sel,
                        E_EDITOR_SELECTION_BLOCK_FORMAT_UNORDERED_LIST);
        break;

      case BIJI_ORDER_LIST:
        biji_toggle_block_format (priv->sel,
                        E_EDITOR_SELECTION_BLOCK_FORMAT_ORDERED_LIST);
        break;

      default:
        g_warning ("biji_webkit_editor_apply_format : Invalid format");
    }
  }
}

void
biji_webkit_editor_cut (BijiWebkitEditor *self)
{
  webkit_web_view_cut_clipboard (WEBKIT_WEB_VIEW (self));
}

void
biji_webkit_editor_copy (BijiWebkitEditor *self)
{
  webkit_web_view_copy_clipboard (WEBKIT_WEB_VIEW (self));
}

void
biji_webkit_editor_paste (BijiWebkitEditor *self)
{
  webkit_web_view_paste_clipboard (WEBKIT_WEB_VIEW (self));
}

static void
set_editor_color (GtkWidget *w, GdkRGBA *col)
{
  gtk_widget_override_background_color (w, GTK_STATE_FLAG_NORMAL, col);
}

void
biji_webkit_editor_set_font (BijiWebkitEditor *self, gchar *font)
{
  BijiWebkitEditorPrivate *priv = self->priv;
  PangoFontDescription *font_desc;

  /* parse : but we only parse font properties we'll be able
   * to transfer to webkit editor
   * Maybe is there a better way than webkitSettings,
   * eg applying format to the whole body */
  font_desc = pango_font_description_from_string (font);
  const gchar * family = pango_font_description_get_family (font_desc);
  gint size = pango_font_description_get_size (font_desc) / 1000 ;

  /* Set */
  g_object_set (G_OBJECT(priv->settings),
                "default-font-family", family,
                "default-font-size", size,
                NULL);

  pango_font_description_free (font_desc);
}


static void
biji_webkit_editor_init (BijiWebkitEditor *self)
{
  WebKitWebView *view = WEBKIT_WEB_VIEW (self);
  BijiWebkitEditorPrivate *priv;

  priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_WEBKIT_EDITOR, BijiWebkitEditorPrivate);
  self->priv = priv;

  priv->sel = e_editor_selection_new (view);
  priv->font_color = NULL;

  /* Settings */
  webkit_web_view_set_editable (view, TRUE);
  webkit_web_view_set_transparent (view, TRUE);
  priv->settings = webkit_web_view_get_settings (view);


  g_object_set (G_OBJECT(priv->settings),
                "enable-plugins", FALSE,
                "enable-file-access-from-file-uris", TRUE,
                "enable-spell-checking", TRUE,
                "tab-key-cycles-through-elements", FALSE,
                NULL);

  priv->spell_check = webkit_get_text_checker ();
}

static void
biji_webkit_editor_finalize (GObject *object)
{
  BijiWebkitEditor *self = BIJI_WEBKIT_EDITOR (object);
  BijiWebkitEditorPrivate *priv = self->priv;

  /* priv->spell_check is ref by webkit. probably not to unref */
  g_object_unref (priv->sel);

  g_free (priv->font_color);

  if (priv->note != NULL)
    g_signal_handler_disconnect (priv->note, priv->color_changed);


  G_OBJECT_CLASS (biji_webkit_editor_parent_class)->finalize (object);
}

static void
on_content_changed (WebKitWebView *view)
{
  BijiWebkitEditor     *self = BIJI_WEBKIT_EDITOR (view);
  BijiNoteObj *note = self->priv->note;
  WebKitDOMDocument    *dom;
  WebKitDOMHTMLElement *elem;
  gchar                *html, *text;
  gchar                **rows;

  /* First html serializing */
  dom = webkit_web_view_get_dom_document (view);
  elem = WEBKIT_DOM_HTML_ELEMENT (webkit_dom_document_get_document_element (dom));
  html = webkit_dom_html_element_get_outer_html (elem);
  text = webkit_dom_html_element_get_inner_text (elem);

  biji_note_obj_set_html (note, html);
  biji_note_obj_set_raw_text (note, text);

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
  g_free (html);
  g_free (text);

  biji_note_obj_set_mtime (note, g_get_real_time () / G_USEC_PER_SEC);
  biji_note_obj_save_note (note);
}


static void
on_css_directory_created (GObject *source,
                          GAsyncResult *res,
                          gpointer user_data)
{
  BijiWebkitEditorPrivate *priv;
  gchar *path_src = NULL, *path_dest = NULL, *css_path = NULL, *font_color = NULL, *css = NULL;
  GFile *src = NULL, *dest = NULL;
  GdkRGBA color;
  GError *error = NULL;
  GFileInputStream *str;
  gchar buffer[1000] = "";
  gsize bytes;
  GRegex *rgxp = NULL;

  priv = BIJI_WEBKIT_EDITOR (user_data)->priv;
  biji_note_obj_get_rgba (priv->note,&color);

  /*
   * Generate font color
   * See if need need to change css.
   */
  if (color.red < 0.5)
    font_color = "white";
  else
    font_color = "black";

  if (g_strcmp0 (priv->font_color, font_color) == 0)
    return;
  else
    priv->font_color = g_strdup (font_color);


  /* build the path for source and Destination of Default.css */
  path_src = g_build_filename(DATADIR, "bijiben", "Default.css", NULL);
  src = g_file_new_for_path (path_src);

  path_dest = g_build_filename(g_get_tmp_dir (), "bijiben", "Default.css", NULL);
  dest = g_file_new_for_path (path_dest);
  css_path = g_file_get_uri (dest);

  /* Read & amend the css */
  str = g_file_read (src, NULL, &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    goto out;
  }

  g_input_stream_read_all (G_INPUT_STREAM (str), buffer, 1000, &bytes, NULL, &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    goto out;
  }

  g_input_stream_close (G_INPUT_STREAM (str), NULL, NULL);
  rgxp = g_regex_new ("_BIJI_TEXT_COLOR", 0, 0, NULL);
  css = g_regex_replace_literal (rgxp, buffer, -1, 0, priv->font_color, 0, &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    goto out;
  }

  /* copy the Default.css file */
  g_file_replace_contents (dest, css, 1000, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL, &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    goto out;
  }

  /* Change the css path to NULL and then
     again set it back to actual css path
     so that changes in css file are considered
     immediately */

  g_object_set (G_OBJECT(priv->settings),
               "user-stylesheet-uri", NULL,
               NULL);

  g_object_set (G_OBJECT(priv->settings),
                "user-stylesheet-uri", css_path,
                NULL);

out:
  g_object_unref (str);
  g_free (css_path);
  g_free (path_src);
  g_free (path_dest);
  g_object_unref (src);
  g_object_unref (dest);
  g_free (css);
  g_regex_unref (rgxp);
}

static void
biji_webkit_editor_change_css_file(BijiWebkitEditor *self)
{
  gchar *bijiben_temp_dir_path;
  GFile *bijiben_temp_dir;

  /* Generate Temp directory path and URI for bijiben */
  bijiben_temp_dir_path = g_build_filename (g_get_tmp_dir (), "bijiben", NULL);
  bijiben_temp_dir = g_file_new_for_path (bijiben_temp_dir_path);
  g_file_make_directory (bijiben_temp_dir, NULL, NULL);

  g_file_make_directory_async (bijiben_temp_dir,
                               G_PRIORITY_DEFAULT,
                               NULL, /* cancellable */
                               on_css_directory_created,
                               self);
}


static void
on_note_color_changed (BijiNoteObj *note, BijiWebkitEditor *self)
{
  GdkRGBA color;

  if (biji_note_obj_get_rgba(note,&color))
    set_editor_color (GTK_WIDGET (self), &color);

  /*Need to change text color as well*/
  biji_webkit_editor_change_css_file(self);
}


static void
open_url ( const char *uri)
{
  gtk_show_uri (gdk_screen_get_default (),
                uri,
                gtk_get_current_event_time (),
                NULL);
}


gboolean
on_navigation_request                  (WebKitWebView             *web_view,
                                        WebKitWebFrame            *frame,
                                        WebKitNetworkRequest      *request,
                                        WebKitWebNavigationAction *navigation_action,
                                        WebKitWebPolicyDecision   *policy_decision,
                                        gpointer                   user_data)
{
  webkit_web_policy_decision_ignore (policy_decision);
  open_url (webkit_network_request_get_uri (request));
  return TRUE;
}

static void
biji_webkit_editor_constructed (GObject *obj)
{
  BijiWebkitEditor *self;
  BijiWebkitEditorPrivate *priv;
  WebKitWebView *view;
  gchar *body;
  GdkRGBA color;

  self = BIJI_WEBKIT_EDITOR (obj);
  view = WEBKIT_WEB_VIEW (self);
  priv = self->priv;

  G_OBJECT_CLASS (biji_webkit_editor_parent_class)->constructed (obj);

  /* Do not segfault at finalize
   * if the note died */
  g_object_add_weak_pointer (G_OBJECT (priv->note), (gpointer*) &priv->note);


  body = biji_note_obj_get_html (priv->note);

  if (!body)
    body = html_from_plain_text ("");

  webkit_web_view_load_string (view, body, "application/xhtml+xml", NULL, NULL);
  g_free (body);


  /* Do not be a browser */
  g_signal_connect (view, "navigation-policy-decision-requested",
                    G_CALLBACK (on_navigation_request), NULL);


  /* Drag n drop */
  GtkTargetList *targets = webkit_web_view_get_copy_target_list (view);
  gtk_target_list_add_image_targets (targets, 0, TRUE);

  /* Apply color */
  if (biji_note_obj_get_rgba (priv->note,&color))
    set_editor_color (GTK_WIDGET (self), &color);

  priv->color_changed = g_signal_connect (priv->note,
                                     "color-changed",
                                     G_CALLBACK (on_note_color_changed),
                                     self);

  /* Save */
  priv->content_changed = g_signal_connect (WEBKIT_WEB_VIEW (self),
                                     "user-changed-contents",
                                     G_CALLBACK (on_content_changed),
                                     NULL);

  /*Add font color*/
  biji_webkit_editor_change_css_file(self);
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

  g_type_class_add_private (klass, sizeof (BijiWebkitEditorPrivate));
}

BijiWebkitEditor *
biji_webkit_editor_new (BijiNoteObj *note)
{
  return g_object_new (BIJI_TYPE_WEBKIT_EDITOR,
                       "note", note,
                       NULL);
}
