/* bjb-note-manager.c
 * Copyright (C) Pierre-Yves LUYTEN 2012 <py@luyten.fr>
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

#define G_LOG_DOMAIN "bjb-manager"

#include <gtk/gtk.h>

#include "config.h"
#include "libbiji.h"

struct _BijiManager
{
  GObject parent_instance;

  /* Notes & Collections.
   * Keep a direct pointer to local provider for convenience
   *
   * TODO: would be nice to have GHashTable onto providers
   * rather than one big central db here
   */

  GListStore *notebooks;
  GListStore *notes;
  GListStore *archives;
  GListStore *providers;

  GFile *location;

  GdkRGBA color;
};


/* Properties */
enum {
  PROP_0,
  PROP_LOCATION,
  PROP_COLOR,
  BIJI_MANAGER_PROPERTIES
};


/* Signals */
enum {
  BOOK_AMENDED,
  BIJI_MANAGER_SIGNALS
};

static guint biji_manager_signals[BIJI_MANAGER_SIGNALS] = { 0 };
static GParamSpec *properties[BIJI_MANAGER_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (BijiManager, biji_manager, G_TYPE_OBJECT)

static void
biji_manager_init (BijiManager *self)
{
  self->notebooks = g_list_store_new (BJB_TYPE_NOTEBOOK);
  self->notes = g_list_store_new (BJB_TYPE_NOTE);
  self->archives = g_list_store_new (BJB_TYPE_NOTE);
}

static void
biji_manager_finalize (GObject *object)
{
  BijiManager *self = BIJI_MANAGER (object);

  g_clear_object (&self->notebooks);
  g_clear_object (&self->location);
  g_clear_object (&self->notes);
  g_clear_object (&self->archives);
  g_clear_object (&self->providers);

  G_OBJECT_CLASS (biji_manager_parent_class)->finalize (object);
}




static void
biji_manager_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BijiManager *self = BIJI_MANAGER (object);
  GdkRGBA *color;

  switch (property_id)
    {
    case PROP_LOCATION:
      self->location = g_value_dup_object (value);
      break;

    case PROP_COLOR:
      color = g_value_get_pointer (value);
      self->color.red = color->red;
      self->color.blue = color->blue;
      self->color.green = color->green;
      self->color.alpha = color->alpha;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_manager_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BijiManager *self = BIJI_MANAGER (object);

  switch (property_id)
    {
    case PROP_LOCATION:
      g_value_set_object (value, self->location);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_manager_constructed (GObject *object)
{
  gchar *filename;
  GFile *cache;

  G_OBJECT_CLASS (biji_manager_parent_class)->constructed (object);

  /* Ensure cache directory for icons */
  filename = g_build_filename (g_get_user_cache_dir (),
                               g_get_application_name (),
                               NULL);
  cache = g_file_new_for_path (filename);
  g_free (filename);
  g_file_make_directory (cache, NULL, NULL);
  g_object_unref (cache);
}

static void
biji_manager_class_init (BijiManagerClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = biji_manager_finalize;
  object_class->constructed = biji_manager_constructed;
  object_class->set_property = biji_manager_set_property;
  object_class->get_property = biji_manager_get_property;

  biji_manager_signals[BOOK_AMENDED] =
    g_signal_new ("changed", G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,                         /* offset & accumulator */
                  _biji_marshal_VOID__ENUM_ENUM_POINTER,
                  G_TYPE_NONE, 3, G_TYPE_INT, G_TYPE_INT, G_TYPE_POINTER);

  properties[PROP_LOCATION] =
    g_param_spec_object ("location",
                         "The manager location",
                         "The location where the notes are loaded and saved",
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);


  properties[PROP_COLOR] =
    g_param_spec_pointer ("color",
                         "Default color",
                         "Note manager default color for notes",
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);


  g_object_class_install_properties (object_class, BIJI_MANAGER_PROPERTIES, properties);
}


GListModel *
biji_manager_get_notes (BijiManager    *self,
                        BijiItemsGroup  group)
{
  switch (group)
  {
    case BIJI_LIVING_ITEMS:
      return G_LIST_MODEL (self->notes);

    case BIJI_ARCHIVED_ITEMS:
      return G_LIST_MODEL (self->archives);

    default:
      g_warning ("invalid BijiItemsGroup:%i", group);
      return NULL;
  }
}

GListModel *
biji_manager_get_notebooks (BijiManager *self)
{
  g_return_val_if_fail (BIJI_IS_MANAGER (self), NULL);

  return G_LIST_MODEL (self->notebooks);
}

BjbItem *
biji_manager_find_notebook (BijiManager *self,
                            const char  *uuid)
{
  GListModel *notebooks;
  guint n_items;

  g_return_val_if_fail (BIJI_IS_MANAGER (self), NULL);
  g_return_val_if_fail (uuid && *uuid, NULL);

  notebooks = biji_manager_get_notebooks (self);
  n_items = g_list_model_get_n_items (notebooks);

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(BjbItem) notebook = NULL;
      const char *item_uuid;

      notebook = g_list_model_get_item (notebooks, i);
      item_uuid = bjb_item_get_uid (notebook);

      if (g_strcmp0 (uuid, item_uuid) == 0)
        return notebook;
    }

  return NULL;
}
