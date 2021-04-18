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
#include "biji-note-obj.h"
#include "biji-timeout.h"
#include "biji-tracker.h"

#include "editor/biji-webkit-editor.h"

typedef struct
{
  /* Metadata */
  char                  *path;
  char                  *title;
  char                  *content;
  gint64                 mtime;
  gint64                 create_date;
  gint64                 last_metadata_change_date;
  GdkRGBA               *color; // Not yet in Tracker

  /* Editing use the same widget
   * for all notes provider. */
  BijiWebkitEditor      *editor;

  /* Save */
  BijiTimeout           *timeout;
  gboolean               needs_save;

  /* Icon might be null until useful
   * Emblem is smaller & just shows the color */
  cairo_surface_t       *icon;
  cairo_surface_t       *emblem;
  cairo_surface_t       *pristine;

  /* Tags
   * In Tomboy, templates are 'system:notebook:%s' tags.*/
  GHashTable            *labels;
  gboolean               is_template;

  /* Signals */
  gulong                 note_renamed;
  gulong                 save;
} BijiNoteObjPrivate;

/* Properties */
enum {
  PROP_0,
  PROP_PATH,
  PROP_TITLE,
  PROP_MTIME,
  PROP_CONTENT,
  BIJI_OBJ_PROPERTIES
};

static GParamSpec *properties[BIJI_OBJ_PROPERTIES] = { NULL, };

/* Signals. Do not interfere with biji-item parent class. */
enum {
  NOTE_RENAMED,
  NOTE_CHANGED,
  NOTE_COLOR_CHANGED,
  BIJI_OBJ_SIGNALS
};

static guint biji_obj_signals [BIJI_OBJ_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (BijiNoteObj, biji_note_obj, BIJI_TYPE_ITEM)

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
biji_note_obj_init (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  priv->timeout = biji_timeout_new ();
  priv->save = g_signal_connect_swapped (priv->timeout, "timeout", G_CALLBACK (on_save_timeout), self);
  priv->labels = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
biji_note_obj_clear_icons (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  g_clear_pointer (&priv->icon, cairo_surface_destroy);
  g_clear_pointer (&priv->emblem, cairo_surface_destroy);
  g_clear_pointer (&priv->pristine, cairo_surface_destroy);
}

static void
biji_note_obj_finalize (GObject *object)
{
  BijiNoteObj        *self = BIJI_NOTE_OBJ(object);
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (priv->timeout)
    g_object_unref (priv->timeout);

  if (priv->needs_save)
    on_save_timeout (self);

  g_free (priv->path);
  g_free (priv->title);
  g_free (priv->content);

  g_hash_table_destroy (priv->labels);

  biji_note_obj_clear_icons (self);

  gdk_rgba_free (priv->color);

  G_OBJECT_CLASS (biji_note_obj_parent_class)->finalize (object);
}

/* we do NOT deserialize here. it might be a brand new note
 * it's up the manager to ask .note to be read*/
static void
biji_note_obj_constructed (GObject *obj)
{
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
      break;
    case PROP_TITLE:
      biji_note_obj_set_title (self, (char *) g_value_get_string (value));
      g_object_notify_by_pspec (object, properties[PROP_TITLE]);
      break;
    case PROP_MTIME:
      priv->mtime = g_value_get_int64 (value);
      break;
    case PROP_CONTENT:
      biji_note_obj_set_raw_text (self, g_value_get_string (value));
      g_object_notify_by_pspec (object, properties[PROP_CONTENT]);
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
      g_value_set_object (value, priv->path);
      break;
    case PROP_TITLE:
      g_value_set_string (value, priv->title);
      break;
    case PROP_MTIME:
      g_value_set_int64 (value, priv->mtime);
      break;
    case PROP_CONTENT:
      g_value_set_string (value, priv->content);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

gboolean
biji_note_obj_are_same (BijiNoteObj *note_a,
                        BijiNoteObj *note_b)
{
  BijiNoteObjPrivate *a_priv = biji_note_obj_get_instance_private (note_a);
  BijiNoteObjPrivate *b_priv = biji_note_obj_get_instance_private (note_b);

  return (g_strcmp0 (a_priv->path,    b_priv->path)    == 0 &&
          g_strcmp0 (a_priv->content, b_priv->content) == 0);

}

/* First cancel timeout
 * this func is most probably stupid it might exists (move file) */
static gboolean
trash (BijiItem *item)
{
  BijiNoteObj        *self      = BIJI_NOTE_OBJ (item);
  BijiNoteObjPrivate *priv      = biji_note_obj_get_instance_private (self);
  GFile              *icon      = NULL;
  char               *icon_path = NULL;
  gboolean            result    = FALSE;

  priv->needs_save = FALSE;
  biji_timeout_cancel (priv->timeout);
  result = BIJI_NOTE_OBJ_GET_CLASS (self)->archive (self);

  if (result == TRUE)
    {
      /* Delete icon file */
      icon_path = biji_note_obj_get_icon_file (self);
      icon = g_file_new_for_path (icon_path);
      g_file_delete (icon, NULL, NULL);
    }

  g_free (icon_path);

  if (icon != NULL)
    g_object_unref (icon);

  return result;
}

gboolean
biji_note_obj_is_trashed (BijiNoteObj *self)
{
  return BIJI_NOTE_OBJ_GET_CLASS (self)->is_trashed (self);
}

const char *
biji_note_obj_get_path (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  return priv->path;
}

static const char *
get_path (BijiItem *item)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), NULL);

  return biji_note_obj_get_path (BIJI_NOTE_OBJ (item));
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
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_PATH]);
}

