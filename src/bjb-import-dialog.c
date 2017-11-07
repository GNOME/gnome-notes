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
 * - maybe align diffrently
 * - we check if apps are installed. rather check for notes...
 * - we might also trigger spinner view when import runs
 */


#include "config.h"

#include <glib/gi18n.h>

#include "bjb-import-dialog.h"


#define IMPORT_EMBLEM_WIDTH 32
#define IMPORT_EMBLEM_HEIGHT 32


typedef struct
{
  GtkWidget     *overlay;
  GtkWidget     *widget;
  GtkWidget     *toggle;

  gboolean       selected;
  gchar         *location;

} ImportDialogChild;


struct _BjbImportDialog
{
  GtkDialog             parent_instance;

  GtkListBox           *box;


  /* Select spec. foder */

  GtkWidget            *cust_box;
  GtkWidget            *browser;
  ImportDialogChild    *custom;

  /* paths and Confirm button */

  GHashTable           *locations;
  GtkWidget            *go_go_go;
};


G_DEFINE_TYPE (BjbImportDialog, bjb_import_dialog, GTK_TYPE_DIALOG)



static ImportDialogChild *
import_dialog_child_new (void)
{
  ImportDialogChild *retval = g_slice_new (ImportDialogChild);

  retval->overlay = NULL;
  retval->widget = NULL;
  retval->toggle = NULL;

  retval->selected = FALSE;
  retval->location = NULL;

  return retval;
}


static void
import_dialog_child_free (ImportDialogChild *child)
{
  g_free (child->location);
  g_slice_free (ImportDialogChild, child);
}


/* First try to find the real icon
 * might use app_info here */

static GdkPixbuf *
get_app_icon (const gchar *app_name)
{
  gint i;
  GdkPixbuf *retval= NULL;
  const gchar * const *paths = g_get_system_data_dirs ();
  gchar *app_svg = g_strdup_printf ("%s.svg", app_name);

  for (i=0; paths[i] != NULL; i++)
  {
    gchar *path;
    GError *error = NULL;

    path = g_build_filename (paths[i], "icons", "hicolor",
                             "scalable", "apps", app_svg, NULL);
    retval = gdk_pixbuf_new_from_file (path, &error);
    g_free (path);

    if (!error && GDK_IS_PIXBUF (retval))
      break;

    else
      retval = NULL;
  }

  g_free (app_svg);
  return retval;
}


static inline GQuark
application_quark (void)
{
  static GQuark quark;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("bjb-application");

  return quark;
}


static GtkWidget *
child_toggle_new (void)
{
  GtkWidget *w;

  w = gtk_image_new_from_icon_name ("object-select-symbolic", GTK_ICON_SIZE_INVALID);
  gtk_image_set_pixel_size (GTK_IMAGE (w), 38);
  gtk_widget_show (w);
  return w;
}


/* row_activated
 * When an item is activated
 * Check for the location
 * Show or delete the visible toggle
 * Add or remove location from import paths */

static void
toggle_widget (BjbImportDialog *self,
               GObject         *widget)
{
  ImportDialogChild *child;
  GtkStyleContext *context;
  GList *keys;

  child = g_object_get_qdata (widget, application_quark ());

  if (!child->selected && child->location == NULL)
    return;

  child->selected = !child->selected;


  if (child->selected)
  {
    child->toggle = child_toggle_new ();
    gtk_overlay_add_overlay (GTK_OVERLAY (child->overlay), child->toggle);

    g_hash_table_add (self->locations, child->location);
  }

  else
  {
    if (child->toggle && GTK_IS_WIDGET (child->toggle))
      gtk_widget_destroy (child->toggle);

    g_hash_table_remove (self->locations, child->location);
  }

  context = gtk_widget_get_style_context (self->go_go_go);
  keys = g_hash_table_get_keys (self->locations);

  if (keys)
  {
    gtk_style_context_add_class (context, "suggested-action");
    gtk_widget_set_sensitive (self->go_go_go, TRUE);
    g_list_free (keys);
  }

  else
  {
    gtk_style_context_remove_class (context, "suggested-action");
  }

  gtk_widget_reset_style (self->go_go_go);
}


static void
on_row_activated_cb    (GtkListBox    *list_box,
                        GtkListBoxRow *row,
                        gpointer       user_data)
{
  toggle_widget (user_data, G_OBJECT (gtk_bin_get_child (GTK_BIN (row))));
}


