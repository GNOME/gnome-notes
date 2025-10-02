/*
 * bjb-import-dialog.c
 * Copyright (C) Pierre-Yves LUYTEN 2013 <py@luyten.fr>
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.*/


/* TODO
 *
 * - better label for dialog (pango for head label)
 * - maybe align differently
 * - we might also trigger spinner view when import runs
 */


#include "config.h"

#include <glib/gi18n.h>

#include "bjb-import-dialog.h"


#define IMPORT_EMBLEM_WIDTH 32
#define IMPORT_EMBLEM_HEIGHT 32

typedef enum
{
  IMPORT_DIALOG_ITEM_TOMBOY = 1 << 0,
  IMPORT_DIALOG_ITEM_GNOTE  = 1 << 1,
  IMPORT_DIALOG_ITEM_CUSTOM = 1 << 2,
} ImportDialogItem;


struct _BjbImportDialog
{
  GtkDialog parent_instance;

  GtkWidget *import_button;

  GtkWidget *gnote_label;
  GtkWidget *gnote_stack;
  GtkWidget *gnote_import;

  GtkWidget *tomboy_label;
  GtkWidget *tomboy_stack;
  GtkWidget *tomboy_import;

  GtkWidget *custom_label;
  GtkWidget *custom_stack;
  GtkWidget *custom_import;

  gchar *custom_location;
  ImportDialogItem items;
};


G_DEFINE_TYPE (BjbImportDialog, bjb_import_dialog, GTK_TYPE_DIALOG)


static void
toggle_selection (BjbImportDialog  *self,
                  GtkWidget        *widget,
                  ImportDialogItem  item)
{
  /* If custom import clicked,  check if location is set */
  if (item != IMPORT_DIALOG_ITEM_CUSTOM || self->custom_location)
    self->items ^= item;
  else /* ie, custom import with no location set */
    self->items &= self->items ^ IMPORT_DIALOG_ITEM_CUSTOM;

  if (self->items & item)
    gtk_stack_set_visible_child_name (GTK_STACK (widget), "tick");
  else
    gtk_stack_set_visible_child_name (GTK_STACK (widget), "empty");

  if (self->items)
    gtk_widget_set_sensitive (self->import_button, TRUE);
  else
    gtk_widget_set_sensitive (self->import_button, FALSE);
}

static void
on_row_activated_cb    (GtkListBox    *list_box,
                        GtkListBoxRow *row,
                        gpointer       user_data)
{
  const gchar *widget_name;
  BjbImportDialog *self = user_data;

  widget_name = gtk_widget_get_name (gtk_list_box_row_get_child (row));

  if (g_strcmp0 (widget_name, "custom") == 0)
    toggle_selection (self, self->custom_stack, IMPORT_DIALOG_ITEM_CUSTOM);
  else if (g_strcmp0 (widget_name, "gnote") == 0)
    toggle_selection (self, self->gnote_stack, IMPORT_DIALOG_ITEM_GNOTE);
  else if (g_strcmp0 (widget_name, "tomboy") == 0)
    toggle_selection (self, self->tomboy_stack, IMPORT_DIALOG_ITEM_TOMBOY);
}

static void
on_file_set_cb (GtkWidget       *chooser,
                BjbImportDialog *self)
{
  g_autoptr(GFile) file = NULL;

  g_assert (GTK_IS_FILE_CHOOSER (chooser));

  file = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (chooser));
  g_clear_pointer (&self->custom_location, g_free);
  self->custom_location = g_file_get_path (file);

  toggle_selection (self, self->custom_stack, IMPORT_DIALOG_ITEM_CUSTOM);

  if (self->custom_location)
    gtk_widget_set_sensitive (self->custom_label, TRUE);
  else
    gtk_widget_set_sensitive (self->custom_label, FALSE);
}

static void
bjb_import_dialog_constructed (GObject *obj)
{
  g_autofree gchar *path = NULL;
  BjbImportDialog *self = BJB_IMPORT_DIALOG (obj);

  G_OBJECT_CLASS(bjb_import_dialog_parent_class)->constructed(obj);

  /*
   * Tomboy and Gnote ~/.local/share are conditional.
   * These are shown only if their data directories are present.
   */

  path = g_build_filename (g_get_user_data_dir (), "tomboy", NULL);

  if (g_file_test (path, G_FILE_TEST_EXISTS))
    gtk_widget_set_visible (self->tomboy_import, TRUE);

  g_free (path);
  path = g_build_filename (g_get_user_data_dir (), "gnote", NULL);

  if (g_file_test (path, G_FILE_TEST_EXISTS))
    gtk_widget_set_visible (self->gnote_import, TRUE);
}

static void
bjb_import_dialog_class_init (BjbImportDialogClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = bjb_import_dialog_constructed;

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/import-dialog.ui");
  gtk_widget_class_bind_template_child (widget_class, BjbImportDialog, import_button);
  gtk_widget_class_bind_template_child (widget_class, BjbImportDialog, gnote_label);
  gtk_widget_class_bind_template_child (widget_class, BjbImportDialog, gnote_stack);
  gtk_widget_class_bind_template_child (widget_class, BjbImportDialog, gnote_import);
  gtk_widget_class_bind_template_child (widget_class, BjbImportDialog, tomboy_label);
  gtk_widget_class_bind_template_child (widget_class, BjbImportDialog, tomboy_stack);
  gtk_widget_class_bind_template_child (widget_class, BjbImportDialog, tomboy_import);
  gtk_widget_class_bind_template_child (widget_class, BjbImportDialog, custom_label);
  gtk_widget_class_bind_template_child (widget_class, BjbImportDialog, custom_stack);
  gtk_widget_class_bind_template_child (widget_class, BjbImportDialog, custom_import);

  gtk_widget_class_bind_template_callback (widget_class, on_row_activated_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_file_set_cb);
}


static void
bjb_import_dialog_init (BjbImportDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


GtkDialog *
bjb_import_dialog_new (GtkApplication *bijiben)
{
  GtkWindow *window;

  window = gtk_application_get_active_window (bijiben);
  return g_object_new (BJB_TYPE_IMPORT_DIALOG,
                       "use-header-bar", TRUE,
                       "transient-for", window,
                       NULL);
}


GList *
bjb_import_dialog_get_paths (BjbImportDialog *self)
{
  GList *list = NULL;

  if (self->items & IMPORT_DIALOG_ITEM_GNOTE)
    list = g_list_prepend (list, g_build_filename (g_get_user_data_dir (),
                                                   "gnote", NULL));
  if (self->items & IMPORT_DIALOG_ITEM_TOMBOY)
    list = g_list_prepend (list, g_build_filename (g_get_user_data_dir (),
                                                   "tomboy", NULL));
  if (self->items & IMPORT_DIALOG_ITEM_CUSTOM &&
      self->custom_location &&
      g_list_find_custom (list, self->custom_location, g_str_equal) == NULL)
    list = g_list_prepend (list, self->custom_location);

  return list;
}