const char *
biji_note_obj_get_title (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  return priv->title;
}

static const char *
get_title (BijiItem *item)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), NULL);

  return biji_note_obj_get_title (BIJI_NOTE_OBJ (item));
}

/* If already a title, then note is renamed */
gboolean
biji_note_obj_set_title (BijiNoteObj *self,
                         const char  *proposed_title)
{
  BijiNoteObjPrivate *priv    = biji_note_obj_get_instance_private (self);
  BijiManager        *manager = biji_item_get_manager (BIJI_ITEM (self));
  g_autofree char    *title   = NULL;

  if (g_strcmp0 (proposed_title, priv->title) == 0)
    return FALSE;

  title = biji_manager_get_unique_title (manager, proposed_title);
  biji_note_obj_set_last_metadata_change_date (self, g_get_real_time () / G_USEC_PER_SEC);

  /* Emit signal even if initial title, just to let know */
  g_free (priv->title);
  priv->title = g_strdup (title);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
  biji_note_obj_save_note (self);
  g_signal_emit (G_OBJECT (self), biji_obj_signals[NOTE_RENAMED], 0);

  return TRUE;
}

gboolean
biji_note_obj_set_mtime (BijiNoteObj *self,
                         gint64       time)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (priv->mtime == time)
    return FALSE;

  priv->mtime = time;
  return TRUE;
}

static gint64
get_mtime (BijiItem *item)
{
  BijiNoteObj        *self = BIJI_NOTE_OBJ (item);
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  return priv->mtime;
}

char *
biji_note_obj_get_last_change_date_string (BijiNoteObj *self)
{
  BijiItem *item = BIJI_ITEM (self);

  return bjb_utils_get_human_time (get_mtime (item));
}

gint64
biji_note_obj_get_last_metadata_change_date (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  return priv->last_metadata_change_date;
}

gboolean
biji_note_obj_set_last_metadata_change_date (BijiNoteObj *self,
                                             gint64       time)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (priv->last_metadata_change_date == time)
    return FALSE;

  priv->last_metadata_change_date = time;
  return TRUE;
}

static void
biji_note_obj_set_rgba_internal (BijiNoteObj   *self,
                                 const GdkRGBA *rgba)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  priv->color = gdk_rgba_copy(rgba);
  g_signal_emit (G_OBJECT (self), biji_obj_signals[NOTE_COLOR_CHANGED], 0);
}

void
biji_note_obj_set_rgba (BijiNoteObj   *self,
                        const GdkRGBA *rgba)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (!priv->color)
    biji_note_obj_set_rgba_internal (self, rgba);

  else if (!gdk_rgba_equal (priv->color,rgba))
    {
      gdk_rgba_free (priv->color);
      biji_note_obj_clear_icons (self);
      biji_note_obj_set_rgba_internal (self, rgba);
      biji_note_obj_set_last_metadata_change_date (self, g_get_real_time () / G_USEC_PER_SEC);
      biji_note_obj_save_note (self);
    }
}

gboolean
biji_note_obj_get_rgba(BijiNoteObj *self,
                       GdkRGBA     *rgba)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (self), FALSE);

  if (priv->color && rgba)
    {
      *rgba = *(priv->color);
      return TRUE;
    }

  return FALSE;
}