static void
header_func (GtkListBoxRow *row,
             GtkListBoxRow *before_row,
             gpointer user_data)
{
  if (before_row != NULL && gtk_list_box_row_get_header (row) != NULL)
    gtk_list_box_row_set_header (row, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

  else
    gtk_list_box_row_set_header (row, NULL);
}


static void
on_file_set_cb (GtkWidget *chooser,
                BjbImportDialog *dialog)
{
  gchar *location;

  /* Remove the former */

  if (dialog->custom->location)
  {
    g_hash_table_remove (dialog->locations,
                         dialog->custom->location);
    g_clear_pointer (&dialog->custom->location, g_free);
  }


  /* Handle the new : force toggle */

  location = dialog->custom->location =
    gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

  if (location)
  {
    gtk_widget_set_sensitive (dialog->cust_box, TRUE);

    dialog->custom->selected = FALSE;
    if (GTK_IS_WIDGET (dialog->custom->toggle))
      gtk_widget_destroy (dialog->custom->toggle);

    toggle_widget (dialog, G_OBJECT (dialog->custom->overlay));
    return;
  }

  gtk_widget_set_sensitive (dialog->cust_box, FALSE);
}


static ImportDialogChild *
add_custom (BjbImportDialog *self)
{
  GtkWidget *box, *w;
  ImportDialogChild *child;

  child = import_dialog_child_new ();
  child->widget = box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  child->overlay = gtk_overlay_new ();
  gtk_container_add (GTK_CONTAINER (child->overlay), child->widget);

  /* Left group */
  self->cust_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box), self->cust_box, TRUE, FALSE, 0);

  w = gtk_image_new_from_icon_name ("folder", GTK_ICON_SIZE_INVALID);
  gtk_image_set_pixel_size (GTK_IMAGE (w), 48);
  gtk_widget_set_margin_start (w, 12);
  gtk_widget_set_margin_end (w, 12);
  gtk_container_add (GTK_CONTAINER (self->cust_box), w);


  w = gtk_label_new (_("Custom Location"));
  gtk_widget_set_margin_end (w, 180);
  gtk_container_add (GTK_CONTAINER (self->cust_box), w);
  gtk_widget_set_valign (w, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (self->cust_box), w, TRUE, FALSE, 0);

  gtk_widget_set_sensitive (self->cust_box, FALSE);

  /* Right group */

  self->browser =
    gtk_file_chooser_button_new ("", GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
  gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (self->browser),
                                       g_get_user_data_dir ());
  gtk_box_pack_start (GTK_BOX (box), self->browser, TRUE, FALSE, 0);

  g_signal_connect (self->browser, "current-folder-changed",
                    G_CALLBACK (on_file_set_cb), self);

  g_object_set_qdata_full (G_OBJECT (child->overlay), application_quark (),
                           child, (GDestroyNotify) import_dialog_child_free);

  return child;
}


static ImportDialogChild *
add_application (const gchar *app,
                 const gchar *visible_label,
                 gchar *location)
{
  GtkWidget *box, *w;
  GdkPixbuf *pix;
  ImportDialogChild *child;

  child = import_dialog_child_new ();
  child->widget = box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  child->overlay = gtk_overlay_new ();
  child->location = location;
  gtk_container_add (GTK_CONTAINER (child->overlay), child->widget);

  if (location)
    g_object_set_qdata_full (G_OBJECT (child->overlay), application_quark (),
                             child, (GDestroyNotify) import_dialog_child_free);

  /* If the icon is not found, consider the app is not installed
   * to avoid the dialog showing the feature */

  pix = get_app_icon (app);

  if (pix == NULL)
  {
    g_object_ref_sink (box);
    import_dialog_child_free (child);
    return NULL;
  }

  w = gtk_image_new_from_pixbuf (pix);
  gtk_widget_set_margin_start (w, 12);
  gtk_container_add (GTK_CONTAINER (box), w);


  w = gtk_label_new (visible_label);
  gtk_widget_set_margin_end (w, 180);
  gtk_container_add (GTK_CONTAINER (box), w);
  gtk_widget_set_valign (w, GTK_ALIGN_CENTER);
  gtk_box_pack_end (GTK_BOX (box), w, FALSE, FALSE, 0);

  gtk_widget_show_all (box);
  return child;
}


