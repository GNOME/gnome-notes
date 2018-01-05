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

#include "config.h"

#include <glib/gi18n.h>

#include "bjb-app-menu.h"
#include "bjb-application.h"
#include "bjb-import-dialog.h"
#include "bjb-main-view.h"
#include "bjb-settings.h"
#include "bjb-window-base.h"

/* Callbacks */
static void
show_about_dialog (GtkApplication *app)
{
  GList * windows = gtk_application_get_windows (app);

  const gchar *authors[] = {
    "Pierre-Yves Luyten <py@luyten.fr>",
    NULL
  };

  const gchar *artists[] = {
    "William Jon McCann <jmccann@redhat.com>",
    NULL
  };

  gtk_show_about_dialog (g_list_nth_data (windows, 0),
                         "program-name", _("Notes"),
                         "comments", _("Simple notebook for GNOME"),
                         "license-type", GTK_LICENSE_GPL_3_0,
                         "version", VERSION,
                         "copyright", "Copyright © 2013 Pierre-Yves Luyten",
                         "authors", authors,
                         "artists", artists,
                         "translator-credits", _("translator-credits"),
                         "website", "https://wiki.gnome.org/Apps/Bijiben",
                         "logo-icon-name", "org.gnome.bijiben",
                         NULL);
}


static void
external_activated (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  GtkDialog *dialog;
  gint result;

  dialog = bjb_import_dialog_new (user_data);
  result = gtk_dialog_run (dialog);

  if (result == GTK_RESPONSE_OK)
  {
    GList *locations, *l;

    locations = bjb_import_dialog_get_paths (BJB_IMPORT_DIALOG (dialog));
    for (l=locations; l!= NULL; l=l->next)
    {
      g_autofree gchar *uri = g_filename_to_uri (l->data, NULL, NULL);
      bijiben_import_notes (user_data, uri);
    }

    g_list_free_full (locations, g_free);
  }

  if (dialog)
    g_clear_pointer (&dialog, gtk_widget_destroy);
}


static void
trash_activated (GSimpleAction *action,
                 GVariant      *param,
                 gpointer       user_data)
{
  GList *l;

  l = gtk_application_get_windows (GTK_APPLICATION (user_data));
  bjb_controller_set_group (
    bjb_window_base_get_controller (BJB_WINDOW_BASE (l->data)),
    BIJI_ARCHIVED_ITEMS);
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


void
help_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  g_autoptr(GError) error = NULL;

  gtk_show_uri_on_window (gtk_application_get_active_window (user_data),
                          "help:bijiben",
                          GDK_CURRENT_TIME,
                          &error);

  if (error)
    g_warning ("%s", error->message);
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
           { "external", external_activated, NULL, NULL, NULL },
           { "trash", trash_activated, NULL, NULL, NULL },
           { "preferences", preferences_activated, NULL, NULL, NULL },
           { "about", about_activated, NULL, NULL, NULL },
           { "help", help_activated, NULL, NULL, NULL },
           { "quit", quit_activated, NULL, NULL, NULL },
};


void bjb_app_menu_set(GApplication *application)
{
  g_action_map_add_action_entries (G_ACTION_MAP (application),
                                   app_entries,
                                   G_N_ELEMENTS (app_entries),
                                   application);
}
