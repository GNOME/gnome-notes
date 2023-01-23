/* biji-note-obj.c
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

#include "biji-date-time.h"
#include "biji-manager.h"
#include "../bjb-utils.h"
#include "libbiji.h"
#include "biji-note-obj.h"
#include "biji-timeout.h"

#include "editor/biji-webkit-editor.h"

typedef struct
{
  /* Metadata */
  char                  *path;

  BijiManager           *manager;

  /* Save */
  BijiTimeout           *timeout;
  gboolean               needs_save;

  /* Tags
   * In Tomboy, templates are 'system:notebook:%s' tags.*/
  GHashTable            *labels;
  gboolean               is_template;

  /* Signals */
  gulong                 save;
} BijiNoteObjPrivate;

/* Properties */
enum {
  PROP_0,
  PROP_PATH,
  PROP_MANAGER,
  BIJI_OBJ_PROPERTIES
};

static GParamSpec *properties[BIJI_OBJ_PROPERTIES] = { NULL, };

/* Signals. Do not interfere with biji-item parent class. */
enum {
  NOTE_DELETED,
  NOTE_TRASHED,
  NOTE_CHANGED,
  NOTE_COLOR_CHANGED,
  BIJI_OBJ_SIGNALS
};

static guint biji_obj_signals [BIJI_OBJ_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (BijiNoteObj, biji_note_obj, BJB_TYPE_NOTE)

static void
on_save_timeout (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  /* g_mutex_lock (priv->mutex); */

  if (!priv->needs_save)
    return;

  g_object_ref (self);

  /* Each note type serializes its own way
   * FIXME: common tracker would make sense */

  BIJI_NOTE_OBJ_GET_CLASS (self)->save_note (self);

  priv->needs_save = FALSE;
  g_object_unref (self);
}

static void
on_note_obj_add_notebook_cb (GObject      *object,
                             GAsyncResult *result,
                             gpointer      user_data)
{
  g_autoptr(BjbNotebook) notebook = user_data;

  g_assert (BJB_IS_NOTEBOOK (notebook));
}

static void
biji_note_obj_init (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  priv->timeout = biji_timeout_new ();
  priv->save = g_signal_connect_swapped (priv->timeout, "timeout", G_CALLBACK (on_save_timeout), self);
  g_object_set_data_full (G_OBJECT (priv->timeout), "note", g_object_ref (self), g_object_unref);
  priv->labels = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
biji_note_obj_finalize (GObject *object)
{
  BijiNoteObj        *self = BIJI_NOTE_OBJ(object);
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  g_clear_signal_handler (&priv->save, priv->timeout);

  if (priv->timeout)
    g_object_unref (priv->timeout);

  if (priv->needs_save)
    on_save_timeout (self);

  g_free (priv->path);

  g_hash_table_destroy (priv->labels);

  G_OBJECT_CLASS (biji_note_obj_parent_class)->finalize (object);
}

static void
biji_note_obj_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BijiNoteObj        *self = BIJI_NOTE_OBJ (object);
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  switch (property_id)
    {
    case PROP_PATH:
      biji_note_obj_set_path (self, g_value_get_string (value));
      bjb_item_set_uid (BJB_ITEM (self), g_value_get_string (value));
      break;
    case PROP_MANAGER:
      priv->manager = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_note_obj_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BijiNoteObj        *self = BIJI_NOTE_OBJ (object);
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  switch (property_id)
    {
    case PROP_PATH:
      g_value_set_string (value, bjb_item_get_uid (BJB_ITEM (self)));
      g_value_set_object (value, priv->path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

const char *
biji_note_obj_get_path (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  return priv->path;
}

void
biji_note_obj_set_path (BijiNoteObj *self,
                        const char  *path)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (g_strcmp0 (path, priv->path) == 0)
    return;

  g_free (priv->path);
  priv->path = g_strdup (path);
  bjb_item_set_uid (BJB_ITEM (self), path);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PATH]);
}

gpointer
biji_note_obj_get_manager (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  return priv->manager;
}

gboolean
biji_note_obj_trash (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);
  gboolean retval;

  priv->needs_save = FALSE;
  biji_timeout_cancel (priv->timeout);

  retval = BIJI_NOTE_OBJ_GET_CLASS (self)->archive (self);

  if (retval)
    g_signal_emit_by_name (self, "trashed", NULL);

  return retval;
}

gboolean
biji_note_obj_delete (BijiNoteObj *self)
{
  gboolean retval;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (self), FALSE);

  retval = BIJI_NOTE_OBJ_GET_CLASS (self)->delete (self);
  if (retval)
    g_signal_emit_by_name (self, "deleted", NULL);

  return retval;
}

