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
#include "biji-note-obj.h"
#include "biji-timeout.h"
#include "biji-tracker.h"

#ifdef BUILD_ZEITGEIST
#include "biji-zeitgeist.h"
#endif /* BUILD_ZEITGEIST */

#include "editor/biji-webkit-editor.h"


#include <libgd/gd.h>

struct _BijiNoteObjPrivate
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
};

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


#define BIJI_NOTE_OBJ_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BIJI_TYPE_NOTE_OBJ, BijiNoteObjPrivate))

G_DEFINE_TYPE (BijiNoteObj, biji_note_obj, BIJI_TYPE_ITEM);

static void
on_save_timeout (BijiNoteObj *self)
{
  BijiNoteObjPrivate *priv = self->priv;

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
  BijiNoteObjPrivate *priv ;

  priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_NOTE_OBJ, BijiNoteObjPrivate);

  self->priv = priv ;
  priv->id = NULL;

  priv->needs_save = FALSE;
  priv->timeout = biji_timeout_new ();
  priv->save = g_signal_connect_swapped (priv->timeout, "timeout",
                            G_CALLBACK (on_save_timeout), self);


  priv->is_template = FALSE ;

  /* The editor is NULL so we know it's not opened
   * neither fully deserialized */
  priv->editor = NULL;

  /* Icon is only computed when necessary */
  priv->icon = NULL;
  priv->emblem = NULL;
  priv->pristine = NULL;

  /* Keep value unitialied, so bijiben knows to assign default color */
  priv->color = NULL;

  priv->labels = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
}

static void
biji_note_obj_finalize (GObject *object)
{
  BijiNoteObj        *self = BIJI_NOTE_OBJ(object);
  BijiNoteObjPrivate *priv = self->priv;

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
  BijiNoteObj *self = BIJI_NOTE_OBJ (object);


  switch (property_id)
    {
    case PROP_ID:
      self->priv->id = g_value_dup_object (value);
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
  BijiNoteObj *self = BIJI_NOTE_OBJ (object);

  switch (property_id)
    {
    case PROP_ID:
      g_value_set_object (value, self->priv->id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


gboolean
biji_note_obj_are_same (BijiNoteObj *a, BijiNoteObj* b)
{
  return biji_note_id_equal (a->priv->id, b->priv->id);
}


/* First cancel timeout
 * this func is most probably stupid it might exists (move file) */
gboolean
biji_note_obj_trash (BijiItem *item)
{
  BijiNoteObj *note_to_kill;
  BijiNoteObjPrivate *priv;
  GFile *icon;
  gchar *icon_path;
  gboolean result;

  note_to_kill = BIJI_NOTE_OBJ (item);
  priv = note_to_kill->priv;
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
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), NULL);

  BijiNoteObj *note = BIJI_NOTE_OBJ (item);

  return biji_note_id_get_path (note->priv->id);
}


BijiNoteID *
note_get_id(BijiNoteObj* n)
{
  return n->priv->id;
}


static const gchar *
biji_note_obj_get_title (BijiItem *note)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (note), NULL);

  return biji_note_id_get_title (BIJI_NOTE_OBJ (note)->priv->id);
}

/* If already a title, then note is renamed */
gboolean
biji_note_obj_set_title (BijiNoteObj *note, const gchar *proposed_title)
{
  gchar *old_title, *title;
  gboolean retval;

  title = NULL;
  old_title = g_strdup (biji_note_id_get_title (note->priv->id));


  if (g_strcmp0 (proposed_title, old_title) == 0)
  {
    retval = FALSE;
    goto out;
  }


  title = biji_manager_get_unique_title (
              biji_item_get_manager (BIJI_ITEM (note)), proposed_title);
  biji_note_id_set_last_metadata_change_date (note->priv->id,
                                              g_get_real_time () / G_USEC_PER_SEC);


  /* Emit signal even if initial title, just to let know */
  biji_note_id_set_title (note->priv->id, title);
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
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (n), FALSE);

  return biji_note_id_set_mtime (n->priv->id, mtime);
}


static gint64
biji_note_obj_get_mtime (BijiItem *note)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (note), 0);

  return biji_note_id_get_mtime (BIJI_NOTE_OBJ (note)->priv->id);
}

gchar *
biji_note_obj_get_last_change_date_string (BijiNoteObj *self)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (self), "");

  return biji_get_time_diff_with_time (
             biji_note_id_get_mtime (self->priv->id));
}


gint64
biji_note_obj_get_last_metadata_change_date (BijiNoteObj *note)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (note), 0);

  return biji_note_id_get_last_metadata_change_date (note->priv->id);
}


gboolean
biji_note_obj_set_last_metadata_change_date (BijiNoteObj* n, gint64 time)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ(n), FALSE);

  return biji_note_id_set_last_metadata_change_date (n->priv->id, time);
}

gboolean
biji_note_obj_set_note_create_date (BijiNoteObj* n, gint64 time)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ(n), FALSE);

  return biji_note_id_set_create_date (n->priv->id, time);
}