static void
bjb_import_dialog_constructed (GObject *obj)
{
  GtkWidget *area, *label_box, *label, *frame;
  gchar *path;
  GtkApplication *app;
  ImportDialogChild *child;
  BjbImportDialog        *self    = BJB_IMPORT_DIALOG (obj);
  GtkDialog              *dialog  = GTK_DIALOG (obj);
  GtkWindow              *win     = GTK_WINDOW (obj);

  G_OBJECT_CLASS(bjb_import_dialog_parent_class)->constructed(obj);

  /* Don't finalize locations with HashTable
   * They belong to qdata in gtkwidgets */

  self->locations = g_hash_table_new (g_str_hash, g_str_equal);

  app = GTK_APPLICATION (g_application_get_default ());

  gtk_window_set_transient_for (win, gtk_application_get_active_window (app));
  gtk_window_set_title (win, _("Import Notes"));
  gtk_window_set_modal (win, TRUE);

  gtk_header_bar_set_show_close_button (GTK_HEADER_BAR
    (gtk_dialog_get_header_bar (GTK_DIALOG (self))), TRUE);


  self->go_go_go = gtk_dialog_add_button (dialog, _("Import"), GTK_RESPONSE_OK);
  gtk_widget_set_sensitive (self->go_go_go, FALSE);

  /* Dialog Label */
  area = gtk_dialog_get_content_area (dialog);
  gtk_widget_set_margin_start (area, 12);
  gtk_container_set_border_width (GTK_CONTAINER (area), 2);
  gtk_widget_set_hexpand (area, TRUE);
  gtk_widget_set_vexpand (area, TRUE);

  label_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  label = gtk_label_new (_("Select import location"));
  gtk_widget_set_halign (label, GTK_ALIGN_START);
  gtk_box_pack_start (GTK_BOX (label_box), label, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (area), label_box, TRUE, FALSE, 12);

  /* Dialog locations to import */

  frame = gtk_frame_new (NULL);
  self->box = GTK_LIST_BOX (gtk_list_box_new ());
  gtk_list_box_set_selection_mode (self->box, GTK_SELECTION_NONE);
  gtk_list_box_set_activate_on_single_click (self->box, TRUE);
  gtk_list_box_set_header_func (self->box, (GtkListBoxUpdateHeaderFunc) header_func, NULL, NULL);
  g_signal_connect (self->box, "row-activated", G_CALLBACK (on_row_activated_cb), self);
  gtk_box_pack_start (GTK_BOX (area), GTK_WIDGET (frame) , TRUE, FALSE, 6);

  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (self->box));

  /* Tomboy Gnote ~/.local/share are conditional
   * these are only packed if app is installed     */

  path = g_build_filename (g_get_user_data_dir (), "tomboy", NULL);
  child = add_application ("tomboy", _("Tomboy application"), path);
  if (child)
    gtk_container_add (GTK_CONTAINER (self->box), child->overlay);


  path = g_build_filename (g_get_user_data_dir (), "gnote", NULL);
  child = add_application ("gnote", _("Gnote application"), path);
  if (child)
    gtk_container_add (GTK_CONTAINER (self->box), child->overlay);


  /* User decides path */

  child = self->custom = add_custom (self);
  gtk_container_add (GTK_CONTAINER (self->box), child->overlay);


  gtk_widget_show_all (GTK_WIDGET (self));
}

static void
bjb_import_dialog_finalize (GObject *o)
{
  BjbImportDialog *self = BJB_IMPORT_DIALOG (o);
  g_hash_table_destroy (self->locations);

  G_OBJECT_CLASS (bjb_import_dialog_parent_class)->finalize (o);
}


static void
bjb_import_dialog_class_init (BjbImportDialogClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = bjb_import_dialog_constructed;
  object_class->finalize = bjb_import_dialog_finalize;
}


static void
bjb_import_dialog_init (BjbImportDialog *self)
{
}


GtkDialog *
bjb_import_dialog_new (GtkApplication *bijiben)
{
  return g_object_new (BJB_TYPE_IMPORT_DIALOG,
		       "use-header-bar", TRUE,
                       NULL);
}


GList *
bjb_import_dialog_get_paths (BjbImportDialog *self)
{
  return g_hash_table_get_values (self->locations);
}
