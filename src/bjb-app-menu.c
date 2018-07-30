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
external_activated (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  bjb_app_import_notes (BJB_APPLICATION (g_application_get_default ()));
}

static void
trash_activated (GSimpleAction *action,
                 GVariant      *param,
                 gpointer       user_data)
{
  GtkApplication *app = GTK_APPLICATION (g_application_get_default ());
  GList *windows = gtk_application_get_windows (app);
  BjbController *controller = bjb_window_base_get_controller (BJB_WINDOW_BASE (windows->data));

  bjb_controller_set_group (controller, BIJI_ARCHIVED_ITEMS);
}


static void
preferences_activated (GSimpleAction *action,
                       GVariant      *parameter,
                       gpointer       user_data)
{
  GtkApplication *app = GTK_APPLICATION (g_application_get_default ());
  GList *windows = gtk_application_get_windows (app);

  show_bijiben_settings_window (g_list_nth_data (windows, 0));
}


static void
about_activated (GSimpleAction *action,
                 GVariant      *parameter,
                 gpointer       user_data)
{
  bjb_app_about (BJB_APPLICATION (g_application_get_default ()));
}


void
help_activated (GSimpleAction *action,
                GVariant      *parameter,
                gpointer       user_data)
{
  bjb_app_help (BJB_APPLICATION (g_application_get_default ()));
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
           { "external",    external_activated,    NULL, NULL, NULL },
           { "trash",       trash_activated,       NULL, NULL, NULL },
           { "preferences", preferences_activated, NULL, NULL, NULL },
           { "about",       about_activated,       NULL, NULL, NULL },
           { "help",        help_activated,        NULL, NULL, NULL },
           { "quit",        quit_activated,        NULL, NULL, NULL },
};


void
bjb_app_menu_set(GApplication *application)
{
  g_action_map_add_action_entries (G_ACTION_MAP (application),
                                   app_entries,
                                   G_N_ELEMENTS (app_entries),
                                   application);
}
