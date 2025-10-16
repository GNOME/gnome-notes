/* bjb-item.c
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

#define G_LOG_DOMAIN "bjb-item"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "bjb-item.h"

typedef struct
{
  char     *uid;
  char     *title;

  GdkRGBA  *rgba;

  /* The last modified time of the item */
  gint64    mtime;
  /* The modified time of metadata of the item */
  gint64    meta_mtime;

  /* The creation time of the item */
  gint64    creation_time;
  gboolean  modified;
  gboolean  trashed;
} BjbItemPrivate;

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BjbItem, bjb_item, G_TYPE_OBJECT)

enum {
  PROP_0,
  PROP_TITLE,
  PROP_COLOR,
  PROP_CREATE_TIME,
  PROP_MTIME,
  PROP_META_MTIME,
  PROP_MODIFIED,
  N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
bjb_item_get_property (GObject    *object,
                       guint       prop_id,
                       GValue     *value,
                       GParamSpec *pspec)
{
  BjbItem *self = (BjbItem *)object;
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_TITLE:
      g_value_set_string (value, bjb_item_get_title (self));
      break;

    case PROP_COLOR:
      g_value_set_boxed (value, priv->rgba);
      break;

    case PROP_CREATE_TIME:
      g_value_set_int64 (value, priv->creation_time);
      break;

    case PROP_MTIME:
      g_value_set_int64 (value, priv->mtime);
      break;

    case PROP_META_MTIME:
      g_value_set_int64 (value, priv->meta_mtime);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bjb_item_set_property (GObject      *object,
                       guint         prop_id,
                       const GValue *value,
                       GParamSpec   *pspec)
{
  BjbItem *self = (BjbItem *)object;
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  switch (prop_id)
    {
    case PROP_TITLE:
      bjb_item_set_title (self, g_value_get_string (value));
      break;

    case PROP_COLOR:
      bjb_item_set_rgba (self, g_value_get_boxed (value));
      break;

    case PROP_CREATE_TIME:
      priv->creation_time = g_value_get_int64 (value);
      break;

    case PROP_MTIME:
      priv->mtime = g_value_get_int64 (value);
      break;

    case PROP_META_MTIME:
      priv->meta_mtime = g_value_get_int64 (value);
      break;

    case PROP_MODIFIED:
      priv->modified = g_value_get_boolean (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bjb_item_finalize (GObject *object)
{
  BjbItem *self = (BjbItem *)object;
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_clear_pointer (&priv->uid, g_free);
  g_clear_pointer (&priv->title, g_free);
  g_clear_pointer (&priv->rgba, gdk_rgba_free);

  G_OBJECT_CLASS (bjb_item_parent_class)->finalize (object);
}

static gboolean
bjb_item_real_is_modified (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_assert (BJB_IS_ITEM (self));

  return priv->modified;
}

static void
bjb_item_real_unset_modified (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_assert (BJB_IS_ITEM (self));

  if (priv->modified == FALSE)
    return;

  priv->modified = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODIFIED]);
}

static gboolean
bjb_item_real_match (BjbItem      *self,
                    const char *needle)
{
  g_autofree char *title = NULL;

  g_assert (BJB_IS_ITEM (self));

  title = g_utf8_casefold (bjb_item_get_title (self), -1);
  if (strstr (title, needle) != NULL)
    return TRUE;

  return FALSE;
}

static BjbFeature
bjb_item_real_get_features (BjbItem *self)
{
  g_assert (BJB_IS_ITEM (self));

  return BJB_FEATURE_NONE;
}

static void
bjb_item_class_init (BjbItemClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bjb_item_get_property;
  object_class->set_property = bjb_item_set_property;
  object_class->finalize = bjb_item_finalize;

  klass->is_modified = bjb_item_real_is_modified;
  klass->unset_modified = bjb_item_real_unset_modified;
  klass->match = bjb_item_real_match;
  klass->get_features = bjb_item_real_get_features;

  /**
   * BjbItem:title:
   *
   * The name of the Item. May be %NULL if title is empty.
   */
  properties[PROP_TITLE] =
    g_param_spec_string ("title",
                         "Title",
                         "The name of the Item",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BjbItem:color:
   *
   * The #GdkRGBA color of the item. Set to %NULL if the item doesn't
   * support color.
   *
   */
  properties[PROP_COLOR] =
    g_param_spec_boxed ("color",
                        "RGBA Color",
                        "The RGBA color of the item",
                        GDK_TYPE_RGBA,
                        G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BjbItem:mtime:
   *
   * The date and time (in seconds since UNIX epoch) the item was last
   * modified. A value of 0 denotes that the item doesn't support this
   * feature, or the note is item is not yet saved. To check if the
   * item is saved use bjb_item_is_new().
   */
  properties[PROP_MTIME] =
    g_param_spec_int64 ("mtime",
                        "Modification Time",
                        "The Unix time in seconds the item was last modified",
                        0, G_MAXINT64, 0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * BjbItem:meta-mtime:
   *
   */
  properties[PROP_META_MTIME] =
    g_param_spec_int64 ("meta-mtime",
                        "Metadata Modification Time",
                        "The Unix time in seconds the item meta-data was last modified",
                        0, G_MAXINT64, 0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * BjbItem:creation-time:
   *
   * The date and time (in seconds, UNIX epoch) the item was created.
   * A value of 0 denotes that the item doesn't support this feature.
   *
   * The creation time can be the time the file was created (if the
   * backend is a file), or the time at which the data was saved to
   * database, etc.
   */
  properties[PROP_CREATE_TIME] =
    g_param_spec_int64 ("create-time",
                        "Create Time",
                        "The Unix time in seconds the item was created",
                        0, G_MAXINT64, 0,
                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  /**
   * BjbItem:modified:
   *
   * The modification state of the item.  %TRUE if the item
   * has modified since last save.
   *
   * As derived classes can have more properties than this base class,
   * the implementation should do bjb_item_unset_modified() after the
   * item have saved.  Also on save, uid should be set.
   */
  properties[PROP_MODIFIED] =
    g_param_spec_boolean ("modified",
                          "Modified",
                          "The modified state of the item",
                          FALSE,
                          G_PARAM_READABLE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bjb_item_init (BjbItem *self)
{
}

/**
 * bjb_item_get_uid:
 * @self: a #BjbItem
 *
 * Get the unique id of the item.
 *
 * Returns: (transfer none) (nullable): the uid of the item
 */
const char *
bjb_item_get_uid (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_ITEM (self), NULL);

  return priv->uid;
}

/**
 * bjb_item_set_uid:
 * @self: a #BjbItem
 * @uid: (nullable): a text to set as uid
 *
 * Set A unique identifier of the item. This can be a URL, URN,
 * Primary key of a database table or any kind of unique string
 * that can be used as an identifier.
 *
 * the uid of a saved item (ie, saved to file, goa, etc.) should
 * not be %NULL.
 */
void
bjb_item_set_uid (BjbItem   *self,
                 const char *uid)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_if_fail (BJB_IS_ITEM (self));

  /* if (g_strcmp0 (priv->uid, uid) == 0) */
  /*   return; */

  g_free (priv->uid);
  priv->uid = g_strdup (uid);

  bjb_item_set_modified (self);
  /* g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_UID]); */
}

/**
 * bjb_item_get_title:
 * @self: a #BjbItem
 *
 * Get the title/name of the item
 *
 * Returns: (transfer none): the title of the item.
 */
const char *
bjb_item_get_title (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_ITEM (self), NULL);

  return priv->title ? priv->title : "";
}

/**
 * bjb_item_set_title:
 * @self: a #BjbItem
 * @title: (nullable): a text to set as title
 *
 * Set the title of the item. Can be %NULL.
 */
void
bjb_item_set_title (BjbItem   *self,
                   const char *title)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_if_fail (BJB_IS_ITEM (self));

  /* If the new and old titles are same, don't bother changing */
  if (g_strcmp0 (priv->title, title) == 0)
    return;

  g_free (priv->title);
  priv->title = g_strdup (title);

  bjb_item_set_modified (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

/**
 * bjb_item_get_rgba:
 * @self: a #BjbItem
 * @rgba (out) (nullable): a location for #GdkRGBA
 *
 * Get the #GdkRGBA of the item
 *
 * Returns: %TRUE if the item has a color. %FALSE otherwise
 */
gboolean
bjb_item_get_rgba (BjbItem *self,
                   GdkRGBA *rgba)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_ITEM (self), FALSE);

  if (priv->rgba == NULL)
    return FALSE;

  if (rgba)
    *rgba = *priv->rgba;

  return TRUE;
}

/**
 * bjb_item_set_rgba:
 * @self: a #BjbItem
 * @rgba: a #GdkRGBA
 *
 * Set the #GdkRGBA of the item
 */
void
bjb_item_set_rgba (BjbItem       *self,
                   const GdkRGBA *rgba)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_if_fail (BJB_IS_ITEM (self));
  g_return_if_fail (rgba != NULL);

  if (priv->rgba != NULL &&
      gdk_rgba_equal (priv->rgba, rgba))
    return;

  gdk_rgba_free (priv->rgba);
  priv->rgba = gdk_rgba_copy (rgba);

  bjb_item_set_modified (self);
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_COLOR]);
}

/**
 * bjb_item_get_creation_time:
 * @self: a #BjbItem
 *
 * Get the creation time of the item. The time returned is the
 * seconds since UNIX epoch.
 *
 * A value of zero means the item isn't yet saved or the item
 * doesn't support creation-time property. bjb_item_is_new()
 * can be used to check if item is saved.
 *
 * Returns: a signed integer representing creation time
 */
gint64
bjb_item_get_create_time (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_ITEM (self), 0);

  return priv->creation_time;
}

void
bjb_item_set_create_time (BjbItem *self,
                          gint64   time)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_if_fail (BJB_IS_ITEM (self));

  priv->creation_time = time;
}

/**
 * bjb_item_get_mtime:
 * @self: a #BjbItem
 *
 * Get the modification time of the item. The time returned is
 * the seconds since UNIX epoch.
 *
 * A value of zero means the item isn't yet saved or the item
 * doesn't support modification-time property. bjb_item_is_new()
 * can be used to check if item is saved.
 *
 * Returns: a signed integer representing modification time
 */
gint64
bjb_item_get_mtime (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_ITEM (self), 0);

