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

#include "config.h"

#include <glib/gi18n.h>
#include <stdlib.h>
#include <libedataserver/libedataserver.h> /* ESourceRegistry */
#include <libecal/libecal.h>               /* ECalClient      */


#include <libbiji/libbiji.h>

#include "bjb-application.h"
#include "bjb-settings.h"
#include "bjb-main-view.h"
#include "bjb-note-view.h"
#include "bjb-window-base.h"
#include "bjb-import-dialog.h"

struct _BjbApplication
{
  GtkApplication parent_instance;

  BijiManager *manager;
  BjbSettings *settings;

  /* Controls. to_open is for startup */
  GAction     *text_size;
  gboolean     first_run;
  gboolean     is_loaded;
  gboolean     new_note;
  GQueue       files_to_open; // paths
};

G_DEFINE_TYPE (BjbApplication, bjb_application, GTK_TYPE_APPLICATION)

static void     bijiben_new_window_internal (BjbApplication *self,
                                             BijiNoteObj   *note);
static gboolean bijiben_open_path           (BjbApplication *self,
                                             gchar          *path,
                                             BjbWindowBase *window);

void            on_import_notes_cb          (GSimpleAction      *action,
                                             GVariant           *parameter,
                                             gpointer            user_data);

void            on_view_trash_cb            (GSimpleAction      *action,
                                             GVariant           *parameter,
                                             gpointer            user_data);

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

static void
on_window_activated_cb (BjbWindowBase  *window,
                        gboolean        available,
                        BjbApplication *self)
{
  GList *l, *next;

  self->is_loaded = TRUE;

  for (l = self->files_to_open.head; l != NULL; l = next)
  {
    next = l->next;
    if (bijiben_open_path (self, l->data, (available ? window : NULL)))
    {
      g_free (l->data);
      g_queue_delete_link (&self->files_to_open, l);
    }
  }

  /* All requested notes are loaded, but we have a new one to create...
   * This implementation is not really safe,
   * we might have loaded SOME provider(s)
   * but not the default one - more work is needed here */
  if (self->new_note && g_queue_is_empty (&self->files_to_open))
  {
    BijiItem *item;

    self->new_note = FALSE;
    item = BIJI_ITEM (biji_manager_note_new (
                        self->manager,
                        NULL,
                        bjb_settings_get_default_location (self->settings)));
    bijiben_new_window_internal (self, BIJI_NOTE_OBJ (item));
  }
}

static void
bijiben_new_window_internal (BjbApplication *self,
                             BijiNoteObj    *note)
{
  BjbWindowBase *window;
  GList         *windows;
  gboolean       not_first_window;

  windows = gtk_application_get_windows (GTK_APPLICATION (self));
  not_first_window = (gboolean) g_list_length (windows);

  window = BJB_WINDOW_BASE (bjb_window_base_new (note));
  g_signal_connect (window, "activated",
                    G_CALLBACK (on_window_activated_cb), self);

  gtk_widget_show (GTK_WIDGET (window));

  if (not_first_window)
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_CENTER);
}

static gboolean
bijiben_open_path (BjbApplication *self,
                   gchar          *path,
                   BjbWindowBase  *window)
{
  BijiItem *item;

  if (!self->is_loaded)
    return FALSE;

  item = biji_manager_get_item_at_path (self->manager, path);

  if (BIJI_IS_NOTE_OBJ (item) || !window)
    bijiben_new_window_internal (self, BIJI_NOTE_OBJ (item));
  else
    bjb_window_base_switch_to_item (window, item);

  return TRUE;
}

void
bijiben_new_window_for_note (GApplication *app,
                             BijiNoteObj *note)
{
  bijiben_new_window_internal (BJB_APPLICATION (app), note);
}

static void
bijiben_activate (GApplication *app)
{
  GList *windows = gtk_application_get_windows (GTK_APPLICATION (app));

  // ensure the last used window is raised
  gtk_window_present (g_list_nth_data (windows, 0));
}


/* If the app is already loaded, just open the file.
 * Else, keep it under the hood */

static void
bijiben_open (GApplication  *application,
              GFile        **files,
              gint           n_files,
              const gchar   *hint)
{
  BjbApplication *self;
  gint i;
  g_autofree gchar *path = NULL;

  self = BJB_APPLICATION (application);

  for (i = 0; i < n_files; i++)
  {
    path = g_file_get_parse_name (files[i]);
    if (!bijiben_open_path (self, path, NULL))
      g_queue_push_head (&self->files_to_open, path);
  }
}

static void
bjb_application_init (BjbApplication *self)
{
  self->settings = bjb_settings_new ();
  g_queue_init (&self->files_to_open);

  gtk_window_set_default_icon_name ("org.gnome.Notes");
}


static void
bijiben_import_notes (BjbApplication *self, gchar *uri)
{
  g_debug ("IMPORT to %s", bjb_settings_get_default_location (self->settings));

  biji_manager_import_uri (self->manager,
                           bjb_settings_get_default_location (self->settings),
                           uri);
}

