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
#include "bjb-import-dialog.h"
#include "bjb-log.h"


struct _BjbApplication
{
  AdwApplication  parent_instance;

  BjbSettings    *settings;

  /* Controls. to_open is for startup */
  GAction        *text_size;
  gboolean        first_run;
  gboolean        is_loaded;
  gboolean        new_note;
  GQueue          files_to_open; // paths
};

G_DEFINE_TYPE (BjbApplication, bjb_application, ADW_TYPE_APPLICATION)

static void     bijiben_new_window_internal (BjbApplication *self,
                                             BjbNote        *note);
void            on_preferences_cb           (GSimpleAction      *action,
                                             GVariant           *parameter,
                                             gpointer            user_data);

void            on_help_cb                  (GSimpleAction      *action,
                                             GVariant           *parameter,
                                             gpointer            user_data);

void            on_about_cb                 (GSimpleAction      *action,
                                             GVariant           *parameter,
                                             gpointer            user_data);

gboolean        text_size_mapping_get       (GValue             *value,
                                             GVariant           *variant,
                                             gpointer            user_data);

GVariant       *text_size_mapping_set       (const GValue       *value,
                                             const GVariantType *expected_type,
                                             gpointer            user_data);

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
bijiben_new_window_internal (BjbApplication *self,
                             BjbNote        *note)
{
  GtkWindow *active_window;
  BjbWindow *window;

  active_window = gtk_application_get_active_window (GTK_APPLICATION (self));

  window = BJB_WINDOW (bjb_window_new ());

  if (active_window)
    bjb_window_set_is_main (window, FALSE);
  else
    bjb_window_set_is_main (window, TRUE);

  bjb_window_set_note (window, note);

  gtk_widget_show (GTK_WIDGET (window));

  if (g_strcmp0 (PROFILE, "") != 0)
    {
      GtkStyleContext *style_context;

      style_context = gtk_widget_get_style_context (GTK_WIDGET (window));
      gtk_style_context_add_class (style_context, "devel");
    }
}

static void
bjb_application_window_removed (GtkApplication *app,
                                GtkWindow      *window)
{
  if (BJB_IS_WINDOW (window) &&
      bjb_window_get_is_main (BJB_WINDOW (window)))
    {
      GList *windows;

      windows = gtk_application_get_windows (app);

      for (GList *node = windows; node; node = node->next)
        {
          if (BJB_IS_WINDOW (node->data) &&
              window != node->data)
            {
              bjb_window_set_is_main (node->data, TRUE);
              gtk_window_present (node->data);
              break;
            }
        }
    }

  GTK_APPLICATION_CLASS (bjb_application_parent_class)->window_removed (app, window);
}

static int
bijiben_handle_local_options (GApplication *application,
                              GVariantDict *options)
{
  if (g_variant_dict_contains (options, "version"))
    {
      g_print ("%s %s\n", _("GNOME Notes"), PACKAGE_VCS_VERSION);
      return 0;
    }

  return -1;
}

static void
bijiben_activate (GApplication *app)
{
  GtkWindow *window;

  window = gtk_application_get_active_window (GTK_APPLICATION (app));

  if (window)
    gtk_window_present (window);
  else
    bijiben_new_window_internal (BJB_APPLICATION (app), NULL);
}

static void
bjb_application_init (BjbApplication *self)
{
  GApplication *app = G_APPLICATION (self);
  const GOptionEntry cmd_options[] = {
    { "verbose", 'v', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, cmd_verbose_cb,
      N_("Show verbose logs"), NULL},
    { "version", 0, 0, G_OPTION_ARG_NONE, NULL,
      N_("Show the application’s version"), NULL},
    { "new-note", 0, 0, G_OPTION_ARG_NONE, &self->new_note,
      N_("Create a new note"), NULL},
    { NULL }
  };

  g_queue_init (&self->files_to_open);

  gtk_window_set_default_icon_name ("org.gnome.Notes");
  g_application_add_main_option_entries (app, cmd_options);
  g_application_set_option_context_parameter_string (app, _("[FILE…]"));
  g_application_set_option_context_summary (app, _("Take notes and export them everywhere."));
}

void
on_preferences_cb (GSimpleAction *action,
                   GVariant      *parameter,
                   gpointer       user_data)
{
  GtkApplication *app = GTK_APPLICATION (user_data);
  GList *windows = gtk_application_get_windows (app);

  show_bijiben_settings_window (g_list_nth_data (windows, 0));
}

void
on_help_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  bjb_app_help (BJB_APPLICATION (user_data));
}

void
on_about_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  bjb_app_about (BJB_APPLICATION (user_data));
}

gboolean
text_size_mapping_get (GValue   *value,
                       GVariant *variant,
                       gpointer  user_data)
{
  g_value_set_variant (value, variant);
  return TRUE;
}

GVariant *
text_size_mapping_set (const GValue       *value,
                       const GVariantType *expected_type,
                       gpointer            user_data)
{
  return g_value_dup_variant (value);
}

