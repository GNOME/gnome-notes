/*
 * Bijiben
 * Copyright (C) Pierre-Yves Luyten 2012 <py@luyten.fr>
 *
 * Bijiben is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * WebkitWebView is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <string.h>

#include "biji-lazy-serializer.h"
#include "../biji-item.h"
#include "../biji-note-obj.h"
#include "../biji-string.h"

enum
{
  PROP_0,
  PROP_NOTE,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

struct _BijiLazySerializer
{
  GObject parent_instance;

  BijiNoteObj *note;

  xmlBufferPtr     buf;
  xmlTextWriterPtr writer;

  /* To get across the html tree */
  xmlTextReaderPtr inner;
};

G_DEFINE_TYPE (BijiLazySerializer, biji_lazy_serializer, G_TYPE_OBJECT)

static void
biji_lazy_serializer_get_property (GObject  *object,
                                   guint     property_id,
                                   GValue   *value,
                                   GParamSpec *pspec)
{
  BijiLazySerializer *self = BIJI_LAZY_SERIALIZER (object);

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
biji_lazy_serializer_set_property (GObject  *object,
                                   guint     property_id,
                                   const GValue *value,
                                   GParamSpec *pspec)
{
  BijiLazySerializer *self = BIJI_LAZY_SERIALIZER (object);

  switch (property_id)
  {
    case PROP_NOTE:
      self->note = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
  }
}

static void
biji_lazy_serializer_init (BijiLazySerializer *self)
{
  self->buf = xmlBufferCreate();
}

static void
biji_lazy_serializer_finalize (GObject *object)
{
  BijiLazySerializer *self = BIJI_LAZY_SERIALIZER (object);

  xmlBufferFree (self->buf);
  xmlFreeTextReader (self->inner);

  G_OBJECT_CLASS (biji_lazy_serializer_parent_class)->finalize (object);
}

static void
biji_lazy_serializer_class_init (BijiLazySerializerClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = biji_lazy_serializer_get_property;
  object_class->set_property = biji_lazy_serializer_set_property;
  object_class->finalize = biji_lazy_serializer_finalize;

  properties[PROP_NOTE] = g_param_spec_object ("note",
                                               "Note",
                                               "Biji Note Obj",
                                               BIJI_TYPE_NOTE_OBJ,
                                               G_PARAM_READWRITE  |
                                               G_PARAM_CONSTRUCT |
                                               G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class,PROP_NOTE,properties[PROP_NOTE]);
}

static void
serialize_node (xmlTextWriterPtr writer,
                const gchar *node,
                const gchar *value)
{
  xmlTextWriterWriteRaw     (writer, BAD_CAST "\n  ");
  xmlTextWriterStartElement (writer, BAD_CAST node);
  xmlTextWriterWriteString  (writer,BAD_CAST value);
  xmlTextWriterEndElement   (writer);
}

static void
serialize_tags (gchar *tag, xmlTextWriterPtr writer)
{
  gchar *string;

  string = g_strdup_printf("system:notebook:%s", tag);
  serialize_node (writer, "tag", string);

  g_free (string);
}

static void
serialize_html (BijiLazySerializer *self)
{
  gchar *html = biji_note_obj_get_html (self->note);

  if (!html)
    return;

  xmlTextWriterWriteRaw(self->writer, BAD_CAST html);
  g_free (html);
}

static gboolean
biji_lazy_serialize_internal (BijiLazySerializer *self)
{
  GList                     *tags;
  GdkRGBA                    color;
  gchar                     *color_str;
  gboolean                   retval;
  const gchar               *path;
  g_autoptr(GDateTime)       change_date = NULL;
  g_autoptr(GDateTime)       metadata_date = NULL;
  g_autoptr(GDateTime)       create_date = NULL;
  g_autofree char           *change_date_str = NULL;
  g_autofree char           *metadata_date_str = NULL;
  g_autofree char           *create_date_str = NULL;

  self->writer = xmlNewTextWriterMemory(self->buf, 0);

  // Header
  xmlTextWriterStartDocument (self->writer,"1.0","utf-8",NULL);

  xmlTextWriterStartElement (self->writer, BAD_CAST "note");
  xmlTextWriterWriteAttributeNS (self->writer, NULL,
                                 BAD_CAST "version",NULL,
                                 BAD_CAST "1");
  xmlTextWriterWriteAttributeNS (self->writer, BAD_CAST "xmlns",
                                 BAD_CAST "link", NULL,
                                 BAD_CAST "http://projects.gnome.org/bijiben/link");
  xmlTextWriterWriteAttributeNS (self->writer, BAD_CAST "xmlns", BAD_CAST "size", NULL,
                                 BAD_CAST "http://projects.gnome.org/bijiben/size");
  xmlTextWriterWriteAttributeNS (self->writer, NULL, BAD_CAST "xmlns", NULL,
                                 BAD_CAST "http://projects.gnome.org/bijiben");

  // <Title>
  serialize_node (self->writer,
                  "title",
                  (gchar*) biji_item_get_title (BIJI_ITEM (self->note)));

  // <text>
  xmlTextWriterWriteRaw(self->writer, BAD_CAST "\n  ");
  xmlTextWriterStartElement(self->writer, BAD_CAST "text");
  xmlTextWriterWriteAttributeNS(self->writer, BAD_CAST "xml",
                                BAD_CAST "space", NULL,
                                BAD_CAST "preserve");
  serialize_html (self);
  // </text>
  xmlTextWriterEndElement(self->writer);

  // <last-change-date>
  change_date = g_date_time_new_from_unix_utc (biji_item_get_mtime (BIJI_ITEM (self->note)));
  change_date_str = g_date_time_format_iso8601 (change_date);
  if (change_date_str)
  {
    serialize_node (self->writer, "last-change-date", change_date_str);
  }

  metadata_date = g_date_time_new_from_unix_utc (biji_note_obj_get_last_metadata_change_date (self->note));
  metadata_date_str = g_date_time_format_iso8601 (metadata_date);
  if (metadata_date_str)
  {
    serialize_node (self->writer, "last-metadata-change-date", metadata_date_str);
  }

  create_date = g_date_time_new_from_unix_utc (biji_note_obj_get_create_date (self->note));
  create_date_str = g_date_time_format_iso8601 (create_date);
  if (create_date_str)
  {
    serialize_node (self->writer, "create-date", create_date_str);
  }

  serialize_node (self->writer, "cursor-position", "0");
  serialize_node (self->writer, "selection-bound-position", "0");
  serialize_node (self->writer, "width", "0");
  serialize_node (self->writer, "height", "0");
  serialize_node (self->writer, "x", "0");
  serialize_node (self->writer, "y", "0");

  if (biji_note_obj_get_rgba (self->note, &color))
  {
    color_str = gdk_rgba_to_string (&color);
    serialize_node (self->writer, "color", color_str);
    g_free (color_str);
  }

  //<tags>
  xmlTextWriterWriteRaw (self->writer, BAD_CAST "\n ");
  xmlTextWriterStartElement (self->writer, BAD_CAST "tags");
  tags = biji_note_obj_get_notebooks (self->note);
  g_list_foreach (tags, (GFunc) serialize_tags, self->writer);
  xmlTextWriterEndElement (self->writer);
  g_list_free (tags);

  // <open-on-startup>
  serialize_node (self->writer, "open-on-startup", "False");

  // <note>
  xmlTextWriterWriteRaw (self->writer, BAD_CAST "\n ");
  xmlTextWriterEndElement (self->writer);

  xmlFreeTextWriter(self->writer);

  path = biji_item_get_uuid (BIJI_ITEM (self->note));
  retval = g_file_set_contents (path, (gchar*) self->buf->content, -1, NULL);

  return retval;
}

gboolean
biji_lazy_serialize (BijiNoteObj *note)
{
  BijiLazySerializer *self;
  gboolean result ;

  self = g_object_new (BIJI_TYPE_LAZY_SERIALIZER,
                       "note", note, NULL);
  result = biji_lazy_serialize_internal (self);
  g_object_unref (self);

  return result;
}

