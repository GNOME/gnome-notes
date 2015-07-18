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


struct _BijiNoteIDPrivate
{
  /* InfoSet */

  const gchar  *path;
  gchar        *title;
  gchar        *content;
  gint64        mtime;
  gint64        create_date ;


  /* Not sure anymore */

  gchar        *basename;
  gint64        last_metadata_change_date;

};

#define NOTE_OBJ_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), NOTE_TYPE_OBJ, NoteObjPrivate))


G_DEFINE_TYPE (BijiNoteID, biji_note_id, G_TYPE_OBJECT);


static void
biji_note_id_init (BijiNoteID *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, biji_note_id_get_type(),
                                            BijiNoteIDPrivate);

  self->priv->title = NULL;
  self->priv->content = NULL;
  self->priv->create_date = 0;
  self->priv->basename = 0;
  self->priv->last_metadata_change_date = 0;

}

static void
biji_note_id_finalize (GObject *object)
{
  BijiNoteID *id = BIJI_NOTE_ID (object);
  BijiNoteIDPrivate *priv = id->priv;

  g_free (priv->title);

  G_OBJECT_CLASS (biji_note_id_parent_class)->finalize (object);
}

static void
biji_note_id_set_path (BijiNoteID *self, const gchar *path)
{
  g_return_if_fail (BIJI_IS_NOTE_ID (self));

  self->priv->path = g_strdup (path);
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
      self->priv->mtime = g_value_get_int64 (value);
      break;
    case PROP_CONTENT:
      self->priv->content = g_strdup (g_value_get_string (value));
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
      g_value_set_object (value, self->priv->basename);
      break;
    case PROP_TITLE:
      g_value_set_string (value, self->priv->title);
      break;
    case PROP_MTIME:
      g_value_set_int64 (value, self->priv->mtime);
      break;
    case PROP_CONTENT:
      g_value_set_string (value, self->priv->content);
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

  g_type_class_add_private (klass, sizeof (BijiNoteIDPrivate));
}

gboolean
biji_note_id_equal (BijiNoteID *a, BijiNoteID *b)
{
  if (g_strcmp0 (a->priv->path, b->priv->path) == 0 &&
      g_strcmp0 (a->priv->content ,b->priv->content) ==0)
    return TRUE;

  return FALSE;
}

const gchar *
biji_note_id_get_path (BijiNoteID* n)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (n), NULL);

  return n->priv->path;
}


void
biji_note_id_set_title  (BijiNoteID *n, gchar* title)
{
  if (n->priv->title)
    g_free (n->priv->title);

  n->priv->title = g_strdup (title);
  g_object_notify_by_pspec (G_OBJECT (n), properties[PROP_TITLE]);
}


const gchar *
biji_note_id_get_title (BijiNoteID* n)
{
  return n->priv->title ;
}


gboolean
biji_note_id_set_content (BijiNoteID *id, gchar *content)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (id), FALSE);
  g_return_val_if_fail ((content != NULL), FALSE);

  if (id->priv->content != NULL &&
      g_strcmp0 (id->priv->content, content) !=0)
    g_clear_pointer (&id->priv->content, g_free);


  if (id->priv->content == NULL)
  {
    id->priv->content = g_strdup (content);
    return TRUE;
  }

  return FALSE;
}


const gchar *
biji_note_id_get_content (BijiNoteID *id)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (id), NULL);

  return id->priv->content;
}



gint64
biji_note_id_get_mtime (BijiNoteID *n)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (n), 0);

  return n->priv->mtime;
}


gboolean
biji_note_id_set_mtime (BijiNoteID *n, gint64 time)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (n), FALSE);

  if (n->priv->mtime != time)
  {
    n->priv->mtime = time;
    return TRUE;
  }

  return FALSE;
}


gint64
biji_note_id_get_last_metadata_change_date (BijiNoteID* n)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (n), 0);

  return n->priv->last_metadata_change_date;
}


gboolean
biji_note_id_set_last_metadata_change_date (BijiNoteID* n, gint64 time)
{
  if (n->priv->last_metadata_change_date != time)
  {
    n->priv->last_metadata_change_date = time;
    return TRUE;
  }

  return FALSE;
}


gint64
biji_note_id_get_create_date (BijiNoteID* n)
{
  g_return_val_if_fail (BIJI_IS_NOTE_ID (n), 0);

  return n->priv->create_date;
}


gboolean
biji_note_id_set_create_date (BijiNoteID* n, gint64 time)
{
  if (n->priv->create_date != time)
  {
    n->priv->create_date = time;
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