gboolean
biji_note_obj_has_notebook (BijiNoteObj *self,
                            const char  *label)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (g_hash_table_lookup (priv->labels, label))
    return TRUE;

  return FALSE;
}

gboolean
biji_note_obj_add_notebook (BijiNoteObj *self,
                            gpointer     notebook,
                            const char  *title)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);
  const char *label = title;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (self), FALSE);

  if (BJB_IS_NOTEBOOK (notebook))
    label = bjb_item_get_title (notebook);

  if (biji_note_obj_has_notebook (self, label))
    return FALSE;

  g_hash_table_add (priv->labels, g_strdup (label));

  if (BJB_IS_NOTEBOOK (notebook))
    {
      biji_tracker_add_note_to_notebook_async (biji_manager_get_tracker (priv->manager),
                                               self, label, on_note_obj_add_notebook_cb,
                                               g_object_ref (notebook));

      bjb_item_set_meta_mtime (BJB_ITEM (self), g_get_real_time () / G_USEC_PER_SEC);
      biji_note_obj_save_note (self);
    }

  return TRUE;
}

gboolean
biji_note_obj_remove_notebook (BijiNoteObj *self,
                               gpointer     notebook)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (self), FALSE);
  g_return_val_if_fail (BJB_IS_NOTEBOOK (notebook), FALSE);

  if (g_hash_table_remove (priv->labels, bjb_item_get_title (notebook)))
    {
      biji_tracker_remove_note_notebook_async (biji_manager_get_tracker (priv->manager),
                                               self, notebook, on_note_obj_add_notebook_cb,
                                               g_object_ref (notebook));

      bjb_item_set_meta_mtime (BJB_ITEM (self), g_get_real_time () / G_USEC_PER_SEC);
      biji_note_obj_save_note (self);
      return TRUE;
    }

  return FALSE;
}

void
biji_note_obj_set_rgba (BijiNoteObj   *self,
                        const GdkRGBA *rgba)
{
  bjb_item_set_rgba (BJB_ITEM (self), rgba);
  g_signal_emit (G_OBJECT (self), biji_obj_signals[NOTE_COLOR_CHANGED], 0);
}

gboolean
biji_note_obj_get_rgba(BijiNoteObj *self,
                       GdkRGBA     *rgba)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (self), FALSE);

  return bjb_item_get_rgba (BJB_ITEM (self), rgba);
}

const char *
biji_note_obj_get_raw_text (BijiNoteObj *self)
{
  const char *content;

  content = bjb_note_get_raw_content (BJB_NOTE (self));

  if (content && *content)
    return content;

  return bjb_note_get_text_content (BJB_NOTE (self));
}

void
biji_note_obj_set_raw_text (BijiNoteObj *self,
                            const char  *plain_text)
{
  bjb_note_set_raw_content (BJB_NOTE (self), plain_text);
  bjb_item_set_modified (BJB_ITEM (self));
  g_signal_emit (self, biji_obj_signals[NOTE_CHANGED],0);
}