  return priv->mtime;
}

void
bjb_item_set_mtime (BjbItem *self,
                    gint64   time)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_if_fail (BJB_IS_ITEM (self));

  priv->mtime = time;
}

/**
 * bjb_item_get_meta_mtime:
 * @self: a #BjbItem
 *
 * Get the meta-data (eg: color, tag, etc.) modification
 * time of the item.
 *
 * see bjb_item_get_mtime() for details.
 *
 * Returns: a signed integer representing metadata
 * modification time
 */
gint64
bjb_item_get_meta_mtime (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_ITEM (self), 0);

  return priv->meta_mtime;
}

void
bjb_item_set_meta_mtime (BjbItem *self,
                         gint64   time)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_if_fail (BJB_IS_ITEM (self));

  priv->meta_mtime = time;
}

gboolean
bjb_item_is_trashed (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_ITEM (self), FALSE);

  return priv->trashed;
}

void
bjb_item_set_is_trashed (BjbItem  *self,
                         gboolean  is_trashed)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_if_fail (BJB_IS_ITEM (self));

  priv->trashed = !!is_trashed;
}

/**
 * bjb_item_is_modified:
 * @self: a #BjbItem
 *
 * Get if the item is modified
 *
 * Returns: %TRUE if the item has been modified since
 * last save. %FALSE otherwise.
 */