void
on_import_notes_cb (GSimpleAction *action,
                    GVariant      *parameter,
                    gpointer       user_data)
{
  bjb_app_import_notes (BJB_APPLICATION (user_data));
}

void
on_view_trash_cb (GSimpleAction *action,
                  GVariant      *parameter,
                  gpointer       user_data)
{
  GList *windows = gtk_application_get_windows (GTK_APPLICATION (user_data));
  BjbController *controller = bjb_window_base_get_controller (BJB_WINDOW_BASE (windows->data));

  bjb_controller_set_group (controller, BIJI_ARCHIVED_ITEMS);
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

static void
theme_changed (GtkSettings *settings)
{
  static GtkCssProvider *provider = NULL;
  g_autofree gchar *theme = NULL;
  GdkScreen *screen;

  g_object_get (settings, "gtk-theme-name", &theme, NULL);
  screen = gdk_screen_get_default ();

  if (g_str_equal (theme, "Adwaita"))
  {
    if (provider == NULL)
    {
        g_autoptr(GFile) file = NULL;

        provider = gtk_css_provider_new ();
        file = g_file_new_for_uri ("resource:///org/gnome/Notes/Adwaita.css");
        gtk_css_provider_load_from_file (provider, file, NULL);
    }

    gtk_style_context_add_provider_for_screen (screen,
                                               GTK_STYLE_PROVIDER (provider),
                                               GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  }
  else if (provider != NULL)
  {
    gtk_style_context_remove_provider_for_screen (screen,
                                                  GTK_STYLE_PROVIDER (provider));
    g_clear_object (&provider);
  }
}

static void
bjb_apply_style (void)
{
  GtkSettings *settings;

  /* Set up a handler to load our custom css for Adwaita.
   * See https://bugzilla.gnome.org/show_bug.cgi?id=732959
   * for a more automatic solution that is still under discussion.
   */
  settings = gtk_settings_get_default ();
  g_signal_connect (settings, "notify::gtk-theme-name", G_CALLBACK (theme_changed), NULL);
  theme_changed (settings);
}

static void
manager_ready_cb (GObject *source,
                  GAsyncResult *res,
                  gpointer user_data)
{
  BjbApplication *self = user_data;
  g_autoptr(GError) error = NULL;
  g_autofree gchar *path = NULL;
  g_autofree gchar *uri = NULL;

  self->manager = biji_manager_new_finish (res, &error);
  g_application_release (G_APPLICATION (self));

  if (error != NULL)
    {
      g_warning ("Cannot initialize BijiManager: %s\n", error->message);
      return;
    }

  /* Automatic imports on startup */
  if (self->first_run == TRUE)
    {
      path = g_build_filename (g_get_user_data_dir (), "tomboy", NULL);
      uri = g_filename_to_uri (path, NULL, NULL);
      if (g_file_test (path, G_FILE_TEST_EXISTS))
        bijiben_import_notes (self, uri);
      g_free (path);
      g_free (uri);

      path = g_build_filename (g_get_user_data_dir (), "gnote", NULL);
      uri = g_filename_to_uri (path, NULL, NULL);
      if (g_file_test (path, G_FILE_TEST_EXISTS))
        bijiben_import_notes (self, uri);
    }

  bijiben_new_window_internal (self, NULL);
}

static GActionEntry app_entries[] = {
  { "import-notes", on_import_notes_cb, NULL, NULL, NULL },
  { "view-trash", on_view_trash_cb, NULL, NULL, NULL },
  { "text-size", NULL, "s", "'medium'", NULL },
  { "preferences", on_preferences_cb, NULL, NULL, NULL },
  { "help", on_help_cb, NULL, NULL, NULL },
  { "about", on_about_cb, NULL, NULL, NULL },
};

static void
bijiben_startup (GApplication *application)
{
  BjbApplication *self;
  g_autofree gchar *storage_path = NULL;
  g_autofree gchar *default_color = NULL;
  g_autoptr(GFile) storage = NULL;
  g_autoptr(GError) error = NULL;
  GdkRGBA         color = {0,0,0,0};

  const gchar *vaccels_redo[] = {"<Primary><Shift>z", NULL};
  const gchar *vaccels_undo[] = {"<Primary>z", NULL};

  G_APPLICATION_CLASS (bjb_application_parent_class)->startup (application);
  self = BJB_APPLICATION (application);

  bjb_apply_style ();

  gtk_application_set_accels_for_action (GTK_APPLICATION (application), "win.redo", vaccels_redo);
  gtk_application_set_accels_for_action (GTK_APPLICATION (application), "win.undo", vaccels_undo);
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
  g_settings_bind_with_mapping (G_SETTINGS (self->settings),
                                "text-size",
                                self->text_size,
                                "state",
                                G_SETTINGS_BIND_DEFAULT,
                                text_size_mapping_get,
                                text_size_mapping_set,
                                NULL,
                                NULL);

  g_application_hold (application);
  biji_manager_new_async (storage, &color, manager_ready_cb, self);
}

static gboolean
bijiben_application_local_command_line (GApplication *application,
                                        gchar ***arguments,
                                        gint *exit_status)
{
  BjbApplication *self;
  gboolean version = FALSE;
  gchar **remaining = NULL;
  GOptionContext *context;
  g_autoptr(GError) error = NULL;
  gint argc = 0;
  gchar **argv = NULL;

  const GOptionEntry options[] = {
    { "version", 0, 0, G_OPTION_ARG_NONE, &version,
      N_("Show the application’s version"), NULL},
    { "new-note", 0, 0, G_OPTION_ARG_NONE, &BJB_APPLICATION(application)->new_note,
      N_("Create a new note"), NULL},
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &remaining,
      NULL,  N_("[FILE…]") },
    { NULL }
  };

  *exit_status = EXIT_SUCCESS;

  context = g_option_context_new (NULL);
  g_option_context_set_summary (context,
                                _("Take notes and export them everywhere."));
  g_option_context_add_main_entries (context, options, NULL);
  g_option_context_add_group (context, gtk_get_option_group (FALSE));

  argv = *arguments;
  argc = g_strv_length (argv);

  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    /* Translators: this is a fatal error quit message
     * printed on the command line */
    g_printerr ("%s: %s\n", _("Could not parse arguments"), error->message);

    *exit_status = EXIT_FAILURE;
    goto out;
  }

  if (version)
  {
    g_print ("%s %s\n", _("GNOME Notes"), VERSION);
    goto out;
  }

  /* bijiben_startup */
  g_application_register (application, NULL, &error);

  if (error != NULL)
  {
    g_printerr ("%s: %s\n",
                /* Translators: this is a fatal error quit message
                 * printed on the command line */
                _("Could not register the application"),
                error->message);

    *exit_status = EXIT_FAILURE;
    goto out;
  }

  self = BJB_APPLICATION (application);

  if (!self->new_note && remaining != NULL)
  {
    gchar **args;

    for (args = remaining; *args; args++)
      if (!bijiben_open_path (self, *args, NULL))
        g_queue_push_head (&self->files_to_open, g_strdup (*args));
  }

 out:
  g_strfreev (remaining);
  g_option_context_free (context);

  return TRUE;
}

