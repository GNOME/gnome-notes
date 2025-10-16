/* bjb-note.c
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

#define G_LOG_DOMAIN "bjb-note"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "bjb-tag.h"
#include "bjb-note.h"

typedef struct
{
  char       *html;
  BjbItem    *notebook;
  /* list of tags currently used by @self */
  GListStore *tags;
} BjbNotePrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BjbNote, bjb_note, BJB_TYPE_ITEM)

enum {
  PROP_0,
  PROP_CONTENT,
  PROP_RAW_CONTENT,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static char *
bjb_note_real_get_text_content (BjbNote *self)
{
  g_assert (BJB_IS_NOTE (self));

  return NULL;
}

static void
bjb_note_real_set_text_content (BjbNote    *self,
                                const char *content)
{
}

static char *
bjb_note_real_get_raw_content (BjbNote *self)
{
  g_assert (BJB_IS_NOTE (self));

  return NULL;
}

static void
bjb_note_real_set_raw_content (BjbNote    *self,
                               const char *content)
{
}

static BjbItem *
bjb_note_real_get_notebook (BjbNote *self)
{
  g_assert (BJB_IS_NOTE (self));

  return NULL;
}

static const char *
bjb_note_real_get_extension (BjbNote *self)
{
  return ".txt";
}

static void
bjb_note_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  BjbNote *self = (BjbNote *)object;

