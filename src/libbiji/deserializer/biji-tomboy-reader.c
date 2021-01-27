/*
 * biji-tomboy-reader.c
 *
 * Copyright 2013 Pierre-Yves Luyten <py@luyten.fr>
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


/* TODO escape strings to have sane html */

#include <gio/gio.h>
#include <libxml/xmlreader.h>
#include <string.h>

#include "../biji-date-time.h"
#include "../biji-info-set.h"
#include "../biji-string.h"

#include "biji-tomboy-reader.h"


#define TOMBOY_NS "http://beatniksoftware.com/tomboy"


/* gobject properties */
enum {
  PROP_0,
  PROP_SRC_PATH,
  TOMBOY_READER_PROP
};



static GParamSpec *properties[TOMBOY_READER_PROP] = { NULL, };


typedef enum
{
  NO_TYPE,
  TOMBOY_1,
  TOMBOY_2,
  TOMBOY_3,
  NUM_NOTE_TYPES
} BijiTomboyType;


struct _BijiTomboyReader
{
  GObject parent_instance;
  /* File */

  gchar *path;
  BijiTomboyType type;


  /* Read */

  xmlTextReaderPtr r;
  xmlTextReaderPtr inner;
  GString *raw_text;
  GString *html;



  /* Return */

  GError *error;
  BijiInfoSet *set;
  gint64  metadata_mtime;
  GQueue *notebooks;

};




G_DEFINE_TYPE (BijiTomboyReader, biji_tomboy_reader, G_TYPE_OBJECT)




/* Tomboy Inner XML */

static void
process_tomboy_start_elem (BijiTomboyReader *self)
{
  const gchar *element_name;

  element_name = (const gchar *) xmlTextReaderConstName (self->inner);

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
process_tomboy_end_elem (BijiTomboyReader *self)
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
process_tomboy_text_elem (BijiTomboyReader *self)
{
  gchar *text, *html_txt;

  text = (gchar*) xmlTextReaderConstValue (self->inner);

  /* Simply append the text to both raw & html
   * FIXME : escape things for html */

  self->raw_text = g_string_append (self->raw_text, text);

  html_txt  =  biji_str_mass_replace (text,
                                    "&",  "&amp;",
                                    "<" , "&lt;" ,
                                    ">",  "&gt;",
                                    NULL);

  self->html = g_string_append (self->html, html_txt);
  g_free (html_txt);
}



static void
process_tomboy_node (BijiTomboyReader *self)
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
process_tomboy_xml_content (BijiTomboyReader *self, gchar *text)
{
  gint ret;

  self->inner = xmlReaderForMemory (text,
                                    strlen (text),
                                    "", "UTF-8", 0);

  ret = xmlTextReaderRead (self->inner);

  /* Make the GString grow as we read */
  while (ret == 1)
  {
    process_tomboy_node (self);
    ret = xmlTextReaderRead (self->inner);
  }


  /* Close the html and set content */
  g_string_append (self->html, "</body></html>");
  self->set->content = g_strdup (self->raw_text->str);
}






static void
processNode (BijiTomboyReader *self)
{
  xmlTextReaderPtr r;
  gchar *name, *result;
  gchar     *tag;
  GString   *norm;

  r = self->r;
  name = (gchar*) xmlTextReaderName (r);


  if (g_strcmp0 (name, "title") == 0)
    self->set->title = (gchar*) xmlTextReaderReadString (r);


  if (g_strcmp0(name, "text") == 0)
  {
    process_tomboy_xml_content (self, (gchar*) xmlTextReaderReadInnerXml (r));
  }


  if (g_strcmp0 (name, "last-change-date") == 0)
  {
    result = (gchar*) xmlTextReaderReadString (r);
    self->set->mtime = iso8601_to_gint64 (result);
    g_free (result);
  }


  if (g_strcmp0 (name, "last-metadata-change-date") == 0)
  {
    result = (gchar*) xmlTextReaderReadString (r);
    self->metadata_mtime = iso8601_to_gint64 (result);
    g_free (result);
  }


  if (g_strcmp0 (name, "create-date") == 0)
  {
    result = (gchar*) xmlTextReaderReadString (r);
    self->set->created  =  iso8601_to_gint64 (result);
    g_free (result);
  }



  if (g_strcmp0 (name,"tag") == 0 )
  {
    tag = (gchar*) xmlTextReaderReadString(r);

    if (g_str_has_prefix (tag,"system:template"))
    {
      self->error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                                 "Aborting import for template note");
    }

    else if (g_str_has_prefix (tag,"system:notebook:"))
    {
      norm = g_string_new (tag);
      g_string_erase (norm,0,16);
      //biji_item_add_notebook (BIJI_ITEM (n), NULL, norm->str);
      //g_queue_push_head (self->notebooks, norm->str);
      g_string_free (norm, TRUE);
    }

    free (tag);
  }


  xmlFree(name);
}

