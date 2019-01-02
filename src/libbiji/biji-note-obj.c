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
#include "biji-note-id.h"
#include "biji-manager.h"
#include "../gn-utils.h"
#include "biji-note-obj.h"
#include "biji-timeout.h"
#include "biji-tracker.h"

#ifdef BUILD_ZEITGEIST
#include "biji-zeitgeist.h"
#endif /* BUILD_ZEITGEIST */

#include "editor/biji-webkit-editor.h"


#include <libgd/gd.h>

typedef struct
{
  /* Metadata */
  BijiNoteID            *id;
  GdkRGBA               *color; // Not yet in Tracker

  /* Editing use the same widget
   * for all notes provider. */
  BijiWebkitEditor      *editor;

  /* Save */
  BijiTimeout           *timeout;
  gboolean              needs_save;

  /* Icon might be null untill usefull
   * Emblem is smaller & just shows the color */
  cairo_surface_t       *icon;
  cairo_surface_t       *emblem;
  cairo_surface_t       *pristine;

  /* Tags
   * In Tomboy, templates are 'system:notebook:%s' tags.*/
  GHashTable            *labels;
  gboolean              is_template;

  /* Signals */
  gulong note_renamed;
  gulong save;
} BijiNoteObjPrivate;

/* Properties */
enum {
  PROP_0,
  PROP_ID,
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

gboolean is_webkit1 (const char *content);
gchar * convert_webkit1_to_webkit2 (const gchar *content);

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

#ifdef BUILD_ZEITGEIST
  insert_zeitgeist (self, ZEITGEIST_ZG_MODIFY_EVENT);
#endif /* BUILD_ZEITGEIST */

  priv->needs_save = FALSE;
  g_object_unref (self);
}

static void
biji_note_obj_init (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (self);

  priv->timeout = biji_timeout_new ();
  priv->save = g_signal_connect_swapped (priv->timeout, "timeout",
                            G_CALLBACK (on_save_timeout), self);

  priv->labels = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
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

  g_clear_object (&priv->id);

  g_hash_table_destroy (priv->labels);

  if (priv->icon)
    cairo_surface_destroy (priv->icon);

  if (priv->emblem)
    cairo_surface_destroy (priv->emblem);

  if (priv->pristine)
    cairo_surface_destroy (priv->pristine);

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
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (object));

  switch (property_id)
    {
    case PROP_ID:
      /* From now on BijiNoteID is managed by BijiNoteObj */
      priv->id = g_value_get_object (value);
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
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (object));

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_object (value, priv->id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


gboolean
biji_note_obj_are_same (BijiNoteObj *a, BijiNoteObj* b)
{
  BijiNoteObjPrivate *a_priv = biji_note_obj_get_instance_private (a);
  BijiNoteObjPrivate *b_priv = biji_note_obj_get_instance_private (b);

  return biji_note_id_equal (a_priv->id, b_priv->id);
}


/* First cancel timeout
 * this func is most probably stupid it might exists (move file) */
static gboolean
biji_note_obj_trash (BijiItem *item)
{
  BijiNoteObj *note_to_kill;
  BijiNoteObjPrivate *priv;
  GFile *icon;
  gchar *icon_path;
  gboolean result;

  note_to_kill = BIJI_NOTE_OBJ (item);
  priv = biji_note_obj_get_instance_private (note_to_kill);
  icon = NULL;
  icon_path = NULL;
  result = FALSE;

  /* The event has to be logged before the note is actually deleted */
#ifdef BUILD_ZEITGEIST
  insert_zeitgeist (note_to_kill, ZEITGEIST_ZG_DELETE_EVENT);
#endif /* BUILD_ZEITGEIST */

  priv->needs_save = FALSE;
  biji_timeout_cancel (priv->timeout);
  result = BIJI_NOTE_OBJ_GET_CLASS (note_to_kill)->archive (note_to_kill);

  if (result == TRUE)
  {
    /* Delete icon file */
    icon_path = biji_note_obj_get_icon_file (note_to_kill);
    icon = g_file_new_for_path (icon_path);
    g_file_delete (icon, NULL, NULL);
  }


  g_free (icon_path);

  if (icon != NULL)
    g_object_unref (icon);

  return result;
}



gboolean
biji_note_obj_is_trashed                    (BijiNoteObj *self)
{
  return BIJI_NOTE_OBJ_GET_CLASS (self)->is_trashed (self);
}


static const gchar *
biji_note_obj_get_path (BijiItem *item)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), NULL);

  priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (item));

  return biji_note_id_get_path (priv->id);
}

