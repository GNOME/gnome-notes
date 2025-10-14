/*
 * bjb-bijiben.c
 * Copyright (C) 2011 Pierre-Yves LUYTEN <py@luyten.fr>
 * Copyright (C) 2017 Iñigo Martínez <inigomartinez@gmail.com>
 * Copyright (C) 2017 Mohammed Sadiq <sadiq@sadiqpk.org>
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

#define G_LOG_DOMAIN "bjb-application"

#include "config.h"
#include "version.h"

#include <glib/gi18n.h>

#include "bjb-application.h"
#include "bjb-settings.h"
#include "bjb-note-view.h"
#include "bjb-window.h"
#include "bjb-log.h"


struct _BjbApplication
{
  AdwApplication  parent_instance;

  BjbSettings    *settings;
  gboolean        new_note;
};

G_DEFINE_TYPE (BjbApplication, bjb_application, ADW_TYPE_APPLICATION)

static gboolean
cmd_verbose_cb (const char  *option_name,
                const char  *value,
                gpointer     data,
                GError     **error)
{
  bjb_log_increase_verbosity ();

  return TRUE;
}

static void
bijiben_activate (GApplication *app)
{
  GtkWindow *window;

  window = gtk_application_get_active_window (GTK_APPLICATION (app));

  if (!window)
    {
      window = GTK_WINDOW (bjb_window_new ());

      if (g_strcmp0 (PROFILE, "") != 0)
        gtk_widget_add_css_class (GTK_WIDGET (window), "devel");
    }

  gtk_window_present (window);
}

static void
bjb_application_init (BjbApplication *self)
{
  GApplication *app = G_APPLICATION (self);
  const GOptionEntry cmd_options[] = {
    { "verbose", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, cmd_verbose_cb,
      N_("Show verbose logs"), NULL},
    { "new-note", 0, 0, G_OPTION_ARG_NONE, &self->new_note,
      N_("Create a new note"), NULL},
    { NULL }
  };

  gtk_window_set_default_icon_name ("org.gnome.Notes");
  g_application_add_main_option_entries (app, cmd_options);
  g_application_set_option_context_parameter_string (app, _("[FILE…]"));
  g_application_set_option_context_summary (app, _("Take notes and export them everywhere."));

  g_application_set_version (app, PACKAGE_VCS_VERSION);
}

static void
bijiben_startup (GApplication *application)
{
  BjbApplication *self;
  g_autofree gchar *storage_path = NULL;
  g_autoptr(GAction) text_size = NULL;
  g_autoptr(GFile) storage = NULL;
  g_autoptr(GError) error = NULL;

  G_APPLICATION_CLASS (bjb_application_parent_class)->startup (application);
  self = BJB_APPLICATION (application);

  self->settings = bjb_settings_new ();

  gtk_window_set_default_icon_name ("org.gnome.Notes");

  storage_path = g_build_filename (g_get_user_data_dir (), "bijiben", NULL);
  storage = g_file_new_for_path (storage_path);

  // Create the bijiben dir to ensure.
  g_file_make_directory (storage, NULL, &error);

  if (error && !g_error_matches (error, G_IO_ERROR, G_IO_ERROR_EXISTS))
    g_warning ("%s", error->message);

  text_size = bjb_settings_get_text_size_gaction (BJB_SETTINGS (self->settings));
  g_action_map_add_action (G_ACTION_MAP (application), text_size);
}

static void
bijiben_finalize (GObject *object)
{
  BjbApplication *self = BJB_APPLICATION (object);

  g_clear_object (&self->settings);

  G_OBJECT_CLASS (bjb_application_parent_class)->finalize (object);
}

static void
bjb_application_class_init (BjbApplicationClass *klass)
{
  GApplicationClass *aclass = G_APPLICATION_CLASS (klass);
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  aclass->activate = bijiben_activate;
  aclass->startup = bijiben_startup;

  oclass->finalize = bijiben_finalize;
}

BjbApplication *
bjb_application_new (void)
{
  return g_object_new (BJB_TYPE_APPLICATION,
                       "application-id", BIJIBEN_APPLICATION_ID,
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

BjbSettings * bjb_app_get_settings(gpointer application)
{
  return BJB_APPLICATION(application)->settings;
}
