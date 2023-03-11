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

#include <glib/gprintf.h>
#include <gtk/gtk.h>
#include <libxml/xmlreader.h>

#include <string.h>

#include "biji-lazy-deserializer.h"
#include "../biji-date-time.h"
#include "../biji-string.h"

/* Notes type : bijiben namespace, tomboy one, then note types */
#define BIJI_NS "http://projects.gnome.org/bijiben"

#define TOMBOY_NS "http://beatniksoftware.com/tomboy"

typedef enum
{
  NO_TYPE,
  BIJIBEN_1,
  TOMBOY_1,
  TOMBOY_2,
  TOMBOY_3,
  NUM_NOTE_TYPES
} BijiXmlType;

struct _BijiLazyDeserializer
{
  GObject parent_instance;

  /* Note, type, first reader for metadata */
  BjbNote    *note;
  BijiXmlType type;
  xmlTextReaderPtr r;

  /* Reader for internal content, either tomboy html or Bijiben xhtml */
  xmlTextReaderPtr inner;
  gchar *content;

  /* Result for both raw_text & html */
  GString *raw_text;
  GString *html;
  gboolean seen_content;
};


G_DEFINE_TYPE (BijiLazyDeserializer, biji_lazy_deserializer, G_TYPE_OBJECT)

static void
biji_lazy_deserializer_init (BijiLazyDeserializer *self)
{
  self->raw_text = g_string_new ("");
  self->html = g_string_new ("");
}

static void
biji_lazy_deserializer_finalize (GObject *object)
{
  BijiLazyDeserializer *self= BIJI_LAZY_DESERIALIZER (object);

  g_string_free (self->raw_text, TRUE);
  g_string_free (self->html, TRUE);
  g_free (self->content);

  xmlFreeTextReader (self->r);
  xmlFreeTextReader (self->inner);

  G_OBJECT_CLASS (biji_lazy_deserializer_parent_class)->finalize (object);
}

static void
biji_lazy_deserializer_class_init (BijiLazyDeserializerClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = biji_lazy_deserializer_finalize;
}

/* Utils */

typedef void BijiReaderFunc (BjbNote *note, char *string);


static void
biji_process_string (xmlTextReaderPtr reader,
                     BijiReaderFunc process_xml,
                     gpointer user_data)
{
  xmlChar *result = xmlTextReaderReadString (reader);
  process_xml (user_data, (gchar*) result);
  free (result);
}



/* Tomboy Inner XML */

static void
process_tomboy_start_elem (BijiLazyDeserializer *self)
{
  const gchar *element_name;

  element_name = (const gchar *) xmlTextReaderConstName(self->inner);

  if (g_strcmp0 (element_name, "note-content")==0)
    return;

  if (g_strcmp0 (element_name, "bold")==0)
    self->html = g_string_append (self->html, "<b>");

  if (g_strcmp0 (element_name, "italic")==0)
    self->html = g_string_append (self->html, "<i>");

  if (g_strcmp0 (element_name, "strikethrough")==0)
    self->html = g_string_append (self->html, "<strike>");

  /* Currently tomboy has unordered list */

  if (g_strcmp0 (element_name, "list")==0)
    self->html = g_string_append (self->html, "<ul>");

  if (g_strcmp0 (element_name, "list-item")==0)
    self->html = g_string_append (self->html, "<li>");
}

static void
process_tomboy_end_elem (BijiLazyDeserializer *self)
{
  const gchar *element_name;

  element_name = (const gchar *) xmlTextReaderConstName (self->inner);

  if (g_strcmp0 (element_name, "note-content")==0)
    return;

  if (g_strcmp0 (element_name, "bold")==0)
    self->html = g_string_append (self->html, "</b>");

  if (g_strcmp0 (element_name, "italic")==0)
    self->html = g_string_append (self->html, "</i>");

  if (g_strcmp0 (element_name, "strikethrough")==0)
    self->html = g_string_append (self->html, "</strike>");

  /* Currently tomboy has unordered list */

  if (g_strcmp0 (element_name, "list")==0)
    self->html = g_string_append (self->html, "</ul>");

  if (g_strcmp0 (element_name, "list-item")==0)
    self->html = g_string_append (self->html, "</li>");
}

static void
process_tomboy_text_elem (BijiLazyDeserializer *self)
{
  const gchar *text;

  text = (const gchar *) xmlTextReaderConstValue (self->inner);

  /* Simply append the text to both raw & html */
  self->raw_text = g_string_append (self->raw_text, text);
  self->html = g_string_append (self->html, text);
}

static void
process_tomboy_node (BijiLazyDeserializer *self)
{
  int            type;
  const xmlChar *name ;

  type  = xmlTextReaderNodeType (self->inner);
  name  = xmlTextReaderConstName (self->inner);

  if (name == NULL)
    name = BAD_CAST "(NULL)";

  switch (type)
  {
    case XML_ELEMENT_NODE:
      process_tomboy_start_elem (self);
      break;

    case XML_ELEMENT_DECL:
      process_tomboy_end_elem (self);
      break;

    case XML_TEXT_NODE:
      process_tomboy_text_elem (self);
      break;

    case XML_DTD_NODE:
      process_tomboy_text_elem (self);
      break;

    default:
      break;
  }
}