const char *
biji_note_obj_get_raw_text (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  return priv->content;
}

void
biji_note_obj_set_raw_text (BijiNoteObj *self,
                            const char  *plain_text)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (g_strcmp0 (plain_text, priv->content) == 0)
    return;

  g_free (priv->content);
  priv->content = g_strdup (plain_text);
  biji_note_obj_clear_icons (self);
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

static gboolean
has_notebook (BijiItem *item,
              char     *label)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (item));

  if (g_hash_table_lookup (priv->labels, label))
    return TRUE;

  return FALSE;
}

static void
_biji_notebook_refresh (gboolean query_result,
                        gpointer coll)
{
  g_return_if_fail (BIJI_IS_NOTEBOOK (coll));

  if (query_result)
    biji_notebook_refresh (BIJI_NOTEBOOK (coll));
}

static gboolean
add_notebook (BijiItem *item,
              BijiItem *notebook,
              char     *title)
{
  BijiNoteObj        *self;
  BijiNoteObjPrivate *priv;
  char               *label = title;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), FALSE);
  self = BIJI_NOTE_OBJ (item);
  priv = biji_note_obj_get_instance_private (self);

  if (BIJI_IS_NOTEBOOK (notebook))
    label = (char*) biji_item_get_title (notebook);

  if (has_notebook (item, label))
    return FALSE;

  g_hash_table_add (priv->labels, g_strdup (label));

  if (BIJI_IS_NOTEBOOK (notebook))
    {
      biji_push_existing_notebook_to_note (self, label, _biji_notebook_refresh, notebook); // Tracker
      biji_note_obj_set_last_metadata_change_date (self, g_get_real_time () / G_USEC_PER_SEC);
      biji_note_obj_save_note (self);
    }

  return TRUE;
}

static gboolean
remove_notebook (BijiItem *item,
                 BijiItem *notebook)
{
  BijiNoteObj        *self;
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), FALSE);
  g_return_val_if_fail (BIJI_IS_NOTEBOOK (notebook), FALSE);

  self = BIJI_NOTE_OBJ (item);
  priv = biji_note_obj_get_instance_private (self);

  if (g_hash_table_remove (priv->labels, biji_item_get_title (notebook)))
    {
      biji_remove_notebook_from_note (self, notebook, _biji_notebook_refresh, notebook); // tracker.
      biji_note_obj_set_last_metadata_change_date (self, g_get_real_time () / G_USEC_PER_SEC);
      biji_note_obj_save_note (self);
      return TRUE;
    }

  return FALSE;
}

gboolean
biji_note_obj_is_template (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  return priv->is_template;
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

char *
biji_note_obj_get_icon_file (BijiNoteObj *self)
{
  const char *uuid;
  char *basename, *filename;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (self), NULL);

  uuid = BIJI_NOTE_OBJ_GET_CLASS (self)->get_basename (self);
  basename = biji_str_mass_replace (uuid, ".note", ".png", ".txt", ".png", NULL);

  filename = g_build_filename (g_get_user_cache_dir (),
                               g_get_application_name (),
                               basename,
                               NULL);

  g_free (basename);
  return filename;
}

