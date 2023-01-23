/* bjb-tag-store.c
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

#define G_LOG_DOMAIN "bjb-tag-store"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "bjb-tag.h"
#include "bjb-tag-store.h"

struct _BjbTagStore
{
  GObject     parent_instance;
  GListStore *notebooks;
  GListStore *tags;
};


G_DEFINE_TYPE (BjbTagStore, bjb_tag_store, G_TYPE_OBJECT)

static BjbItem *
bjb_tag_store_find_item (GListStore *store,
                         const char *title)
{
  g_autofree char *casefold = NULL;
  guint n_items;

  g_return_val_if_fail (G_IS_LIST_STORE (store), NULL);
  g_return_val_if_fail (title && *title, NULL);

  n_items = g_list_model_get_n_items (G_LIST_MODEL (store));
  casefold = g_utf8_casefold (title, -1);

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(BjbItem) item = NULL;
      g_autofree char *item_casefold = NULL;

      item = g_list_model_get_item (G_LIST_MODEL (store), i);
      item_casefold = g_utf8_casefold (bjb_item_get_title (item), -1);

      if (g_strcmp0 (item_casefold, casefold) == 0)
        return g_object_ref (item);
    }

  return NULL;
}

static void
bjb_tag_store_finalize (GObject *object)
{
  BjbTagStore *self = (BjbTagStore *)object;

  g_clear_object (&self->notebooks);
  g_clear_object (&self->tags);

  G_OBJECT_CLASS (bjb_tag_store_parent_class)->finalize (object);
}

static void
bjb_tag_store_class_init (BjbTagStoreClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bjb_tag_store_finalize;
}

static void
bjb_tag_store_init (BjbTagStore *self)
{
  self->tags = g_list_store_new (BJB_TYPE_TAG);
  self->notebooks = g_list_store_new (BJB_TYPE_NOTEBOOK);
}

BjbTagStore *
bjb_tag_store_new (void)
{
  return g_object_new (BJB_TYPE_TAG_STORE, NULL);
}

GListModel *
bjb_tag_store_get_tags (BjbTagStore *self)
{
  g_return_val_if_fail (BJB_IS_TAG_STORE (self), NULL);

  return G_LIST_MODEL (self->tags);
}

GListModel *
bjb_tag_store_get_notebooks  (BjbTagStore *self)
{
  g_return_val_if_fail (BJB_IS_TAG_STORE (self), NULL);

  return G_LIST_MODEL (self->notebooks);
}

/**
 * bjb_tag_store_add_tag:
 * @self: A #BjbTagStore
 * @tag_title: The tag to add
 *
 * Add a new tag with @tag_title.  If a tag with
 * @tag_title already exists, return that, instead
 * of adding a new one.
 *
 * Returns; (transfer none): A #BjbItem
 */
BjbItem *
bjb_tag_store_add_tag (BjbTagStore *self,
                       const char  *tag_title)
{
  g_autoptr(BjbItem) tag = NULL;

  g_return_val_if_fail (BJB_IS_TAG_STORE (self), NULL);
  g_return_val_if_fail (tag_title && *tag_title, NULL);

  tag = bjb_tag_store_find_item (self->tags, tag_title);

  if (!tag)
    {
      tag = bjb_tag_new (tag_title);
      g_list_store_append (self->tags, tag);
    }

  return tag;
}

/**
 * bjb_notebook_store_add_notebook:
 * @self: A #BjbNotebookStore
 * @notebook_title: The notebook to add
 *
 * Add a new notebook with @notebook_title.  If a notebook
 * with @notebook_title already exists, return that, instead
 * of adding a new one.
 *
 * A note can have at most one notebook.  But a note can
 * have multiple tags.
 *
 * Returns; (transfer none): A #BjbItem
 */
BjbItem *
bjb_tag_store_add_notebook (BjbTagStore *self,
                            const char  *notebook_title)
{
  g_autoptr(BjbItem) notebook = NULL;

  g_return_val_if_fail (BJB_IS_TAG_STORE (self), NULL);
  g_return_val_if_fail (notebook_title && *notebook_title, NULL);

  notebook = bjb_tag_store_find_item (self->notebooks, notebook_title);

  if (!notebook)
    {
      notebook = bjb_notebook_new (NULL, notebook_title, 0);
      g_list_store_append (self->notebooks, notebook);
    }

  return notebook;
}