static void
biji_note_obj_clear_icons (BijiNoteObj *note)
{
  g_clear_pointer (&note->priv->icon, cairo_surface_destroy);
  g_clear_pointer (&note->priv->emblem, cairo_surface_destroy);
  g_clear_pointer (&note->priv->pristine, cairo_surface_destroy);
}

static void
biji_note_obj_set_rgba_internal (BijiNoteObj *n, GdkRGBA *rgba)
{
  n->priv->color = gdk_rgba_copy(rgba);

  g_signal_emit (G_OBJECT (n), biji_obj_signals[NOTE_COLOR_CHANGED],0);
}

void
biji_note_obj_set_rgba (BijiNoteObj *n, GdkRGBA *rgba)
{
  if (!n->priv->color)
    biji_note_obj_set_rgba_internal (n, rgba);

  else if (!gdk_rgba_equal (n->priv->color,rgba))
  {
    gdk_rgba_free (n->priv->color);
    biji_note_obj_clear_icons (n);
    biji_note_obj_set_rgba_internal (n, rgba);

    biji_note_id_set_last_metadata_change_date (n->priv->id, g_get_real_time () / G_USEC_PER_SEC);
    biji_note_obj_save_note (n);
  }
}

gboolean
biji_note_obj_get_rgba(BijiNoteObj *n,
                       GdkRGBA *rgba)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (n), FALSE);

  if (n->priv->color && rgba)
    {
      *rgba = *(n->priv->color);
      return TRUE;
    }

  return FALSE;
}


const gchar *
biji_note_obj_get_raw_text                  (BijiNoteObj *note)
{
  return biji_note_id_get_content (note->priv->id);
}

void
biji_note_obj_set_raw_text (BijiNoteObj *note, gchar *plain_text)
{
  if (biji_note_id_set_content (note->priv->id, plain_text))
  {
    biji_note_obj_clear_icons (note);
    g_signal_emit (note, biji_obj_signals[NOTE_CHANGED],0);
  }
}

GList *
biji_note_obj_get_notebooks (BijiNoteObj *n)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (n), NULL);

  return g_hash_table_get_values (n->priv->labels);
}

gboolean
biji_note_obj_has_notebook (BijiItem *item, gchar *label)
{
  BijiNoteObj *note = BIJI_NOTE_OBJ (item);

  if (g_hash_table_lookup (note->priv->labels, label))
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


/*static */ gboolean
biji_note_obj_add_notebook (BijiItem *item,
                              BijiItem *notebook,
                              gchar    *title)
{
  BijiNoteObj *note;
  gchar *label = title;

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), FALSE);
  note = BIJI_NOTE_OBJ (item);

  if (BIJI_IS_NOTEBOOK (notebook))
    label = (gchar*) biji_item_get_title (notebook);

  if (biji_note_obj_has_notebook (item, label))
    return FALSE;

  g_hash_table_add (note->priv->labels, g_strdup (label));

  if (BIJI_IS_NOTEBOOK (notebook))
  {
    biji_push_existing_notebook_to_note (
      note, label, _biji_notebook_refresh, notebook); // Tracker
    biji_note_id_set_last_metadata_change_date (note->priv->id,
                                                g_get_real_time () / G_USEC_PER_SEC);
    biji_note_obj_save_note (note);
  }

  return TRUE;
}


gboolean
biji_note_obj_remove_notebook (BijiItem *item, BijiItem *notebook)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), FALSE);
  g_return_val_if_fail (BIJI_IS_NOTEBOOK (notebook), FALSE);

  BijiNoteObj *note = BIJI_NOTE_OBJ (item);

  if (g_hash_table_remove (note->priv->labels, biji_item_get_title (notebook)))
  {
    biji_remove_notebook_from_note (
      note, notebook, _biji_notebook_refresh, notebook); // tracker.
    biji_note_id_set_last_metadata_change_date (note->priv->id,
                                                g_get_real_time () / G_USEC_PER_SEC);
    biji_note_obj_save_note (note);
    return TRUE;
  }

  return FALSE;
}

gboolean
biji_note_obj_has_tag_prefix (BijiNoteObj *note, gchar *label)
{
  gboolean retval = FALSE;
  GList *tags, *l;

  tags = g_hash_table_get_keys (note->priv->labels);

  for (l = tags; l != NULL; l=l->next)
  {
    if (g_str_has_prefix (l->data, label))
    {
      retval = TRUE;
      break;
    }
  }

  g_list_free (tags);
  return retval;
}

gboolean
note_obj_is_template (BijiNoteObj *n)
{
  g_return_val_if_fail(BIJI_IS_NOTE_OBJ(n),FALSE);
  return n->priv->is_template;
}

void
note_obj_set_is_template (BijiNoteObj *n,gboolean is_template)
{
  n->priv->is_template = is_template ;
}

void
biji_note_obj_save_note (BijiNoteObj *self)
{
  self->priv->needs_save = TRUE;
  biji_timeout_reset (self->priv->timeout, 3000);
}

