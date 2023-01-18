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

struct _BjbController
{
  GObject parent_instance;

  /* needle, notebook and group define what the controller shows */
  BijiManager    *manager;
  BijiItemsGroup  group;

  GListStore          *list_of_notes;
  GtkFlattenListModel *flatten_notes;
  GtkSortListModel    *sorted_notes;
};


G_DEFINE_TYPE (BjbController, bjb_controller, G_TYPE_OBJECT)

/* GObject */

static int
controller_sort_notes (gconstpointer a,
                       gconstpointer b,
                       gpointer     user_data)
{
  gint64 time_a, time_b;

  time_a = bjb_item_get_mtime ((gpointer) a);
  time_b = bjb_item_get_mtime ((gpointer) b);

  return time_b - time_a;
}

static void
bjb_controller_init (BjbController *self)
{
  GtkSorter *sorter;

  self->group = BIJI_LIVING_ITEMS;

  self->list_of_notes = g_list_store_new (G_TYPE_LIST_MODEL);
  self->flatten_notes = gtk_flatten_list_model_new (BIJI_TYPE_NOTE_OBJ, G_LIST_MODEL (self->list_of_notes));

  sorter = gtk_custom_sorter_new (controller_sort_notes, NULL, NULL);
  self->sorted_notes = gtk_sort_list_model_new (G_LIST_MODEL (self->flatten_notes), sorter);
}

static void
bjb_controller_class_init (BjbControllerClass *klass)
{
}

BjbController *
bjb_controller_new (BijiManager *manager)
{
  BjbController *self;
  GListModel *notes;

  self = g_object_new ( BJB_TYPE_CONTROLLER,
                       NULL);

  /*
   * Add only active notes, which is the default.  Archived
   * notes shall be added when the user filters notes
   */
  self->manager = manager;
  notes = biji_manager_get_notes (manager, BIJI_LIVING_ITEMS);
  g_list_store_append (self->list_of_notes, notes);

  return self;
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