static const gchar *
biji_note_obj_get_title (BijiItem *item)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), NULL);

  priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (item));

  return biji_note_id_get_title (priv->id);
}

/* If already a title, then note is renamed */
gboolean
biji_note_obj_set_title (BijiNoteObj *note, const gchar *proposed_title)
{
  gchar *old_title, *title;
  gboolean retval;
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  title = NULL;
  old_title = g_strdup (biji_note_id_get_title (priv->id));


  if (g_strcmp0 (proposed_title, old_title) == 0)
  {
    retval = FALSE;
    goto out;
  }


  title = biji_manager_get_unique_title (
              biji_item_get_manager (BIJI_ITEM (note)), proposed_title);
  biji_note_id_set_last_metadata_change_date (priv->id,
                                              g_get_real_time () / G_USEC_PER_SEC);


  /* Emit signal even if initial title, just to let know */
  biji_note_id_set_title (priv->id, title);
  g_signal_emit (G_OBJECT (note), biji_obj_signals[NOTE_RENAMED], 0);
  retval = TRUE;

out:
  if (old_title != NULL)
    g_free (old_title);

  if (title != NULL)
    g_free (title);

  return retval;
}


gboolean
biji_note_obj_set_mtime (BijiNoteObj* n, gint64 mtime)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (n), FALSE);

  priv = biji_note_obj_get_instance_private (n);

  return biji_note_id_set_mtime (priv->id, mtime);
}


static gint64
biji_note_obj_get_mtime (BijiItem *note)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (note), 0);

  priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (note));

  return biji_note_id_get_mtime (priv->id);
}

gchar *
biji_note_obj_get_last_change_date_string (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (self), g_strdup (""));

  priv = biji_note_obj_get_instance_private (self);

  return gn_utils_get_human_time (biji_note_id_get_mtime (priv->id));
}


gint64
biji_note_obj_get_last_metadata_change_date (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (note), 0);

  priv = biji_note_obj_get_instance_private (note);

  return biji_note_id_get_last_metadata_change_date (priv->id);
}


gboolean
biji_note_obj_set_last_metadata_change_date (BijiNoteObj* n, gint64 time)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ(n), FALSE);

  priv = biji_note_obj_get_instance_private (n);

  return biji_note_id_set_last_metadata_change_date (priv->id, time);
}

static void
biji_note_obj_clear_icons (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  g_clear_pointer (&priv->icon, cairo_surface_destroy);
  g_clear_pointer (&priv->emblem, cairo_surface_destroy);
  g_clear_pointer (&priv->pristine, cairo_surface_destroy);
}

static void
biji_note_obj_set_rgba_internal (BijiNoteObj *n,
                                 const GdkRGBA *rgba)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (n);

  priv->color = gdk_rgba_copy(rgba);

  g_signal_emit (G_OBJECT (n), biji_obj_signals[NOTE_COLOR_CHANGED],0);
}

void
biji_note_obj_set_rgba (BijiNoteObj *n,
                        const GdkRGBA *rgba)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (n);

  if (!priv->color)
    biji_note_obj_set_rgba_internal (n, rgba);

  else if (!gdk_rgba_equal (priv->color,rgba))
  {
    gdk_rgba_free (priv->color);
    biji_note_obj_clear_icons (n);
    biji_note_obj_set_rgba_internal (n, rgba);

    biji_note_id_set_last_metadata_change_date (priv->id, g_get_real_time () / G_USEC_PER_SEC);
    biji_note_obj_save_note (n);
  }
}

gboolean
biji_note_obj_get_rgba(BijiNoteObj *n,
                       GdkRGBA *rgba)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (n);

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (n), FALSE);

  if (priv->color && rgba)
    {
      *rgba = *(priv->color);
      return TRUE;
    }

  return FALSE;
}


const gchar *
biji_note_obj_get_raw_text                  (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  return biji_note_id_get_content (priv->id);
}