gchar *
biji_note_obj_get_icon_file (BijiNoteObj *note)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (note), NULL);

  const gchar *uuid;
  gchar *basename, *filename;

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

  if (note->priv->icon)
    return note->priv->icon;

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
  text = biji_note_id_get_content (note->priv->id);
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

  note->priv->icon = gd_embed_surface_in_frame (surface, "resource:///org/gnome/bijiben/thumbnail-frame.png",
                                                &frame_slice, &frame_slice);
  cairo_surface_destroy (surface);

  return note->priv->icon;
}

static cairo_surface_t *
biji_note_obj_get_pristine (BijiItem *item,
                            gint scale)
{
  GdkRGBA                note_color;
  cairo_t               *cr;
  cairo_surface_t       *surface = NULL;
  BijiNoteObj           *note = BIJI_NOTE_OBJ (item);

  if (note->priv->pristine)
    return note->priv->pristine;

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

  note->priv->pristine = surface;
  return note->priv->pristine;
}

static cairo_surface_t *
biji_note_obj_get_emblem (BijiItem *item,
                          gint scale)
{
  GdkRGBA                note_color;
  cairo_t               *cr;
  cairo_surface_t       *surface = NULL;
  BijiNoteObj           *note = BIJI_NOTE_OBJ (item);

  if (note->priv->emblem)
    return note->priv->emblem;

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

  note->priv->emblem = surface;
  return note->priv->emblem;
}

/* Single Note */

void
biji_note_obj_set_all_dates_now             (BijiNoteObj *note)
{
  gint64 time;
  BijiNoteID *id;

  g_return_if_fail (BIJI_IS_NOTE_OBJ (note));

  id = note->priv->id;
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
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (note), 0);

  return biji_note_id_get_create_date (note->priv->id);
}


gboolean
biji_note_obj_set_create_date (BijiNoteObj *note, gint64 time)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (note), FALSE);

  return biji_note_id_set_create_date (note->priv->id, time);
}


/* Webkit */

gchar *
html_from_plain_text                        (gchar *content)
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
                        "<body contenteditable='true' id='editable'>",
                        "<script type='text/javascript'>",
                        "    window.onload = function () {",
                        "      document.getElementById('editable').focus();",
                        "    };",
                        "</script>",
                        escaped,
                        "</body></html>", NULL);

  g_free (escaped);
  return retval;
}


gchar *
biji_note_obj_get_html (BijiNoteObj *note)
{
  return BIJI_NOTE_OBJ_GET_CLASS (note)->get_html (note);
}

void
biji_note_obj_set_html (BijiNoteObj *note,
                        gchar *html)
{
  BIJI_NOTE_OBJ_GET_CLASS (note)->set_html (note, html);
}

gboolean
biji_note_obj_is_opened (BijiNoteObj *note)
{
  return BIJI_IS_WEBKIT_EDITOR (note->priv->editor);
}

static void
on_biji_note_obj_closed_cb (BijiNoteObj *note)
{
  BijiNoteObjPrivate *priv;
  BijiItem *item;
  const gchar *title;

  priv = note->priv;
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
  note->priv->editor = biji_webkit_editor_new (note);

  g_signal_connect_swapped (note->priv->editor, "destroy",
                            G_CALLBACK (on_biji_note_obj_closed_cb), note);

#ifdef BUILD_ZEITGEIST
  insert_zeitgeist (note, ZEITGEIST_ZG_ACCESS_EVENT);
#endif /* BUILD_ZEITGEIST */

  return GTK_WIDGET (note->priv->editor);
}

GtkWidget *
biji_note_obj_get_editor (BijiNoteObj *note)
{
  if (!biji_note_obj_is_opened (note))
    return NULL;

  return GTK_WIDGET (note->priv->editor);
}


gboolean
biji_note_obj_can_format       (BijiNoteObj *note)
{
  return BIJI_NOTE_OBJ_GET_CLASS (note)->can_format (note);
}


void
biji_note_obj_editor_apply_format (BijiNoteObj *note, gint format)
{
  if (biji_note_obj_is_opened (note))
    biji_webkit_editor_apply_format ( note->priv->editor , format);
}

gboolean
biji_note_obj_editor_has_selection (BijiNoteObj *note)
{
  if (biji_note_obj_is_opened (note))
    return biji_webkit_editor_has_selection (note->priv->editor);

  return FALSE;
}

gchar *
biji_note_obj_editor_get_selection (BijiNoteObj *note)
{
  if (biji_note_obj_is_opened (note))
    return biji_webkit_editor_get_selection (note->priv->editor);

  return NULL;
}

void biji_note_obj_editor_cut (BijiNoteObj *note)
{
  if (biji_note_obj_is_opened (note))
    biji_webkit_editor_cut (note->priv->editor);
}

void biji_note_obj_editor_copy (BijiNoteObj *note)
{
  if (biji_note_obj_is_opened (note))
    biji_webkit_editor_copy (note->priv->editor);
}

void biji_note_obj_editor_paste (BijiNoteObj *note)
{
  if (biji_note_obj_is_opened (note))
    biji_webkit_editor_paste (note->priv->editor);
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


  g_type_class_add_private (klass, sizeof (BijiNoteObjPrivate));

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