static cairo_surface_t *
get_icon (BijiItem *item,
          int       scale)
{
  GdkRGBA                note_color;
  const char            *text;
  cairo_t               *cr;
  PangoLayout           *layout;
  PangoFontDescription  *desc;
  cairo_surface_t       *surface = NULL;
  BijiNoteObj           *note = BIJI_NOTE_OBJ (item);
  BijiNoteObjPrivate    *priv = biji_note_obj_get_instance_private (note);

  if (priv->icon)
    return priv->icon;

  /* Create & Draw surface */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        BIJI_ICON_WIDTH * scale,
                                        BIJI_ICON_HEIGHT * scale);
  cairo_surface_set_device_scale (surface, scale, scale);
  cr = cairo_create (surface);

  /* Background */
  cairo_rectangle (cr, 0, 0, BIJI_ICON_WIDTH, BIJI_ICON_HEIGHT);
  if (biji_note_obj_get_rgba (note, &note_color))
    gdk_cairo_set_source_rgba (cr, &note_color);

  cairo_fill (cr);

  /* Text */
  text = biji_note_obj_get_raw_text (note);
  if (text != NULL)
    {
      cairo_translate (cr, 10, 10);
      layout = pango_cairo_create_layout (cr);

      pango_layout_set_width (layout, 180000 );
      pango_layout_set_wrap (layout, PANGO_WRAP_WORD_CHAR);
      pango_layout_set_height (layout, 180000 ) ;

      pango_layout_set_text (layout, text, -1);
      desc = pango_font_description_from_string (BIJI_ICON_FONT);
      pango_layout_set_font_description (layout, desc);
      pango_font_description_free (desc);

      if(note_color.red < 0.5)
        cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
      else
        cairo_set_source_rgb (cr, 0.0, 0.0, 0.0);

      pango_cairo_update_layout (cr, layout);
      pango_cairo_show_layout (cr, layout);

      g_object_unref (layout);

      cairo_translate (cr, -10, -10);
    }

  /* Border */
  cairo_set_source_rgba (cr, 0.3, 0.3, 0.3, 1);
  cairo_set_line_width (cr, 1 * scale);
  cairo_rectangle (cr, 0, 0, BIJI_ICON_WIDTH, BIJI_ICON_HEIGHT);
  cairo_stroke (cr);

  cairo_destroy (cr);

  priv->icon = surface;
  return priv->icon;
}

static cairo_surface_t *
get_pristine (BijiItem *item,
              int       scale)
{
  GdkRGBA                note_color;
  cairo_t               *cr;
  cairo_surface_t       *surface = NULL;
  BijiNoteObj           *note = BIJI_NOTE_OBJ (item);
  BijiNoteObjPrivate    *priv = biji_note_obj_get_instance_private (note);

  if (priv->pristine)
    return priv->pristine;

  /* Create & Draw surface */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        BIJI_EMBLEM_WIDTH * scale,
                                        BIJI_EMBLEM_HEIGHT * scale) ;
  cairo_surface_set_device_scale (surface, scale, scale);
  cr = cairo_create (surface);

  /* Background */
  cairo_rectangle (cr, 0, 0, BIJI_EMBLEM_WIDTH, BIJI_EMBLEM_HEIGHT);
  if (biji_note_obj_get_rgba (note, &note_color))
    gdk_cairo_set_source_rgba (cr, &note_color);

  cairo_fill (cr);
  cairo_destroy (cr);

  priv->pristine = surface;
  return priv->pristine;
}

static cairo_surface_t *
get_emblem (BijiItem *item,
            int       scale)
{
  GdkRGBA                note_color;
  cairo_t               *cr;
  cairo_surface_t       *surface = NULL;
  BijiNoteObj           *note = BIJI_NOTE_OBJ (item);
  BijiNoteObjPrivate    *priv = biji_note_obj_get_instance_private (note);

  if (priv->emblem)
    return priv->emblem;

  /* Create & Draw surface */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        BIJI_EMBLEM_WIDTH * scale,
                                        BIJI_EMBLEM_HEIGHT * scale) ;
  cairo_surface_set_device_scale (surface, scale, scale);
  cr = cairo_create (surface);

  /* Background */
  cairo_rectangle (cr, 0, 0, BIJI_EMBLEM_WIDTH, BIJI_EMBLEM_HEIGHT);
  if (biji_note_obj_get_rgba (note, &note_color))
    gdk_cairo_set_source_rgba (cr, &note_color);

  cairo_fill (cr);

  /* Border */
  cairo_set_source_rgba (cr, 0.3, 0.3, 0.3, 1);
  cairo_set_line_width (cr, 1 * scale);
  cairo_rectangle (cr, 0, 0, BIJI_EMBLEM_WIDTH, BIJI_EMBLEM_HEIGHT);
  cairo_stroke (cr);

  cairo_destroy (cr);

  priv->emblem = surface;
  return priv->emblem;
}

/* Single Note */

void
biji_note_obj_set_all_dates_now (BijiNoteObj *self)
{
  gint64 time = g_get_real_time () / G_USEC_PER_SEC;

  biji_note_obj_set_create_date (self, time);
  biji_note_obj_set_last_metadata_change_date (self, time);
  biji_note_obj_set_mtime (self, time);
}

gint64
biji_note_obj_get_create_date (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  return priv->create_date;
}

gboolean
biji_note_obj_set_create_date (BijiNoteObj *self,
                               gint64       time)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (priv->create_date == time)
    return FALSE;

  priv->create_date = time;
  return TRUE;
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
biji_note_obj_is_opened (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  return BIJI_IS_WEBKIT_EDITOR (priv->editor);
}