void
biji_note_obj_set_raw_text (BijiNoteObj *note,
                            const gchar *plain_text)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  if (biji_note_id_set_content (priv->id, plain_text))
  {
    biji_note_obj_clear_icons (note);
    g_signal_emit (note, biji_obj_signals[NOTE_CHANGED],0);
  }
}

GList *
biji_note_obj_get_notebooks (BijiNoteObj *n)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (n), NULL);

  priv = biji_note_obj_get_instance_private (n);

  return g_hash_table_get_values (priv->labels);
}

static gboolean
biji_note_obj_has_notebook (BijiItem *item, gchar *label)
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
biji_note_obj_add_notebook (BijiItem *item,
                              BijiItem *notebook,
                              gchar    *title)
{
  BijiNoteObj *note;
  BijiNoteObjPrivate *priv;
  gchar *label = title;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), FALSE);
  note = BIJI_NOTE_OBJ (item);
  priv = biji_note_obj_get_instance_private (note);

  if (BIJI_IS_NOTEBOOK (notebook))
    label = (gchar*) biji_item_get_title (notebook);

  if (biji_note_obj_has_notebook (item, label))
    return FALSE;

  g_hash_table_add (priv->labels, g_strdup (label));

  if (BIJI_IS_NOTEBOOK (notebook))
  {
    biji_push_existing_notebook_to_note (
      note, label, _biji_notebook_refresh, notebook); // Tracker
    biji_note_id_set_last_metadata_change_date (priv->id,
                                                g_get_real_time () / G_USEC_PER_SEC);
    biji_note_obj_save_note (note);
  }

  return TRUE;
}


static gboolean
biji_note_obj_remove_notebook (BijiItem *item, BijiItem *notebook)
{
  BijiNoteObj *note;
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), FALSE);
  g_return_val_if_fail (BIJI_IS_NOTEBOOK (notebook), FALSE);

  note = BIJI_NOTE_OBJ (item);
  priv = biji_note_obj_get_instance_private (note);

  if (g_hash_table_remove (priv->labels, biji_item_get_title (notebook)))
  {
    biji_remove_notebook_from_note (
      note, notebook, _biji_notebook_refresh, notebook); // tracker.
    biji_note_id_set_last_metadata_change_date (priv->id,
                                                g_get_real_time () / G_USEC_PER_SEC);
    biji_note_obj_save_note (note);
    return TRUE;
  }

  return FALSE;
}

gboolean
note_obj_is_template (BijiNoteObj *n)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail(BIJI_IS_NOTE_OBJ(n),FALSE);

  priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (n));

  return priv->is_template;
}

void
note_obj_set_is_template (BijiNoteObj *n,gboolean is_template)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (n));

  priv->is_template = is_template;
}

void
biji_note_obj_save_note (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (self));

  priv->needs_save = TRUE;
  biji_timeout_reset (priv->timeout, 3000);
}

gchar *
biji_note_obj_get_icon_file (BijiNoteObj *note)
{
  const gchar *uuid;
  gchar *basename, *filename;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (note), NULL);

  uuid = BIJI_NOTE_OBJ_GET_CLASS (note)->get_basename (note);
  basename = biji_str_mass_replace (uuid, ".note", ".png", ".txt", ".png", NULL);

  filename = g_build_filename (g_get_user_cache_dir (),
                               g_get_application_name (),
                               basename,
                               NULL);

  g_free (basename);
  return filename;
}

static cairo_surface_t *
biji_note_obj_get_icon (BijiItem *item,
                        gint scale)
{
  GdkRGBA               note_color;
  const gchar           *text;
  cairo_t               *cr;
  PangoLayout           *layout;
  PangoFontDescription  *desc;
  cairo_surface_t       *surface = NULL;
  GtkBorder              frame_slice = { 4, 3, 3, 6 };
  BijiNoteObj *note = BIJI_NOTE_OBJ (item);
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  if (priv->icon)
    return priv->icon;

  /* Create & Draw surface */
  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32,
                                        BIJI_ICON_WIDTH * scale,
                                        BIJI_ICON_HEIGHT * scale) ;
  cairo_surface_set_device_scale (surface, scale, scale);
  cr = cairo_create (surface);

  /* Background */
  cairo_rectangle (cr, 0, 0, BIJI_ICON_WIDTH, BIJI_ICON_HEIGHT);
  if (biji_note_obj_get_rgba (note, &note_color))
    gdk_cairo_set_source_rgba (cr, &note_color);

  cairo_fill (cr);

  /* Text */
  text = biji_note_id_get_content (priv->id);
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
  }

  cairo_destroy (cr);

  priv->icon = gd_embed_surface_in_frame (surface, "resource:///org/gnome/Notes/thumbnail-frame.png",
                                                &frame_slice, &frame_slice);
  cairo_surface_destroy (surface);

  return priv->icon;
}