static void
process_tomboy_xml_content (BijiLazyDeserializer *self)
{
  int ret;
  gchar *revamped_html;

  g_string_append (self->html, "<html xmlns=\"http://www.w3.org/1999/xhtml\"><body>");

  self->inner = xmlReaderForMemory (self->content,
                                    strlen(self->content),
                                    "", "UTF-8", 0);

  ret = xmlTextReaderRead (self->inner);

  /* Make the GString grow as we read */
  while (ret == 1)
  {
    process_tomboy_node (self);
    ret = xmlTextReaderRead (self->inner);
  }

  g_string_append (self->html, "</body></html>");

  /* Now the inner content is known, we can
   * assign note values and let deserialization work on last elements*/
  bjb_note_set_raw_content (BJB_NOTE (self->note), self->raw_text->str);

  revamped_html = biji_str_replace (self->html->str, "\n", "<br/>");
  bjb_note_set_html (BJB_NOTE (self->note), revamped_html);
  g_free (revamped_html);
}

/* Bijiben Inner HTML */


/*
 * as of today js is injected in the middle of the body,
 * and interpreted as text.
 * we need to prevent js to be included in note's
 * raw content. This way works
 */
static gboolean is_node_script = FALSE;


static void
process_bijiben_start_elem (BijiLazyDeserializer *self)
{
  const gchar *element_name;

  element_name = (const gchar *) xmlTextReaderConstName(self->inner);

  if (g_strcmp0 (element_name, "script") == 0)
    is_node_script = TRUE;

  if (g_strcmp0 (element_name, "body") == 0)
    is_node_script = FALSE;

  /* Block level elements introduce a new line, except that blocks
     at the very beginning of their parent don't, and <br/> at the
     end of a block causes the next block to skip the new line.
     list-item elements add also a bullet at the beginning.

     These are the only block elements we produce and therefore
     support. If you manage to introduce more (eg. by copy-pasting),
     you accept that the result may not be faithful.

     TODO: use webkit_web_view_get_snapshot() instead of showing
     the raw text content in the main view.
  */
  if (g_strcmp0 (element_name, "br") == 0) {
    g_string_append (self->raw_text, "\n");
    self->seen_content = FALSE;
  }

  if (self->seen_content &&
      (g_strcmp0 (element_name, "div") == 0 ||
       g_strcmp0 (element_name, "br") == 0 ||
       g_strcmp0 (element_name, "ul") == 0 ||
       g_strcmp0 (element_name, "ol") == 0 ||
       g_strcmp0 (element_name, "li") == 0)) {
    g_string_append (self->raw_text, "\n");
    self->seen_content = FALSE;
  }

  if (g_strcmp0 (element_name, "li") == 0)
    g_string_append (self->raw_text, "- ");
}

static void
process_bijiben_text_elem (BijiLazyDeserializer *self)
{
  gchar *text;

  text = (gchar *) xmlTextReaderConstValue (self->inner);

  if (text)
  {
    self->raw_text = g_string_append (self->raw_text, text);
    self->seen_content = TRUE;
  }
}

static void
process_bijiben_node (BijiLazyDeserializer *self)
{
  int type;
  const xmlChar *name ;

  type  = xmlTextReaderNodeType (self->inner);
  name  = xmlTextReaderConstName (self->inner);

  if (name == NULL)
    name = BAD_CAST "(NULL)";

  switch (type)
    {
    case XML_READER_TYPE_ELEMENT:
      process_bijiben_start_elem (self);
      break;

    case XML_READER_TYPE_TEXT:
      process_bijiben_text_elem (self);
      break;

    default:
      /* Ignore other node types (and ignore
         gcc warnings */
      ;
  }
}

static void
process_bijiben_html_content (BijiLazyDeserializer *self,
                              xmlTextReaderPtr      reader)
{
  int ret;
  gchar *sane_html;

  sane_html = (gchar*) xmlTextReaderReadInnerXml (reader);

  self->inner = xmlReaderForMemory (sane_html,
                                    strlen(sane_html),
                                    "", "UTF-8", 0);

  ret = xmlTextReaderRead (self->inner);

  /* Make the GString grow as we read */
  while (ret == 1)
  {
    process_bijiben_node (self);
    ret = xmlTextReaderRead (self->inner);
  }

  bjb_note_set_raw_content (BJB_NOTE (self->note), self->raw_text->str);
  bjb_note_set_html (BJB_NOTE (self->note), sane_html);

  xmlFree (BAD_CAST sane_html);
}