static gboolean
transform_to (GBinding     *binding,
              const GValue *from_value,
              GValue       *to_value,
              gpointer      user_data)
{
  const char *value;

  value = g_value_get_string (from_value);
  g_value_set_variant (to_value, g_variant_new_string (value));

  return TRUE;
}

static gboolean
transform_from (GBinding     *binding,
                const GValue *from_value,
                GValue       *to_value,
                gpointer      user_data)
{
  GVariant *value;

  value = g_value_get_variant (from_value);
  g_value_set_string (to_value, g_variant_get_string (value, NULL));

  return TRUE;
}

static GActionEntry app_entries[] = {
  { "text-size", NULL, "s", "'medium'", NULL },
  { "preferences", on_preferences_cb, NULL, NULL, NULL },
  { "help", on_help_cb, NULL, NULL, NULL },
  { "about", on_about_cb, NULL, NULL, NULL },
};

static void
bijiben_startup (GApplication *application)
{
  BjbApplication *self;
  g_autofree char *path = NULL;
  g_autofree gchar *storage_path = NULL;
  g_autofree gchar *default_color = NULL;
  g_autoptr(GFile) storage = NULL;
  g_autoptr(GError) error = NULL;
  GdkRGBA         color = {0,0,0,0};

  const gchar *vaccels_close[] = {"<Primary>w", NULL};
  const gchar *vaccels_detach[] = {"<Primary>d", NULL};
  const gchar *vaccels_redo[] = {"<Primary><Shift>z", NULL};
  const gchar *vaccels_undo[] = {"<Primary>z", NULL};
  const gchar *vaccels_trash[] = {"<Primary>Delete", NULL};

  G_APPLICATION_CLASS (bjb_application_parent_class)->startup (application);
  self = BJB_APPLICATION (application);

  self->settings = bjb_settings_new ();

  gtk_window_set_default_icon_name ("org.gnome.Notes");

  gtk_application_set_accels_for_action (GTK_APPLICATION (application), "win.close", vaccels_close);
  gtk_application_set_accels_for_action (GTK_APPLICATION (application), "win.detach-window", vaccels_detach);
  gtk_application_set_accels_for_action (GTK_APPLICATION (application), "win.redo", vaccels_redo);
  gtk_application_set_accels_for_action (GTK_APPLICATION (application), "win.undo", vaccels_undo);
  gtk_application_set_accels_for_action (GTK_APPLICATION (application), "win.trash", vaccels_trash);

  g_action_map_add_action_entries (G_ACTION_MAP (application),
                                   app_entries,
                                   G_N_ELEMENTS (app_entries),
                                   application);

  storage_path = g_build_filename (g_get_user_data_dir (), "bijiben", NULL);
  storage = g_file_new_for_path (storage_path);

  // Create the bijiben dir to ensure.
  self->first_run = TRUE;
  g_file_make_directory (storage, NULL, &error);

  // If fails it's not the first run
  if (error && error->code == G_IO_ERROR_EXISTS)
    self->first_run = FALSE;
  else if (error)
    g_warning ("%s", error->message);

  g_object_get (self->settings, "color", &default_color, NULL);
  gdk_rgba_parse (&color, default_color);

  self->text_size = g_action_map_lookup_action (G_ACTION_MAP (application), "text-size");
  g_object_bind_property_full (BJB_SETTINGS (self->settings), "text-size",
                               self->text_size, "state",
                               G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL,
                               transform_to, transform_from, NULL, NULL);

  /* Load last opened note item. */
  path = bjb_settings_get_last_opened_item (self->settings);
  if (g_strcmp0 (path, "") != 0)
    g_queue_push_head (&self->files_to_open, path);
}

static void
bijiben_finalize (GObject *object)
{
  BjbApplication *self = BJB_APPLICATION (object);

  g_clear_object (&self->settings);
  g_queue_foreach (&self->files_to_open, (GFunc) g_free, NULL);
  g_queue_clear (&self->files_to_open);

  G_OBJECT_CLASS (bjb_application_parent_class)->finalize (object);
}

static void
bjb_application_class_init (BjbApplicationClass *klass)
{
  GtkApplicationClass *app_class = GTK_APPLICATION_CLASS (klass);
  GApplicationClass *aclass = G_APPLICATION_CLASS (klass);
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  app_class->window_removed = bjb_application_window_removed;

  aclass->handle_local_options = bijiben_handle_local_options;
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

void
bjb_app_help (BjbApplication *self)
{
  GtkApplication *app = GTK_APPLICATION (self);

  gtk_show_uri (gtk_application_get_active_window (app),
                "help:bijiben",
                GDK_CURRENT_TIME);
}


void
bjb_app_about (BjbApplication *self)
{
  GtkApplication *app = GTK_APPLICATION (self);
  GList *windows = gtk_application_get_windows (app);

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
                         "website", "https://wiki.gnome.org/Apps/Notes",
                         "logo-icon-name", BIJIBEN_APPLICATION_ID,
                         NULL);
}