static void
bijiben_finalize (GObject *object)
{
  BjbApplication *self = BJB_APPLICATION (object);

  g_clear_object (&self->manager);
  g_clear_object (&self->settings);
  g_queue_foreach (&self->files_to_open, (GFunc) g_free, NULL);
  g_queue_clear (&self->files_to_open);

  G_OBJECT_CLASS (bjb_application_parent_class)->finalize (object);
}

static void
bjb_application_class_init (BjbApplicationClass *klass)
{
  GApplicationClass *aclass = G_APPLICATION_CLASS (klass);
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  aclass->activate = bijiben_activate;
  aclass->open = bijiben_open;
  aclass->startup = bijiben_startup;
  aclass->local_command_line = bijiben_application_local_command_line;

  oclass->finalize = bijiben_finalize;
}

BjbApplication *
bjb_application_new (void)
{
  return g_object_new (BJB_TYPE_APPLICATION,
                       "application-id", "org.gnome.Notes",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

BijiManager *
bijiben_get_manager(BjbApplication *self)
{
  return self->manager;
}

const gchar *
bijiben_get_bijiben_dir (void)
{
  return DATADIR;
}

BjbSettings * bjb_app_get_settings(gpointer application)
{
  return BJB_APPLICATION(application)->settings;
}

void
bjb_app_import_notes (BjbApplication *self)
{
  GtkDialog *dialog = bjb_import_dialog_new (GTK_APPLICATION (self));
  gint result = gtk_dialog_run (dialog);

  if (result == GTK_RESPONSE_OK)
    {
      GList *locations = bjb_import_dialog_get_paths (BJB_IMPORT_DIALOG (dialog));
      for (GList *l = locations; l != NULL; l = l->next)
        {
          g_autofree gchar *uri = g_filename_to_uri (l->data, NULL, NULL);
          bijiben_import_notes (self, uri);
        }

      g_list_free_full (locations, g_free);
    }

  gtk_widget_destroy (GTK_WIDGET (dialog));
}

void
bjb_app_help (BjbApplication *self)
{
  GtkApplication *app = GTK_APPLICATION (self);
  g_autoptr(GError) error = NULL;

  gtk_show_uri_on_window (gtk_application_get_active_window (app),
                          "help:bijiben",
                          GDK_CURRENT_TIME,
                          &error);

  if (error)
    g_warning ("%s", error->message);
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
                         "logo-icon-name", "org.gnome.Notes",
                         NULL);
}

