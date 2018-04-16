/* bjb-note-id.c
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

#include "biji-note-id.h"

/* Properties */
enum {
  PROP_0,
  PROP_PATH,
  PROP_TITLE,
  PROP_MTIME,
  PROP_CONTENT,
  BIJI_ID_PROPERTIES
};

static GParamSpec *properties[BIJI_ID_PROPERTIES] = { NULL, };


struct _BijiNoteID
{
  GObject       parent_instance;
  /* InfoSet */

  gchar        *path;
  gchar        *title;
  gchar        *content;
  gint64        mtime;
  gint64        create_date ;


  /* Not sure anymore */

  gchar        *basename;
  gint64        last_metadata_change_date;

};


G_DEFINE_TYPE (BijiNoteID, biji_note_id, G_TYPE_OBJECT)


static void
biji_note_id_init (BijiNoteID *self)
{
}

static void
biji_note_id_finalize (GObject *object)
{
  BijiNoteID *self = BIJI_NOTE_ID (object);

  g_free (self->path);
  g_free (self->title);
  g_free (self->content);

  G_OBJECT_CLASS (biji_note_id_parent_class)->finalize (object);
}

static void
biji_note_id_set_path (BijiNoteID *self, const gchar *path)
{
  g_return_if_fail (BIJI_IS_NOTE_ID (self));

  self->path = g_strdup (path);
}


static void
biji_note_id_set_property  (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BijiNoteID *self = BIJI_NOTE_ID (object);


  switch (property_id)
    {
    case PROP_PATH:
      biji_note_id_set_path (self, g_value_get_string (value));
      break;
    case PROP_TITLE:
      biji_note_id_set_title (self, (gchar*) g_value_get_string (value));
      g_object_notify_by_pspec (object, properties[PROP_TITLE]);
      break;
    case PROP_MTIME:
      self->mtime = g_value_get_int64 (value);
      break;
    case PROP_CONTENT:
      biji_note_id_set_content (self, g_value_get_string (value));
      g_object_notify_by_pspec (object, properties[PROP_CONTENT]);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_note_id_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BijiNoteID *self = BIJI_NOTE_ID (object);

  switch (property_id)
    {
    case PROP_PATH:
      g_value_set_object (value, self->basename);
      break;
    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;
    case PROP_MTIME:
      g_value_set_int64 (value, self->mtime);
      break;
    case PROP_CONTENT:
      g_value_set_string (value, self->content);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_note_id_class_init (BijiNoteIDClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = biji_note_id_finalize;
  object_class->get_property = biji_note_id_get_property;
  object_class->set_property = biji_note_id_set_property;

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

  g_object_class_install_properties (object_class, BIJI_ID_PROPERTIES, properties);
}

gboolean
biji_note_id_equal (BijiNoteID *a, BijiNoteID *b)
{
  if (g_strcmp0 (a->path, b->path) == 0 &&
      g_strcmp0 (a->content, b->content) == 0)
    return TRUE;

  return FALSE;
}

const gchar *
biji_note_id_get_path (BijiNoteID* n)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (n), NULL);

  return n->path;
}


void
biji_note_id_set_title  (BijiNoteID *n, gchar* title)
{
  if (n->title)
    g_free (n->title);

  n->title = g_strdup (title);
  g_object_notify_by_pspec (G_OBJECT (n), properties[PROP_TITLE]);
}


const gchar *
biji_note_id_get_title (BijiNoteID* n)
{
  return n->title;
}


gboolean
biji_note_id_set_content (BijiNoteID *id,
                          const gchar *content)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (id), FALSE);
  g_return_val_if_fail ((content != NULL), FALSE);

  if (id->content != NULL &&
      g_strcmp0 (id->content, content) != 0)
    g_clear_pointer (&id->content, g_free);


  if (id->content == NULL)
  {
    id->content = g_strdup (content);
    return TRUE;
  }

  return FALSE;
}


const gchar *
biji_note_id_get_content (BijiNoteID *id)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (id), NULL);

  return id->content;
}



gint64
biji_note_id_get_mtime (BijiNoteID *n)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (n), 0);

  return n->mtime;
}


gboolean
biji_note_id_set_mtime (BijiNoteID *n, gint64 time)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (n), FALSE);

  if (n->mtime != time)
  {
    n->mtime = time;
    return TRUE;
  }

  return FALSE;
}


gint64
biji_note_id_get_last_metadata_change_date (BijiNoteID* n)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (n), 0);

  return n->last_metadata_change_date;
}


gboolean
biji_note_id_set_last_metadata_change_date (BijiNoteID* n, gint64 time)
{
  if (n->last_metadata_change_date != time)
  {
    n->last_metadata_change_date = time;
    return TRUE;
  }

  return FALSE;
}


gint64
biji_note_id_get_create_date (BijiNoteID* n)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (n), 0);

  return n->create_date;
}


gboolean
biji_note_id_set_create_date (BijiNoteID* n, gint64 time)
{
  if (n->create_date != time)
  {
    n->create_date = time;
    return TRUE;
  }

  return FALSE;
}


BijiNoteID *
biji_note_id_new_from_info     (BijiInfoSet *info)
{
  return g_object_new (BIJI_TYPE_NOTE_ID,
                       "path",  info->url,
                       "title", info->title,
                       "mtime", info->mtime,
                       "content", info->content,
                       NULL);
}