static void
biji_tomboy_reader_constructed (GObject *obj)
{
  BijiTomboyReader *self;
  xmlDocPtr doc;
  xmlNodePtr cur;
  xmlChar     *version;

  self = BIJI_TOMBOY_READER (obj);

  doc = xmlParseFile (self->path);

  if (doc == NULL )
  {
    self->error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                               "File not parsed successfully");
    return;
  }


  cur = xmlDocGetRootElement (doc);

  if (cur == NULL)
  {
    self->error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                               "File empty");
    xmlFreeDoc(doc);
    return;
  }


  if (xmlStrcmp(cur->name, (const xmlChar *) "note"))
  {
    self->error = g_error_new (G_IO_ERROR, G_IO_ERROR_FAILED,
                               "Root node != note");
    xmlFreeDoc(doc);
    return;
  }

  version = xmlGetNoNsProp (cur, BAD_CAST "version");

  if (g_strcmp0 ((const gchar*) version, "0.1") == 0)
    self->type = TOMBOY_1;

  else if (g_strcmp0 ((const gchar*) version, "0.2") == 0)
    self->type = TOMBOY_2;

  else if (g_strcmp0 ((const gchar*) version, "0.3") == 0)
    self->type = TOMBOY_3;

  self->r = xmlNewTextReaderFilename (self->path);

  while ( xmlTextReaderRead(self->r) == 1 )
  {
    if ( xmlTextReaderNodeType(self->r) == 1 )
    {
      processNode(self);
    }
  }

  xmlFree (version);
  xmlFreeDoc (doc);
}


static void
biji_tomboy_reader_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BijiTomboyReader *self = BIJI_TOMBOY_READER (object);


  switch (property_id)
    {
    case PROP_SRC_PATH:
      self->path = g_value_dup_string (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}



static void
biji_tomboy_reader_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BijiTomboyReader *self = BIJI_TOMBOY_READER (object);

  switch (property_id)
    {
    case PROP_SRC_PATH:
      g_value_set_string (value, self->path);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}




static void
biji_tomboy_reader_finalize (GObject *object)
{
  BijiTomboyReader *self;

  g_return_if_fail (BIJI_IS_TOMBOY_READER (object));

  self = BIJI_TOMBOY_READER (object);

  g_string_free (self->raw_text, TRUE);
  g_string_free (self->html, TRUE);
  g_clear_error (&self->error);

  xmlFreeTextReader (self->r);
  xmlFreeTextReader (self->inner);


  G_OBJECT_CLASS (biji_tomboy_reader_parent_class)->finalize (object);
}


static void
biji_tomboy_reader_init (BijiTomboyReader *self)
{
  self->raw_text = g_string_new ("");
  self->html = g_string_new ("<html xmlns=\"http://www.w3.org/1999/xhtml\"><body>");

  self->set = biji_info_set_new ();
}



static void
biji_tomboy_reader_class_init (BijiTomboyReaderClass *klass)
{
  GObjectClass *g_object_class;

  g_object_class = G_OBJECT_CLASS (klass);

  g_object_class->finalize = biji_tomboy_reader_finalize;
  g_object_class->constructed = biji_tomboy_reader_constructed;
  g_object_class->set_property = biji_tomboy_reader_set_property;
  g_object_class->get_property = biji_tomboy_reader_get_property;

  properties[PROP_SRC_PATH] =
    g_param_spec_string ("path",
                         "External note path",
                         "A Tomboy-formated xml note to import",
                         "",
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (g_object_class, TOMBOY_READER_PROP, properties);
}


static BijiTomboyReader *
biji_tomboy_reader_new (gchar *source)
{
  return g_object_new (BIJI_TYPE_TOMBOY_READER, "path", source, NULL);
}



gboolean
biji_tomboy_reader_read (gchar *source,
                         GError **error,
                         BijiInfoSet **set,
                         gchar       **html,
                         GList **notebooks)
{
  BijiTomboyReader *self;
  gchar *html_revamped;

  self = biji_tomboy_reader_new (source);

  html_revamped = biji_str_mass_replace (self->html->str,
                                         "\n", "<br/>",
                                         NULL);

  *error = self->error;
  *set = self->set;
  *html = html_revamped;

  g_object_unref (self);
  return TRUE;
}