GList *
biji_note_obj_get_notebooks (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (self), NULL);

  priv = biji_note_obj_get_instance_private (self);

  return g_hash_table_get_values (priv->labels);
}

void
biji_note_obj_set_is_template (BijiNoteObj *self,
                               gboolean     is_template)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  priv->is_template = is_template;
}

void
biji_note_obj_save_note (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (self));

  priv->needs_save = TRUE;
  biji_timeout_reset (priv->timeout, 3000);
}

/* Webkit */

char *
html_from_plain_text (const char *content)
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

static gboolean
is_webkit1 (const char *content)
{
  return g_strstr_len (content, -1, "<script type=\"text/javascript\">    window.onload = function () {      document.getElementById('editable').focus();    };</script>") != NULL;
}

static char *
convert_webkit1_to_webkit2 (const char *content)
{
  g_autofree char *stripped = NULL;

  stripped = biji_str_mass_replace (content,
                                    "<html xmlns=\"http://www.w3.org/1999/xhtml\"><body contenteditable=\"true\" id=\"editable\">", "",
                                    "<script type=\"text/javascript\">    window.onload = function () {      document.getElementById('editable').focus();    };</script>", "\n",
                                    "<div><br/></div>", "\n",
                                    "<div>", "",
                                    "</div>", "\n",
                                    "<br/>", "\n",
                                    "</body></html>", "",
                                    NULL);

	return html_from_plain_text (stripped);
}

static gboolean
is_contenteditable_hardcoded (const char *content)
{
  return g_strstr_len (content, -1, "contenteditable=\"true\"") != NULL;
}

static char *
remove_hardcoded_contenteditable (const char *content)
{
  return biji_str_mass_replace (content,
                                "contenteditable=\"true\"", "",
                                NULL);
}

char *
biji_note_obj_get_html (BijiNoteObj *self)
{
  char *content = BIJI_NOTE_OBJ_GET_CLASS (self)->get_html (self);

  if (content && is_webkit1 (content))
    {
      content = convert_webkit1_to_webkit2 (content);
      biji_note_obj_set_html (self, content);
      g_free (content);
      content = BIJI_NOTE_OBJ_GET_CLASS (self)->get_html (self);
    }
  else if (content && is_contenteditable_hardcoded (content))
    {
      content = remove_hardcoded_contenteditable (content);
      biji_note_obj_set_html (self, content);
      g_free (content);
      content = BIJI_NOTE_OBJ_GET_CLASS (self)->get_html (self);
    }

  return content;
}

void
biji_note_obj_set_html (BijiNoteObj *self,
                        const char  *html)
{
  BIJI_NOTE_OBJ_GET_CLASS (self)->set_html (self, html);
}

gboolean
biji_note_obj_can_format (BijiNoteObj *self)
{
  return BIJI_NOTE_OBJ_GET_CLASS (self)->can_format (self);
}

static void
biji_note_obj_class_init (BijiNoteObjClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS  (klass);

  object_class->finalize     = biji_note_obj_finalize;
  object_class->get_property = biji_note_obj_get_property;
  object_class->set_property = biji_note_obj_set_property;

  properties[PROP_PATH] =
    g_param_spec_string("path",
                        "The note file",
                        "The location where the note is stored and saved",
                        NULL,
                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  properties[PROP_MANAGER] =
    g_param_spec_object("manager",
                        "Note Manager",
                        "The Note Manager controlling this item",
                        BIJI_TYPE_MANAGER,
                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, BIJI_OBJ_PROPERTIES, properties);

  biji_obj_signals[NOTE_DELETED] =
    g_signal_new ("deleted",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  biji_obj_signals[NOTE_TRASHED] =
    g_signal_new ("trashed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  biji_obj_signals[NOTE_CHANGED] =
    g_signal_new ("changed",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);

  biji_obj_signals[NOTE_COLOR_CHANGED] =
    g_signal_new ("color-changed" ,
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL,
                  NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
                  0);
}

