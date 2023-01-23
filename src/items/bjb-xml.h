/* bjb-macro.h
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

#pragma once

#include <glib-object.h>
#include <libxml/xmlreader.h>
#include <libxml/xmlwriter.h>

G_BEGIN_DECLS

static inline xmlDoc *
xml_doc_new (const char *data,
             int        size)
{
  return xmlReadMemory (data, size , "", "utf-8",
                        XML_PARSE_RECOVER | XML_PARSE_NONET);
}

static inline void
xml_doc_free (xmlDoc *doc)
{
  xmlFreeDoc (doc);
}

static inline xmlNode *
xml_doc_get_root_element (xmlDoc *doc)
{
  return xmlDocGetRootElement (doc);
}

static inline xmlTextReader *
xml_reader_new (const char *data,
                int         size)
{
  return xmlReaderForMemory (data, size, "", "utf-8",
                             XML_PARSE_RECOVER | XML_PARSE_NONET);
}

static inline void
xml_reader_free (xmlTextReader *reader)
{
  xmlFreeTextReader (reader);
}

static inline int
xml_reader_read (xmlTextReader *reader)
{
  return xmlTextReaderRead (reader);
}

static inline xmlDoc *
xml_reader_get_doc (xmlTextReader *reader)
{
  return xmlTextReaderCurrentDoc (reader);
}

static inline const char *
xml_reader_get_name (xmlTextReader *reader)
{
  return (const char *)xmlTextReaderConstName (reader);
}

static inline const char *
xml_reader_get_value (xmlTextReader *reader)
{
  return (const char *)xmlTextReaderConstValue (reader);
}

static inline int
xml_reader_get_node_type (xmlTextReader *reader)
{
  return xmlTextReaderNodeType (reader);
}

static inline char *
xml_reader_dup_inner_xml (xmlTextReader *reader)
{
  return (char *)xmlTextReaderReadInnerXml (reader);
}

static inline char *
xml_reader_dup_string (xmlTextReader *reader)
{
  return (char *)xmlTextReaderReadString (reader);
}

static inline xmlBuffer *
xml_buffer_new (void)
{
  return xmlBufferCreate ();
}

static inline xmlTextWriter *
xml_writer_new (xmlBuffer *buffer)
{
  return xmlNewTextWriterMemory (buffer, 0);
}

static inline int
xml_writer_start_doc (xmlTextWriter *writer)
{
  return xmlTextWriterStartDocument (writer, "1.0", "utf-8", NULL);
}

static inline int
xml_writer_start_tag (xmlTextWriter *writer,
                      const char    *tag)
{
  return xmlTextWriterStartElement (writer, (const xmlChar *)tag);
}

static inline int
xml_writer_end_tag (xmlTextWriter *writer)
{
  return xmlTextWriterEndElement (writer);
}

static inline int
xml_writer_write_raw (xmlTextWriter *writer,
                      const char    *str)
{
  return xmlTextWriterWriteRaw (writer, (const xmlChar *)str);
}

static inline int
xml_writer_write_string (xmlTextWriter *writer,
                         const char    *str)
{
  return xmlTextWriterWriteString (writer, (const xmlChar *)str);
}

static inline void
xml_writer_add_tag (xmlTextWriter *writer,
                    const char    *tag_name,
                    const char    *content)
{
  xml_writer_start_tag (writer, tag_name);
  xml_writer_write_string (writer, content);
  xml_writer_end_tag (writer);
}

static inline int
xml_writer_write_attribute (xmlTextWriter *writer,
                            const char    *prefix,
                            const char    *name,
                            const char    *content)
{
  return xmlTextWriterWriteAttributeNS (writer, (const xmlChar *)prefix,
                                        (const xmlChar *)name, NULL,
                                        (const xmlChar *)content);
}

static inline int
xml_writer_end_doc (xmlTextWriter *writer)
{
  return xmlTextWriterEndDocument (writer);
}

G_END_DECLS
