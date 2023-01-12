/* bjb-tag.h
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

#define G_LOG_DOMAIN "bjb-tag"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "bjb-tag.h"

struct _BjbTag
{
  BjbItem    parent_instance;
};

G_DEFINE_FINAL_TYPE (BjbTag, bjb_tag, BJB_TYPE_ITEM)

static void
bjb_tag_class_init (BjbTagClass *klass)
{
}

static void
bjb_tag_init (BjbTag *self)
{
}

/**
 * bjb_tag_new:
 * @title: The tag name
 *
 * Create a new tag with given name.  A note
 * can have multiple tags if supported.
 *
 * Returns: (transfer full): A #BjbTag
 */
BjbItem *
bjb_tag_new (const char *title)
{
  g_return_val_if_fail (title && *title, NULL);

  return g_object_new (BJB_TYPE_TAG, "title", title, NULL);
}


struct _BjbNotebook
{
  BjbItem    parent_instance;
};

G_DEFINE_TYPE (BjbNotebook, bjb_notebook, BJB_TYPE_ITEM)

static void
bjb_notebook_class_init (BjbNotebookClass *klass)
{
}

static void
bjb_notebook_init (BjbNotebook *self)
{
}

/**
 * bjb_notebook_new:
 *
 * Create a new notebook with given name.  A note
 * can have at most one notebook if supported.
 *
 * Returns: (transfer full): A #BjbNotebook
 */
BjbItem *
bjb_notebook_new (const char *uid,
                  const char *title,
                  gint64     mtime)
{
  BjbItem *item;

  g_return_val_if_fail (title && *title, NULL);

  item = g_object_new (BJB_TYPE_NOTEBOOK,
                       "title", title,
                       "mtime", mtime,
                       NULL);
  bjb_item_set_uid (item, uid);

  return item;
}