static cairo_surface_t *
biji_note_obj_get_pristine (BijiItem *item,
                            gint scale)
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
biji_note_obj_get_emblem (BijiItem *item,
                          gint scale)
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
biji_note_obj_set_all_dates_now             (BijiNoteObj *note)
{
  gint64 time;
  BijiNoteID *id;
  BijiNoteObjPrivate *priv;

  g_return_if_fail (BIJI_IS_NOTE_OBJ (note));

  priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (note));
  id = priv->id;
  time = g_get_real_time () / G_USEC_PER_SEC;
  biji_note_id_set_create_date (id, time);
  biji_note_id_set_last_metadata_change_date (id, time);
  biji_note_id_set_mtime (id, time);
}


gboolean
biji_note_obj_is_template(BijiNoteObj *note)
{
  return note_obj_is_template(note);
}


gint64
biji_note_obj_get_create_date (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (note), 0);

  priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (note));

  return biji_note_id_get_create_date (priv->id);
}


gboolean
biji_note_obj_set_create_date (BijiNoteObj *note, gint64 time)
{
  BijiNoteObjPrivate *priv;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (note), FALSE);

  priv = biji_note_obj_get_instance_private (BIJI_NOTE_OBJ (note));

  return biji_note_id_set_create_date (priv->id, time);
}


/* Webkit */

gchar *
html_from_plain_text (const gchar *content)
{
  gchar *escaped, *retval;

  if (content == NULL)
    content = "";

  escaped = biji_str_mass_replace (content,
                                "&", "&amp;",
                                "<", "&lt;",
                                ">", "&gt;",
                                "\n", "<br/>",
                                NULL);

  retval = g_strconcat ("<html xmlns=\"http://www.w3.org/1999/xhtml\">",
                        "<head>",
                        "<link rel='stylesheet' href='Default.css' type='text/css'/>",
                        "<script language='javascript' src='bijiben.js'></script>"
                        "</head>",
                        "<body contenteditable='true' id='editable'>",
                        escaped,
                        "</body></html>", NULL);

  g_free (escaped);
  return retval;
}

gboolean
is_webkit1 (const char *content)
{
	if (g_strstr_len (content, -1, "<script type=\"text/javascript\">    window.onload = function () {      document.getElementById('editable').focus();    };</script>") == NULL)
		return FALSE;

	return TRUE;
}

gchar *
convert_webkit1_to_webkit2 (const gchar *content)
{
	gchar *stripped = NULL;
	gchar *webkit2 = NULL;

	stripped = biji_str_mass_replace (content,
																		"<html xmlns=\"http://www.w3.org/1999/xhtml\"><body contenteditable=\"true\" id=\"editable\">", "",
																		"<script type=\"text/javascript\">    window.onload = function () {      document.getElementById('editable').focus();    };</script>", "\n",
																		"<div><br/></div>", "\n",
																		"<div>", "",
																		"</div>", "\n",
																		"<br/>", "\n",
																		"</body></html>", "",
																		NULL);

	webkit2 = html_from_plain_text (stripped);

	g_free (stripped);

	return webkit2;
}

gchar *
biji_note_obj_get_html (BijiNoteObj *note)
{
	gchar *content = BIJI_NOTE_OBJ_GET_CLASS (note)->get_html (note);

	if (content && is_webkit1 (content))
		{
			content = convert_webkit1_to_webkit2 (content);
			biji_note_obj_set_html (note, content);
			g_free (content);
			content = BIJI_NOTE_OBJ_GET_CLASS (note)->get_html (note);
		}

	return content;
}

void
biji_note_obj_set_html (BijiNoteObj *note,
                        const gchar *html)
{
  BIJI_NOTE_OBJ_GET_CLASS (note)->set_html (note, html);
}