/* Common XML format for both Bijiben / Tomboy */
static void
processNode (BijiLazyDeserializer *self)
{
  xmlTextReaderPtr r = self->r;
  BjbNote   *n = self->note;
  xmlChar   *name;
  GdkRGBA    color;
  gchar     *tag, *color_str;

  name = xmlTextReaderName (r);

  if ( g_strcmp0((gchar*)name,"title") == 0 )
    biji_process_string (r, (BijiReaderFunc*) bjb_item_set_title, n);

  if ( g_strcmp0((gchar*)name,"text") == 0 )
  {
    if (self->type == BIJIBEN_1)
    {
      process_bijiben_html_content (self, r);
    }

    else if (self->type == TOMBOY_1 ||
             self->type == TOMBOY_2 ||
             self->type == TOMBOY_3 )
    {
      self->content = (gchar*) xmlTextReaderReadInnerXml (r);
      process_tomboy_xml_content (self);
    }
  }

  if (g_strcmp0 ((gchar*) name, "last-change-date") == 0)
  {
    gchar *result = (gchar*) xmlTextReaderReadString (r);
    bjb_item_set_mtime (BJB_ITEM (n), iso8601_to_gint64 (result));
    free (result);
  }

  if (g_strcmp0 ((gchar*) name, "last-metadata-change-date") == 0)
  {
    gchar *result = (gchar*) xmlTextReaderReadString (r);
    bjb_item_set_meta_mtime (BJB_ITEM (n), iso8601_to_gint64 (result));
    free (result);
  }

  if (g_strcmp0 ((gchar*) name, "create-date") == 0)
  {
    gchar *result = (gchar*) xmlTextReaderReadString (r);
    bjb_item_set_create_time (BJB_ITEM (n), iso8601_to_gint64 (result));
    free (result);
  }

  if (g_strcmp0 ((gchar*) name, "color") == 0 )
  {
    color_str = (gchar*) xmlTextReaderReadString (r);

    if (gdk_rgba_parse (&color, color_str))
      bjb_item_set_rgba (BJB_ITEM (n), &color);

    else
      g_warning ("color invalid:%s", color_str);

    free (color_str);
  }

  if ( g_strcmp0((gchar*)name,"tag") == 0 )
  {
    tag = (gchar*) xmlTextReaderReadString(r);

    if (g_str_has_prefix (tag,"system:template"))
    {
      g_object_set_data (G_OBJECT (n), "template", GINT_TO_POINTER (TRUE));
    }

    else if (g_str_has_prefix (tag,"system:notebook:"))
    {
      bjb_note_add_tag (BJB_NOTE (n), tag + strlen ("system:notebook:"));
    }

    free (tag);
  }

  xmlFree(name);
}

static void
biji_parse_file (BijiLazyDeserializer *self)
{
  while ( xmlTextReaderRead(self->r) == 1 )
  {
    if ( xmlTextReaderNodeType(self->r) == 1 )
    {
      processNode(self);
    }
  }
}

static gboolean
biji_lazy_deserialize_internal (BijiLazyDeserializer *self)
{
  BjbNote *n = self->note;
  const gchar *path;
  xmlDocPtr doc;
  xmlNodePtr cur;
  xmlChar     *version;

  path = bjb_item_get_uid (BJB_ITEM (n));
  doc = xmlParseFile (path);

  if (doc == NULL )
  {
    g_warning ("File not parsed successfully");
    return FALSE;
  }

  cur = xmlDocGetRootElement (doc);

  if (cur == NULL)
  {
    g_warning ("File empty");
    xmlFreeDoc(doc);
    return FALSE;
  }

  if (xmlStrcmp(cur->name, (const xmlChar *) "note"))
  {
    g_message ("document of the wrong type, root node != note");
    xmlFreeDoc(doc);
    return FALSE;
  }

  /* Switch there for note type
   * Despite not yet handled */

  version = xmlGetNoNsProp (cur, BAD_CAST "version");

  /* Bijiben type */
  if (g_strcmp0 ((gchar*) cur->ns->href, BIJI_NS) ==0) {
    self->type = BIJIBEN_1;
  }

  /* Tomboy type */
  else {
    if (g_strcmp0 ((gchar*) cur->ns->href, TOMBOY_NS) == 0)
    {
      if (g_strcmp0 ((const gchar*) version, "0.1") == 0)
        self->type = TOMBOY_1;

      if (g_strcmp0 ((const gchar*) version, "0.2") == 0)
        self->type = TOMBOY_2;

      if (g_strcmp0 ((const gchar*) version, "0.3") == 0)
        self->type = TOMBOY_3;
    }

  /* Wow this note won't be loaded i guess */
    else {
      self->type = NO_TYPE;
    }
  }

  xmlFree (version);

  path = bjb_item_get_uid (BJB_ITEM (n));
  self->r = xmlNewTextReaderFilename (path);

  biji_parse_file (self);
  xmlFreeDoc (doc);

  return TRUE ;
}

static BijiLazyDeserializer *
biji_lazy_deserializer_new (BjbNote *note)
{
  BijiLazyDeserializer *self;

  g_return_val_if_fail (BJB_IS_NOTE (note), NULL);

  self = g_object_new (BIJI_TYPE_LAZY_DESERIALIZER, NULL);
  g_set_object (&self->note, note);

  return self;
}

gboolean
biji_lazy_deserialize (BjbNote *note)
{
  BijiLazyDeserializer *bld;
  gboolean result;

  bld = biji_lazy_deserializer_new (note);
  result = biji_lazy_deserialize_internal (bld);
  g_clear_object (&bld);

  return result;
}