static void
on_biji_note_obj_closed_cb (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv;
  BijiItem *item;
  const char *title;

  priv = biji_note_obj_get_instance_private (self);
  item = BIJI_ITEM (self);
  priv->editor = NULL;
  title = biji_item_get_title (item);

  /*
   * Delete (not _trash_ if note is totaly blank
   * A Cancellable would be better than needs->save
   */
  if (biji_note_obj_get_raw_text (self) == NULL)
    {
      priv->needs_save = FALSE;
      biji_item_delete (item);
    }

  /* If the note has no title */
  else if (title == NULL)
    {
      title = biji_note_obj_get_raw_text (self);
      biji_note_obj_set_title (self, title);
    }
}

GtkWidget *
biji_note_obj_open (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  priv->editor = biji_webkit_editor_new (self);

  g_signal_connect_swapped (priv->editor, "destroy",
                            G_CALLBACK (on_biji_note_obj_closed_cb), self);

  return GTK_WIDGET (priv->editor);
}

GtkWidget *
biji_note_obj_get_editor (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (!biji_note_obj_is_opened (self))
    return NULL;

  return GTK_WIDGET (priv->editor);
}

gboolean
biji_note_obj_can_format (BijiNoteObj *self)
{
  return BIJI_NOTE_OBJ_GET_CLASS (self)->can_format (self);
}

void
biji_note_obj_editor_apply_format (BijiNoteObj *self,
                                   int          format)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (biji_note_obj_is_opened (self))
    biji_webkit_editor_apply_format (priv->editor, format);
}

gboolean
biji_note_obj_editor_has_selection (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (biji_note_obj_is_opened (self))
    return biji_webkit_editor_has_selection (priv->editor);

  return FALSE;
}

const char *
biji_note_obj_editor_get_selection (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (biji_note_obj_is_opened (self))
    return biji_webkit_editor_get_selection (priv->editor);

  return NULL;
}

void
biji_note_obj_editor_cut (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (biji_note_obj_is_opened (self))
    biji_webkit_editor_cut (priv->editor);
}

void
biji_note_obj_editor_copy (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (biji_note_obj_is_opened (self))
    biji_webkit_editor_copy (priv->editor);
}

void
biji_note_obj_editor_paste (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  if (biji_note_obj_is_opened (self))
    biji_webkit_editor_paste (priv->editor);
}

static void
biji_note_obj_class_init (BijiNoteObjClass *klass)
{
  GObjectClass  *object_class = G_OBJECT_CLASS  (klass);
  BijiItemClass *item_class   = BIJI_ITEM_CLASS (klass);

  object_class->constructed  = biji_note_obj_constructed;
  object_class->finalize     = biji_note_obj_finalize;
  object_class->get_property = biji_note_obj_get_property;
  object_class->set_property = biji_note_obj_set_property;

  properties[PROP_PATH] =
    g_param_spec_string("path",
                        "The note file",
                        "The location where the note is stored and saved",
                        NULL,
                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  properties[PROP_TITLE] =
    g_param_spec_string("title",
                        "The note title",
                        "Note current title",
                        NULL,
                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  properties[PROP_MTIME] =
    g_param_spec_int64 ("mtime",
                        "Msec since epoch",
                        "The note last modification time",
                        G_MININT64, G_MAXINT64, 0,
                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE);


  properties[PROP_CONTENT] =
    g_param_spec_string("content",
                        "The note content",
                        "Plain text note content",
                        NULL,
                        G_PARAM_CONSTRUCT | G_PARAM_READWRITE);

  g_object_class_install_properties (object_class, BIJI_OBJ_PROPERTIES, properties);

  biji_obj_signals[NOTE_RENAMED] =
    g_signal_new ("renamed",
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

  /* Interface
   * is_collectable is implemented at higher level, eg local_note */
  item_class->get_title       = get_title;
  item_class->get_uuid        = get_path;
  item_class->get_icon        = get_icon;
  item_class->get_emblem      = get_emblem;
  item_class->get_pristine    = get_pristine;
  item_class->get_mtime       = get_mtime;
  item_class->trash           = trash;
  item_class->has_notebook    = has_notebook;
  item_class->add_notebook    = add_notebook;
  item_class->remove_notebook = remove_notebook;
}

