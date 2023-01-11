/*
 * biji-notebook.c
 *
 * Copyright (C) Pierre-Yves LUYTEN 2013 <py@luyten.fr>
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

/*
 * in current implementation, one cannot add a notebook
 * to a notebook
 * but tracker would be fine with this
 * given we prevent self-containment.
 */

/*
 * biji_create_notebook_icon
 * is adapted from Photos (photos_utils_create_notebook_icon),
 * which is ported from Documents
 */

#include <math.h>

#include "libbiji.h"
#include "biji-notebook.h"


static void on_collected_item_change (BijiNotebook *self);


struct _BijiNotebook
{
  GObject          parent_instance;

  gchar           *urn;
  gchar           *name;
  gint64           mtime;

  BijiManager     *manager;
  GList           *collected_items;
};

static void biji_notebook_finalize (GObject *object);

G_DEFINE_TYPE (BijiNotebook, biji_notebook, G_TYPE_OBJECT)

/* Properties */
enum {
  PROP_0,
  PROP_URN,
  PROP_NAME,
  PROP_MTIME,
  PROP_MANAGER,
  BIJI_NOTEBOOK_PROPERTIES
};


static GParamSpec *properties[BIJI_NOTEBOOK_PROPERTIES] = { NULL, };

static void
biji_notebook_set_property (GObject      *object,
                              guint         property_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  BijiNotebook *self = BIJI_NOTEBOOK (object);


  switch (property_id)
    {
      case PROP_URN:
        self->urn = g_strdup (g_value_get_string (value));
        break;
      case PROP_NAME:
        self->name = g_strdup (g_value_get_string (value));
        break;
      case PROP_MTIME:
        self->mtime = g_value_get_int64 (value);
        break;
      case PROP_MANAGER:
        self->manager = g_value_get_object (value);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static void
biji_notebook_get_property (GObject    *object,
                              guint       property_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  BijiNotebook *self = BIJI_NOTEBOOK (object);

  switch (property_id)
    {
      case PROP_URN:
        g_value_set_string (value, self->urn);
        break;
      case PROP_NAME:
        g_value_set_string (value, self->name);
        break;
      case PROP_MTIME:
        g_value_set_int64 (value, self->mtime);
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
        break;
    }
}


static void
on_notebook_get_notes_cb (GObject      *object,
                          GAsyncResult *result,
                          gpointer      user_data)
{
  g_autoptr(BijiNotebook) self = user_data;
  GList *notes;

  g_clear_pointer (&self->collected_items, g_list_free);

  notes = biji_tracker_get_notes_with_notebook_finish (BIJI_TRACKER (object), result, NULL);
  self->collected_items = notes;

  /* Connect */
  for (GList *l = self->collected_items; l != NULL; l = l->next)
    {
      g_signal_connect_swapped (l->data, "color-changed",
                                G_CALLBACK (on_collected_item_change), self);

      g_signal_connect_swapped (l->data, "trashed",
                                G_CALLBACK (on_collected_item_change), self);
    }
}

static void
on_collected_item_change (BijiNotebook *self)
{
  BijiManager *manager;
  GList *l;

  manager = self->manager;

  /* Disconnected any handler */
  for (l = self->collected_items; l != NULL; l = l->next)
  {
    g_signal_handlers_disconnect_by_func (l->data, on_collected_item_change, self);
  }

  /* Then re-process the whole stuff */
  biji_tracker_get_notes_with_notebook_async (biji_manager_get_tracker (manager),
                                              self->name,
                                              on_notebook_get_notes_cb,
                                              g_object_ref (self));
}

void
biji_notebook_refresh (BijiNotebook *self)
{
  on_collected_item_change (self);
}

static void
biji_notebook_constructed (GObject *obj)
{
  BijiNotebook *self = BIJI_NOTEBOOK (obj);
  BijiManager *manager;


  manager = self->manager;

  biji_tracker_get_notes_with_notebook_async (biji_manager_get_tracker (manager),
                                              self->name,
                                              on_notebook_get_notes_cb,
                                              g_object_ref (self));
}

static void
biji_notebook_class_init (BijiNotebookClass *klass)
{
  GObjectClass *g_object_class;

  g_object_class = G_OBJECT_CLASS (klass);

  g_object_class->constructed = biji_notebook_constructed;
  g_object_class->finalize = biji_notebook_finalize;
  g_object_class->set_property = biji_notebook_set_property;
  g_object_class->get_property = biji_notebook_get_property;

  properties[PROP_URN] =
    g_param_spec_string ("urn",
                         "Collection URN",
                         "Collection URN as in Tracker",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  properties[PROP_NAME] =
    g_param_spec_string ("name",
                         "Collection label",
                         "The tag as a string",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  properties[PROP_MTIME] =
    g_param_spec_int64  ("mtime",
                         "Modification time",
                         "Last modified time",
                         G_MININT64, G_MAXINT64, 0,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  properties[PROP_MANAGER] =
    g_param_spec_object("manager",
                        "Note Manager",
                        "The Note Manager controlling this item",
                        BIJI_TYPE_MANAGER,
                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (g_object_class, BIJI_NOTEBOOK_PROPERTIES, properties);
}


static void
biji_notebook_finalize (GObject *object)
{
  BijiNotebook *self;
  g_return_if_fail (BIJI_IS_NOTEBOOK (object));

  self = BIJI_NOTEBOOK (object);
  g_free (self->name);
  g_free (self->urn);

  G_OBJECT_CLASS (biji_notebook_parent_class)->finalize (object);
}


static void
biji_notebook_init (BijiNotebook *self)
{
}


BijiNotebook *
biji_notebook_new (GObject *manager, gchar *urn, gchar *name, gint64 mtime)
{
  return g_object_new (BIJI_TYPE_NOTEBOOK,
                       "manager", manager,
                       "name",      name,
                       "urn",       urn,
                       "mtime",     mtime,
                       NULL);
}

const char *
biji_notebook_get_title (BijiNotebook *self)
{
  g_return_val_if_fail (BIJI_IS_NOTEBOOK (self), NULL);

  return self->name;
}

const char *
biji_notebook_get_uuid (BijiNotebook *self)
{
  g_return_val_if_fail (BIJI_IS_NOTEBOOK (self), NULL);

  return self->urn;
}
