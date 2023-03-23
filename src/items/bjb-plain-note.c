/* bjb-plain-note.c
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

#define G_LOG_DOMAIN "bjb-plain-note"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <gtk/gtk.h>

#include "biji-string.h"
#include "bjb-plain-note.h"

struct _BjbPlainNote
{
  BjbNote      parent_instance;

  char        *content;
};

G_DEFINE_TYPE (BjbPlainNote, bjb_plain_note, BJB_TYPE_NOTE)

static char *
html_from_plain_content (const char *content)
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

static char *
bjb_plain_note_get_text_content (BjbNote *note)
{
  BjbPlainNote *self = BJB_PLAIN_NOTE (note);

  g_assert (BJB_IS_NOTE (note));

  return g_strdup (self->content);
}

static void
bjb_plain_note_set_text_content (BjbNote    *note,
                                 const char *content)
{
  BjbPlainNote *self = BJB_PLAIN_NOTE (note);
  g_autofree char *html = NULL;

  g_assert (BJB_IS_PLAIN_NOTE (self));

  g_free (self->content);
  self->content = g_strdup (content);

  html = html_from_plain_content (content);
  bjb_note_set_html (note, html);
  bjb_item_set_modified (BJB_ITEM (self));
}

static void
bjb_plain_note_finalize (GObject *object)
{
  BjbPlainNote *self = BJB_PLAIN_NOTE (object);

  g_clear_pointer (&self->content, g_free);

  G_OBJECT_CLASS (bjb_plain_note_parent_class)->finalize (object);
}

static void
bjb_plain_note_class_init (BjbPlainNoteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BjbNoteClass *note_class = BJB_NOTE_CLASS (klass);

  object_class->finalize = bjb_plain_note_finalize;

  note_class->get_text_content = bjb_plain_note_get_text_content;
  note_class->set_text_content = bjb_plain_note_set_text_content;
  note_class->get_raw_content = bjb_plain_note_get_text_content;
  note_class->set_raw_content = bjb_plain_note_set_text_content;
}

static void
bjb_plain_note_init (BjbPlainNote *self)
{
}

/**
 * bjb_plain_note_new_from_data:
 * @uid: (nullable): A unique id of the note
 * @title: (nullable): The title of the note
 * @content: (nullable): The content of the note
 *
 * Create a new plain note from the given details.
 *
 * Returns: (transfer full): a new #BjbPlainNote
 */
BjbItem *
bjb_plain_note_new_from_data (const char *uid,
                              const char *title,
                              const char *content)
{
  BjbPlainNote *self;

  self = g_object_new (BJB_TYPE_PLAIN_NOTE,
                       "title", title,
                       "content", content,
                       NULL);

  bjb_item_set_uid (BJB_ITEM (self), uid);

  return BJB_ITEM (self);
}
