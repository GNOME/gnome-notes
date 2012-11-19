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
  "version", "3.7.2",
  "copyright", "Pierre-Yves Luyten 2012",
  NULL,NULL,NULL);

}

void 
summary()
{
  GError *error = NULL;
  gtk_show_uri (NULL, "ghelp:bijiben", gtk_get_current_event_time (), &error);

  if (error)
  {
    g_warning (error->message);
    g_error_free (error);
  }
}

static void
new_activated (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
  bjb_window_base_new();
}

static void
import_gnote_notes (Bijiben *self)
{
  import_notes (self, "gnote");
}

static void
import_tomboy_notes (Bijiben *self)
{
  import_notes (self, "tomboy");
}

static void
external_activated (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  GtkWidget *dialog, *area, *hbox, *button;
  GList *windows;
  Bijiben *app = BIJIBEN_APPLICATION (user_data);

  windows = gtk_application_get_windows (GTK_APPLICATION(user_data));
  
  dialog = gtk_dialog_new_with_buttons ("External data",
                                        g_list_nth_data (windows, 0),
                                        GTK_DIALOG_MODAL| 
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        GTK_STOCK_OK,
                                        GTK_RESPONSE_OK,
                                        NULL);

  /* User chooses which folder to import */
  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (area), 8);
  gtk_widget_set_hexpand (area, TRUE);
  gtk_widget_set_vexpand (area, TRUE);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 20);
  gtk_box_pack_start (GTK_BOX (area), hbox, TRUE, FALSE, 2);
  button = gtk_button_new_with_label ("Import Tomboy Notes");
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (import_tomboy_notes), app);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 2);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 20);
  gtk_box_pack_start (GTK_BOX (area), hbox, TRUE, FALSE, 2);
  button = gtk_button_new_with_label ("Import GNote Notes");
  g_signal_connect_swapped (button, "clicked",
                            G_CALLBACK (import_gnote_notes), app);
  gtk_box_pack_start (GTK_BOX (hbox), button, TRUE, FALSE, 2);

  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
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
  gtk_show_uri (NULL, "ghelp:bijiben", gtk_get_current_event_time (), &error);

  if (error)
  {
    g_warning (error->message);
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