gboolean
bjb_item_is_modified (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_ITEM (self), FALSE);

  return priv->modified;
}

void
bjb_item_set_modified (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_assert (BJB_IS_ITEM (self));

  /*
   * Irrespective of the previous state, we always need to notify
   * ::modified.
   */
  priv->modified = TRUE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODIFIED]);
}

/**
 * bjb_item_unset_modified:
 * @self: a #BjbItem
 *
 * Unmark the item as modified.
 */
void
bjb_item_unset_modified (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_if_fail (BJB_IS_ITEM (self));

  priv->modified = FALSE;
  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODIFIED]);
}

/**
 * bjb_item_is_new:
 * @self: a #BjbItem
 *
 * Get if the item is new (not yet saved).
 *
 * Returns: a boolean
 */
gboolean
bjb_item_is_new (BjbItem *self)
{
  BjbItemPrivate *priv = bjb_item_get_instance_private (self);

  g_return_val_if_fail (BJB_IS_ITEM (self), FALSE);

  /* If uid is NULL, that means the item isn't saved */
  return priv->uid == NULL;
}

/**
 * bjb_item_compare:
 * @a: A #BjbItem
 * @b: A #BjbItem
 *
 * Compare two #BjbItem
 *
 * Returns: an integer less than, equal to, or greater
 * than 0, if title of @a is <, == or > than title of @b.
 */
int
bjb_item_compare (gconstpointer a,
                  gconstpointer b,
                  gpointer      user_data)
{
  BjbItemPrivate *item_a = bjb_item_get_instance_private ((BjbItem *)a);
  BjbItemPrivate *item_b = bjb_item_get_instance_private ((BjbItem *)b);
  g_autofree char *title_a = NULL;
  g_autofree char *title_b = NULL;

  if (item_a == item_b)
    return 0;

  title_a = g_utf8_casefold (item_a->title, -1);
  title_b = g_utf8_casefold (item_b->title, -1);

  return g_strcmp0 (title_a, title_b);
}

gboolean
bjb_item_match (BjbItem   *self,
               const char *needle)
{
  g_autofree char *casefold_needle = NULL;

  g_return_val_if_fail (BJB_IS_ITEM (self), FALSE);
  g_return_val_if_fail (needle != NULL, FALSE);

  casefold_needle = g_utf8_casefold (needle, -1);

  return BJB_ITEM_GET_CLASS (self)->match (self, casefold_needle);
}

/**
 * bjb_item_get_features:
 * @self: a #BjbItem
 *
 * Get the features supported by @self.
 *
 * Returns: A #BjbFeature
 */
BjbFeature
bjb_item_get_features (BjbItem *self)
{
  g_return_val_if_fail (BJB_IS_ITEM (self), 0);

  return BJB_ITEM_GET_CLASS (self)->get_features (self);
}