  switch (prop_id)
    {
    case PROP_CONTENT:
      g_value_take_string (value, bjb_note_get_text_content (self));
      break;

    case PROP_RAW_CONTENT:
      g_value_take_string (value, bjb_note_get_raw_content (self));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bjb_note_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  BjbNote *self = BJB_NOTE (object);

  switch (prop_id)
    {
    case PROP_CONTENT:
      bjb_note_set_text_content (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bjb_note_finalize (GObject *object)
{
  BjbNote *self = BJB_NOTE (object);
  BjbNotePrivate *priv = bjb_note_get_instance_private (self);

  g_clear_pointer (&priv->html, g_free);
  g_clear_object (&priv->notebook);
  g_clear_object (&priv->tags);

  G_OBJECT_CLASS (bjb_note_parent_class)->finalize (object);
}

static void
bjb_note_class_init (BjbNoteClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bjb_note_get_property;
  object_class->set_property = bjb_note_set_property;
  object_class->finalize = bjb_note_finalize;

  klass->get_text_content = bjb_note_real_get_text_content;
  klass->set_text_content = bjb_note_real_set_text_content;
  klass->get_raw_content = bjb_note_real_get_raw_content;
  klass->set_raw_content = bjb_note_real_set_raw_content;

  klass->get_notebook = bjb_note_real_get_notebook;
  klass->get_extension = bjb_note_real_get_extension;

  /**
   * BjbNote:content:
   *
   * The plain text content of the note. Shall be used to feed
   * into tracker, or to search within the note.
   */
  properties[PROP_CONTENT] =
    g_param_spec_string ("content",
                         "Content",
                         "The text content of the note",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * BjbNote:raw-content:
   *
   * The raw-content of the note may include XML/markdown content,
   * or whatever is used to keep the note formatted, if any.
   *
   * This is the text that is feed as the content to save. May
   * include title, color, and other details.
   */
  properties[PROP_RAW_CONTENT] =
    g_param_spec_string ("raw-content",
                         "Raw Content",
                         "The raw content of the note",
                         NULL,
                         G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bjb_note_init (BjbNote *self)
{
  BjbNotePrivate *priv = bjb_note_get_instance_private (self);

  priv->tags = g_list_store_new (BJB_TYPE_TAG);
}

/**
 * bjb_note_get_text_content:
 * @self: a #BjbNote
 *
 * Get the plain content of the note, without any
 * formatting tags, if any
 *
 * Returns (transfer full) (nullable): the plain
 * text content of the note
 */
char *
bjb_note_get_text_content (BjbNote *self)
{
  g_return_val_if_fail (BJB_IS_NOTE (self), NULL);

  return BJB_NOTE_GET_CLASS (self)->get_text_content (self);
}

/**
 * bjb_note_set_text_content:
 * @self: a #BjbNote
 * @content: The text to set as content
 *
 * Set the plain text content of the note. This should
 * not contain the title of the note.
 */
void
bjb_note_set_text_content (BjbNote    *self,
                           const char *content)
{
  g_return_if_fail (BJB_IS_NOTE (self));

  BJB_NOTE_GET_CLASS (self)->set_text_content (self, content);
}

/**
 * bjb_note_get_raw_content:
 * @self: a #BjbNote
 *
 * Get the raw content, may contain XML tags, or
 * whatever is used for formatting.
 *
 * Returns (transfer full) (nullable): the raw text
 * content of the note.
 */
char *
bjb_note_get_raw_content (BjbNote *self)
{
  g_return_val_if_fail (BJB_IS_NOTE (self), NULL);

  return BJB_NOTE_GET_CLASS (self)->get_raw_content (self);
}

/**
 * bjb_note_set_raw_content:
 * @self: a #BjbNote
 * @content: The text to set as content
 *
 * Set the raw text content of the note.
 */
void
bjb_note_set_raw_content (BjbNote    *self,
                          const char *content)
{
  g_return_if_fail (BJB_IS_NOTE (self));

  BJB_NOTE_GET_CLASS (self)->set_raw_content (self, content);
}

char *
bjb_note_get_html (BjbNote *self)
{
  BjbNotePrivate *priv = bjb_note_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_NOTE (self), NULL);

  return g_strdup (priv->html);
}

void
bjb_note_set_html (BjbNote    *self,
                   const char *html)
{
  BjbNotePrivate *priv = bjb_note_get_instance_private (self);

  g_return_if_fail (BJB_IS_NOTE (self));

  g_free (priv->html);
  priv->html = g_strdup (html);
}

void
bjb_note_add_tag (BjbNote    *self,
                  const char *tag_title)
{
  BjbNotePrivate *priv = bjb_note_get_instance_private (self);
  BjbTagStore *tag_store;
  BjbItem *tag;

  g_return_if_fail (BJB_IS_NOTE (self));

  if (!tag_title || !*tag_title)
    return;

  tag_store = BJB_NOTE_GET_CLASS (self)->get_tag_store (self);
  g_assert (BJB_IS_TAG_STORE (tag_store));
  tag = bjb_tag_store_add_tag (tag_store, tag_title);

  if (g_list_store_find (priv->tags, tag, NULL))
    return;

  g_list_store_append (priv->tags, tag);
  bjb_item_set_modified (BJB_ITEM (self));
}

void
bjb_note_remove_tag (BjbNote *self,
                     BjbItem *tag)
{
  BjbNotePrivate *priv = bjb_note_get_instance_private (self);
  guint position;

  g_return_if_fail (BJB_IS_NOTE (self));
  g_return_if_fail (BJB_IS_TAG (tag));


  if (g_list_store_find (priv->tags, tag, &position))
    {
      g_list_store_remove (priv->tags, position);
      bjb_item_set_modified (BJB_ITEM (self));
    }
}

void
bjb_note_set_notebook (BjbNote *self,
                       BjbItem *notebook)
{
  BjbNotePrivate *priv = bjb_note_get_instance_private (self);

  g_return_if_fail (BJB_IS_NOTE (self));
  g_return_if_fail (BJB_IS_NOTEBOOK (notebook));

  if (g_set_object (&priv->notebook, notebook))
    bjb_item_set_modified (BJB_ITEM (self));
}

/**
 * bjb_note_get_notebook:
 * @self: a #BjbNote
 *
 * Get the notebook that contains @self.
 *
 * Returns: (transfer none) (nullable): A #BjbItem
 * or %NULL if not in notebook or if unsupported.
 */
BjbItem *
bjb_note_get_notebook (BjbNote *self)
{
  BjbNotePrivate *priv = bjb_note_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_NOTE (self), NULL);

  return priv->notebook;
}

/**
 * bjb_note_get_tags:
 * @self: a #BjbNote
 *
 * Get the list of tags (labels) of @self, if any.
 *
 * Returns: (transfer none) (nullable): A list of #BjbTags, or %NULL
 * if not supported.
 */
GListModel *
bjb_note_get_tags (BjbNote *self)
{
  BjbNotePrivate *priv = bjb_note_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_NOTE (self), NULL);

  return G_LIST_MODEL (priv->tags);
}

/**
 * bjb_note_get_file_extension:
 * @self: a #BjbNote
 *
 * Get the commonly used file extension for the given
 * note @self.  Say for example, if @self is a #BjbPlainNote
 * ".txt" is returned.
 *
 * Returns: (transfer none): The file extension for the
 * given note
 */
const char *
bjb_note_get_file_extension (BjbNote *self)
{
  g_return_val_if_fail (BJB_IS_NOTE (self), ".txt");

  return BJB_NOTE_GET_CLASS (self)->get_extension (self);
}
