/*
 * bjb-controller.c
 * Copyright (C) Pierre-Yves Luyten 2012, 2013 <py@luyten.fr>
 * Copyright 2017 Mohammed Sadiq <sadiq@sadiqpk.org>
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
 * bjb-controler is window-wide.
 * mainly useful for BjbMainView,
 * it controls the window behaviour.
 */

#include <contrib/gtk.h>

#include "bjb-application.h"
#include "bjb-controller.h"
#include "bjb-window.h"

/*
 * The start-up number of items to show,
 * and its incrementation
 */

#define BJB_ITEMS_SLICE 48


struct _BjbController
{
  GObject parent_instance;

  /* needle, notebook and group define what the controller shows */
  BijiManager    *manager;
  BijiItemsGroup  group;

  BjbWindow      *window;
  BjbSettings    *settings;

  GListStore          *list_of_notes;
  GtkFlattenListModel *flatten_notes;
  GtkSortListModel    *sorted_notes;
};


enum {
  PROP_0,
  PROP_BOOK,
  PROP_WINDOW,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (BjbController, bjb_controller, G_TYPE_OBJECT)

/* GObject */

static int
controller_sort_notes (gconstpointer a,
                       gconstpointer b,
                       gpointer     user_data)
{
  gint64 time_a, time_b;

  time_a = biji_note_obj_get_mtime ((gpointer) a);
  time_b = biji_note_obj_get_mtime ((gpointer) b);

  return time_b - time_a;
}

static void
bjb_controller_init (BjbController *self)
{
  self->group = BIJI_LIVING_ITEMS;
}

static void
bjb_controller_get_property (GObject  *object,
               guint     property_id,
               GValue   *value,
               GParamSpec *pspec)
{
  BjbController *self = BJB_CONTROLLER (object);

  switch (property_id)
  {
  case PROP_BOOK:
    g_value_set_object (value, self->manager);
    break;
  case PROP_WINDOW:
    g_value_set_object (value, self->window);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bjb_controller_set_property (GObject  *object,
               guint     property_id,
               const GValue *value,
               GParamSpec *pspec)
{
  BjbController *self = BJB_CONTROLLER (object);

  switch (property_id)
  {
  case PROP_BOOK:
    bjb_controller_set_manager(self,g_value_get_object(value));
    break;
  case PROP_WINDOW:
    self->window = g_value_get_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bjb_controller_constructed (GObject *obj)
{
  BjbController *self = BJB_CONTROLLER (obj);
  GListModel *notes;
  GtkSorter *sorter;

  G_OBJECT_CLASS(bjb_controller_parent_class)->constructed(obj);

  self->settings = bjb_app_get_settings (g_application_get_default ());

  self->list_of_notes = g_list_store_new (G_TYPE_LIST_MODEL);
  self->flatten_notes = gtk_flatten_list_model_new (BIJI_TYPE_NOTE_OBJ, G_LIST_MODEL (self->list_of_notes));

  sorter = gtk_custom_sorter_new (controller_sort_notes, NULL, NULL);
  self->sorted_notes = gtk_sort_list_model_new (G_LIST_MODEL (self->flatten_notes), sorter);

  /*
  * Add only active notes, which is the default.  Archived
   * notes shall be added when the user filters notes
   */
  notes = biji_manager_get_notes (self->manager, BIJI_LIVING_ITEMS);
  g_list_store_append (self->list_of_notes, notes);
}

static void
bjb_controller_class_init (BjbControllerClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bjb_controller_get_property;
  object_class->set_property = bjb_controller_set_property;
  object_class->constructed = bjb_controller_constructed;

  properties[PROP_BOOK] = g_param_spec_object ("manager",
                                               "Book",
                                               "The BijiManager",
                                               BIJI_TYPE_MANAGER,
                                               G_PARAM_READWRITE |
                                               G_PARAM_CONSTRUCT |
                                               G_PARAM_STATIC_STRINGS);

  properties[PROP_WINDOW] = g_param_spec_object ("window",
                                                 "GtkWindow",
                                                 "BjbWindow",
                                                 BJB_TYPE_WINDOW,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT |
                                                 G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

}

BjbController *
bjb_controller_new (BijiManager  *manager,
                    GtkWindow     *window)
{
  return g_object_new ( BJB_TYPE_CONTROLLER,
              "manager", manager,
              "window", window,
              NULL);
}

void
bjb_controller_set_manager (BjbController *self, BijiManager  *manager )
{
  self->manager = manager;
}

GListModel *
bjb_controller_get_notes (BjbController *self)
{
  g_return_val_if_fail (BJB_IS_CONTROLLER (self), NULL);

  return G_LIST_MODEL (self->sorted_notes);
}

BijiItemsGroup
bjb_controller_get_group (BjbController *self)
{
  return self->group;
}



void
bjb_controller_set_group (BjbController   *self,
                          BijiItemsGroup   group)
{
  GListModel *notes;

  if (self->group == group)
    return;

  self->group = group;

  g_list_store_remove_all (self->list_of_notes);

  notes = biji_manager_get_notes (self->manager, group);
  g_list_store_append (self->list_of_notes, notes);
}