gboolean
biji_note_obj_is_opened (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  return BIJI_IS_WEBKIT_EDITOR (priv->editor);
}

static void
on_biji_note_obj_closed_cb (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv;
  BijiItem *item;
  const gchar *title;

  priv = biji_note_obj_get_instance_private (note);
  item = BIJI_ITEM (note);
  priv->editor = NULL;
  title = biji_item_get_title (item);

#ifdef BUILD_ZEITGEIST
  insert_zeitgeist (note, ZEITGEIST_ZG_LEAVE_EVENT);
#endif /* BUILD_ZEITGEIST */

  /*
   * Delete (not _trash_ if note is totaly blank
   * A Cancellable would be better than needs->save
   */
  if (biji_note_id_get_content (priv->id) == NULL)
  {
    priv->needs_save = FALSE;
    biji_item_delete (item);
  }

  /* If the note has no title */
  else if (title == NULL)
  {
    title = biji_note_obj_get_raw_text (note);
    biji_note_obj_set_title (note, title);
  }
}

GtkWidget *
biji_note_obj_open (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  priv->editor = biji_webkit_editor_new (note);

  g_signal_connect_swapped (priv->editor, "destroy",
                            G_CALLBACK (on_biji_note_obj_closed_cb), note);

#ifdef BUILD_ZEITGEIST
  insert_zeitgeist (note, ZEITGEIST_ZG_ACCESS_EVENT);
#endif /* BUILD_ZEITGEIST */

  return GTK_WIDGET (priv->editor);
}

GtkWidget *
biji_note_obj_get_editor (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  if (!biji_note_obj_is_opened (note))
    return NULL;

  return GTK_WIDGET (priv->editor);
}


gboolean
biji_note_obj_can_format       (BijiNoteObj *note)
{
  return BIJI_NOTE_OBJ_GET_CLASS (note)->can_format (note);
}


void
biji_note_obj_editor_apply_format (BijiNoteObj *note, gint format)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  if (biji_note_obj_is_opened (note))
    biji_webkit_editor_apply_format (priv->editor, format);
}

gboolean
biji_note_obj_editor_has_selection (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  if (biji_note_obj_is_opened (note))
    return biji_webkit_editor_has_selection (priv->editor);

  return FALSE;
}

const gchar *
biji_note_obj_editor_get_selection (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  if (biji_note_obj_is_opened (note))
    return biji_webkit_editor_get_selection (priv->editor);

  return NULL;
}

void biji_note_obj_editor_cut (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  if (biji_note_obj_is_opened (note))
    biji_webkit_editor_cut (priv->editor);
}

void biji_note_obj_editor_copy (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  if (biji_note_obj_is_opened (note))
    biji_webkit_editor_copy (priv->editor);
}

void biji_note_obj_editor_paste (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv = biji_note_obj_get_instance_private (note);

  if (biji_note_obj_is_opened (note))
    biji_webkit_editor_paste (priv->editor);
}

static void
biji_note_obj_class_init (BijiNoteObjClass *klass)
{
  BijiItemClass*  item_class = BIJI_ITEM_CLASS (klass);
  GObjectClass* object_class = G_OBJECT_CLASS  (klass);

  object_class->constructed = biji_note_obj_constructed;
  object_class->finalize = biji_note_obj_finalize;
  object_class->get_property = biji_note_obj_get_property;
  object_class->set_property = biji_note_obj_set_property;

  properties[PROP_ID] =
    g_param_spec_object("id",
                        "The note id",
                        "The basic information about the note",
                        BIJI_TYPE_NOTE_ID,
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
  item_class->get_title = biji_note_obj_get_title;
  item_class->get_uuid = biji_note_obj_get_path;
  item_class->get_icon = biji_note_obj_get_icon;
  item_class->get_emblem = biji_note_obj_get_emblem;
  item_class->get_pristine = biji_note_obj_get_pristine;
  item_class->get_mtime = biji_note_obj_get_mtime;
  item_class->trash = biji_note_obj_trash;
  item_class->has_notebook = biji_note_obj_has_notebook;
  item_class->add_notebook = biji_note_obj_add_notebook;
  item_class->remove_notebook = biji_note_obj_remove_notebook;
}

