/* bjb-xml-note.c
 *
 * Copyright 2023 Mohammed Sadiq <sadiq@sadiqpk.org>
 * Copyright 2023 Purism SPC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s):
 *   Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "bjb-xml-note"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>

#include "deserializer/biji-lazy-deserializer.h"
#include "bjb-tag.h"
#include "bjb-xml.h"
#include "bjb-xml-note.h"

#define COMMON_XML_HEAD "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"
#define BIJIBEN_XML_NS "http://projects.gnome.org/bijiben"
#define TOMBOY_XML_NS  "http://beatniksoftware.com/tomboy"

typedef enum
{
  NOTE_FORMAT_UNKNOWN,
  NOTE_FORMAT_TOMBOY_1,
  NOTE_FORMAT_TOMBOY_2,
  NOTE_FORMAT_TOMBOY_3,
  NOTE_FORMAT_BIJIBEN_1,
  NOTE_FORMAT_BIJIBEN_2,
  N_NOTE_FORMATS
} NoteFormat;

struct _BjbXmlNote
{
  BjbNote       parent_instance;

  char        *raw_xml;     /* full XML data to be saved to file */
  char        *content_xml; /* pointer to the beginning of content */
  char        *text_content; /* pointer to the beginning of content */
  GString     *markup;
  char        *title;
  /* contains all tags known to BjbManager, only for lookup */
  BjbTagStore *tag_store;

  NoteFormat   note_format;
  guint        parse_complete : 1;
};

G_DEFINE_TYPE (BjbXmlNote, bjb_xml_note, BJB_TYPE_NOTE)

static char *
bjb_xml_note_get_raw_content (BjbNote *note)
{
  BjbXmlNote *self = BJB_XML_NOTE (note);

  g_assert (BJB_IS_NOTE (note));

  return g_strdup (self->raw_xml);
}

static void
bjb_xml_note_set_raw_content (BjbNote    *note,
                              const char *content)
{
  BjbXmlNote *self = BJB_XML_NOTE (note);

  g_assert (BJB_IS_XML_NOTE (self));

  g_free (self->raw_xml);
  self->raw_xml = g_strdup (content);
  bjb_item_set_modified (BJB_ITEM (note));
}

static void
bjb_xml_note_set_text_content (BjbNote    *note,
                               const char *content)
{
  BjbXmlNote *self = BJB_XML_NOTE (note);

  g_assert (BJB_IS_XML_NOTE (self));

  g_free (self->text_content);
  self->text_content = g_strdup (content);
}

static BjbTagStore *
bjb_xml_note_get_tag_store (BjbNote *note)
{
  BjbXmlNote *self = BJB_XML_NOTE (note);

  g_assert (BJB_IS_XML_NOTE (self));

  return self->tag_store;
}

static const char *
bjb_xml_note_get_extension (BjbNote *note)
{
  return ".note";
}

static void
bjb_xml_note_finalize (GObject *object)
{
  BjbXmlNote *self = BJB_XML_NOTE (object);

  g_clear_pointer (&self->text_content, g_free);
  g_clear_pointer (&self->title, g_free);

  G_OBJECT_CLASS (bjb_xml_note_parent_class)->finalize (object);
}

static void
bjb_xml_note_class_init (BjbXmlNoteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BjbNoteClass *note_class = BJB_NOTE_CLASS (klass);

  object_class->finalize = bjb_xml_note_finalize;

  note_class->get_raw_content = bjb_xml_note_get_raw_content;
  note_class->set_raw_content = bjb_xml_note_set_raw_content;
  note_class->set_text_content = bjb_xml_note_set_text_content;
  note_class->get_tag_store = bjb_xml_note_get_tag_store;
  note_class->get_extension = bjb_xml_note_get_extension;
}

static void
bjb_xml_note_init (BjbXmlNote *self)
{
  self->note_format = NOTE_FORMAT_BIJIBEN_2;
}

/**
 * bjb_xml_note_new_from_data:
 * @uid: A unique id string
 * @tag_store: a #BjbTagStore
 *
 * Create a new XML note from the given data.
 *
 * Returns: (transfer full): a new #BjbXmlNote
 */
BjbItem *
bjb_xml_note_new_from_data (const char  *uid,
                            BjbTagStore *tag_store)
{
  BjbXmlNote *self;

  self = g_object_new (BJB_TYPE_XML_NOTE, NULL);
  g_set_object (&self->tag_store, tag_store);
  bjb_item_set_uid (BJB_ITEM (self), uid);

  biji_lazy_deserialize (BJB_NOTE (self));

  return BJB_ITEM (self);
}
