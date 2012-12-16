/* bjb-app-menu.c
 * Copyright (C) Pierre-Yves LUYTEN 2011 <py@luyten.fr>
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

#include "bjb-app-menu.h"
#include "bjb-bijiben.h"
#include "bjb-main-view.h"
#include "bjb-settings.h"
#include "bjb-window-base.h"

/* Callbacks */

void show_about_dialog(GtkApplication *app)
{
  GList * windows = gtk_application_get_windows (app);
  
  gtk_show_about_dialog( g_list_nth_data (windows, 0),
  "program-name", "Bijiben",
  "comments", "Simple noteboook for GNOME",
  "license", "GPLv3",
  "version", "3.7.3",
  "copyright", "Pierre-Yves Luyten 2012",
  NULL,NULL,NULL);

}

static void
new_activated (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  bjb_window_base_new();
}

/* Import external data - TODO : BJB_TYPE_IMPORT_DIALOG.c */

enum {
  IMPORT_DIALOG_ID,
  IMPORT_DIALOG_ICON,
  IMPORT_DIALOG_LABEL,
  IMPORT_DIALOG_N_COLUMNS
};

enum {
  TOMBOY_EXT_SRC,
  GNOTE_EXT_SRC
};

/* First try to find the real icon. FIXME Fallback otherwise
 * in case the user has specific install */
static GdkPixbuf *
get_app_icon (gchar *app_name)
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

static void
on_import_source_activated_cb (GtkTreeView       *t_view,
                               GtkTreePath       *path,
                               GtkTreeViewColumn *column,
                               GtkWidget         *dialog)
{
  GtkTreeIter iter;
  GtkTreeModel *model = gtk_tree_view_get_model (t_view);

  if (gtk_tree_model_get_iter (model, &iter, path))
  {
    GValue value = G_VALUE_INIT;
    gint retval;
    Bijiben *self = BIJIBEN_APPLICATION (g_application_get_default());

    gtk_tree_model_get_value (model, &iter, IMPORT_DIALOG_ID, &value);
    retval = g_value_get_int (&value);
    g_value_unset (&value);

    switch (retval)
    {
      case TOMBOY_EXT_SRC:
        import_notes (self, "tomboy");
        gtk_widget_destroy (dialog);
        dialog = NULL;
        break;

      case GNOTE_EXT_SRC:
        import_notes (self, "gnote");
        gtk_widget_destroy (dialog);
        dialog = NULL;
        break;

      default:
        break;
    }
  }
}

/* TODO : spinner or any notification */
static void
external_activated (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  GtkWidget *dialog, *area, *label, *t_view;
  GtkListStore *store;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GList *windows;
  GdkPixbuf *app_icon;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *column;

  windows = gtk_application_get_windows (GTK_APPLICATION(user_data));

  dialog = gtk_dialog_new_with_buttons ("External Data",
                                        g_list_nth_data (windows, 0),
                                        GTK_DIALOG_MODAL| 
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_CANCEL,
                                        GTK_RESPONSE_CANCEL,
                                        NULL);
  gtk_widget_set_size_request (dialog, 320, 280);

  /* User chooses which folder to import */
  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (area), 8);
  gtk_widget_set_hexpand (area, TRUE);
  gtk_widget_set_vexpand (area, TRUE);

  label = gtk_label_new ("Click on the external data you want to import");
  gtk_box_pack_start (GTK_BOX (area), label, TRUE, FALSE, 2);

  store = gtk_list_store_new (IMPORT_DIALOG_N_COLUMNS,
                              G_TYPE_INT,
                              GDK_TYPE_PIXBUF,
                              G_TYPE_STRING);
  model = GTK_TREE_MODEL (store);

  /* Tomboy */
  app_icon = get_app_icon ("tomboy");
  
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      IMPORT_DIALOG_ID, TOMBOY_EXT_SRC,
                      IMPORT_DIALOG_ICON, app_icon,
                      IMPORT_DIALOG_LABEL, "Tomboy", -1);

  /* Gnote */
  app_icon = get_app_icon ("gnote");

  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, &iter,
                      IMPORT_DIALOG_ID, GNOTE_EXT_SRC,
                      IMPORT_DIALOG_ICON, app_icon,
                      IMPORT_DIALOG_LABEL, "Gnote", -1);

  /* Handle the t_view */
  t_view = gtk_tree_view_new_with_model (model);
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (t_view), FALSE);
  g_signal_connect (t_view, "row-activated",
                    G_CALLBACK (on_import_source_activated_cb), dialog);

  renderer = gtk_cell_renderer_pixbuf_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
                                                     "pixbuf", IMPORT_DIALOG_ICON,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (t_view), column);

  renderer = gtk_cell_renderer_text_new ();
  column = gtk_tree_view_column_new_with_attributes (NULL, renderer,
                                                     "text", IMPORT_DIALOG_LABEL,
                                                     NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (t_view), column);

  gtk_box_pack_start (GTK_BOX (area), t_view, TRUE, FALSE, 2);
  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));

  if (dialog)
  {
    gtk_widget_destroy (dialog);
    dialog = NULL;
  }
}

static void
preferences_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       user_data)
{
  GList * windows = gtk_application_get_windows (GTK_APPLICATION(user_data));
  show_bijiben_settings_window (g_list_nth_data (windows, 0));
}

static void
about_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  show_about_dialog(GTK_APPLICATION(user_data));
}

static void
help_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  GError *error = NULL;
  gtk_show_uri (NULL, "help:bijiben", gtk_get_current_event_time (), &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }
}

static void
quit_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  GApplication *app = user_data;

  g_application_quit (app);
}

/* Menu */

static GActionEntry app_entries[] = {
           { "new", new_activated, NULL, NULL, NULL },
           { "external", external_activated, NULL, NULL, NULL },
           { "preferences", preferences_activated, NULL, NULL, NULL },
           { "about", about_activated, NULL, NULL, NULL },
           { "help", help_activated, NULL, NULL, NULL },
           { "quit", quit_activated, NULL, NULL, NULL },
};


void bjb_app_menu_set(GApplication *application) 
{
  GtkBuilder *builder;

  g_action_map_add_action_entries (G_ACTION_MAP (application),
                                   app_entries,
                                   G_N_ELEMENTS (app_entries),
                                   application);
    
  builder = gtk_builder_new ();
  gtk_builder_add_from_resource (builder, "/org/gnome/bijiben/app-menu.ui", NULL);

  gtk_application_set_app_menu (GTK_APPLICATION (application), 
                                G_MENU_MODEL (gtk_builder_get_object (builder, "app-menu")));
  g_object_unref (builder);
}
