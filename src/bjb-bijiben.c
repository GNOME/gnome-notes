/*
 * bjb-bijiben.c
 * Copyright (C) 2011 Pierre-Yves LUYTEN <py@luyten.fr>
 * Copyright (C) 2017 Iñigo Martínez <inigomartinez@gmail.com>
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

#include "bjb-app-menu.h"
#include "bjb-bijiben.h"
#include "bjb-settings.h"
#include "bjb-main-view.h"
#include "bjb-note-view.h"
#include "bjb-window-base.h"

struct _BijibenPriv
{
  BijiManager *manager;
  BjbSettings *settings;

  /* Controls. to_open is for startup */

  gboolean     first_run;
  gboolean     is_loaded;
  gboolean     new_note;
  GQueue       files_to_open; // paths
};


G_DEFINE_TYPE (Bijiben, bijiben, GTK_TYPE_APPLICATION);

static void     bijiben_new_window_internal (Bijiben       *self,
                                             BijiNoteObj   *note);
static gboolean bijiben_open_path           (Bijiben       *self,
                                             gchar         *path,
                                             BjbWindowBase *window);

static void
on_window_activated_cb (BjbWindowBase *window,
                        gboolean       available,
                        Bijiben       *self)
{
  BijibenPriv *priv;
  GList *l, *next;

  priv = self->priv;
  priv->is_loaded = TRUE;

  for (l = priv->files_to_open.head; l != NULL; l = next)
  {
    next = l->next;
    if (bijiben_open_path (self, l->data, (available ? window : NULL)))
    {
      g_free (l->data);
      g_queue_delete_link (&priv->files_to_open, l);
    }
  }

  /* All requested notes are loaded, but we have a new one to create...
   * This implementation is not really safe,
   * we might have loaded SOME provider(s)
   * but not the default one - more work is needed here */
  if (priv->new_note && g_queue_is_empty (&priv->files_to_open))
  {
    BijiItem *item;

    priv->new_note = FALSE;
    item = BIJI_ITEM (biji_manager_note_new (
                        priv->manager,
                        NULL,
                        bjb_settings_get_default_location (self->priv->settings)));
    bijiben_new_window_internal (self, BIJI_NOTE_OBJ (item));
  }
}

static void
bijiben_new_window_internal (Bijiben     *self,
                             BijiNoteObj *note)
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
bijiben_open_path (Bijiben       *self,
                   gchar         *path,
                   BjbWindowBase *window)
{
  BijiItem *item;

  if (!self->priv->is_loaded)
    return FALSE;

  item = biji_manager_get_item_at_path (self->priv->manager, path);

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
  bijiben_new_window_internal (BIJIBEN_APPLICATION (app), note);
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
  Bijiben *self;
  gint i;
  gchar *path;

  self = BIJIBEN_APPLICATION (application);

  for (i = 0; i < n_files; i++)
  {
    path = g_file_get_parse_name (files[i]);
    if (bijiben_open_path (self, path, NULL))
      g_free (path);
    else
      g_queue_push_head (&self->priv->files_to_open, path);
  }
}

static void
bijiben_init (Bijiben *self)
{
  BijibenPriv *priv;


  priv = self->priv =
    G_TYPE_INSTANCE_GET_PRIVATE (self, BIJIBEN_TYPE_APPLICATION, BijibenPriv);

  priv->settings = bjb_settings_new ();
  g_queue_init (&priv->files_to_open);
  priv->new_note = FALSE;
  priv->is_loaded = FALSE;
}


void
bijiben_import_notes (Bijiben *self, gchar *uri)
{
  g_debug ("IMPORT to %s", bjb_settings_get_default_location (self->priv->settings));

  biji_manager_import_uri (self->priv->manager,
                           bjb_settings_get_default_location (self->priv->settings),
                           uri);
}

static void
theme_changed (GtkSettings *settings)
{
  static GtkCssProvider *provider = NULL;
  gchar *theme;
  GdkScreen *screen;

  g_object_get (settings, "gtk-theme-name", &theme, NULL);
  screen = gdk_screen_get_default ();

  if (g_str_equal (theme, "Adwaita"))
  {
    if (provider == NULL)
    {
        GFile *file;

        provider = gtk_css_provider_new ();
        file = g_file_new_for_uri ("resource:///org/gnome/bijiben/Adwaita.css");
        gtk_css_provider_load_from_file (provider, file, NULL);
        g_object_unref (file);
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

  g_free (theme);
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
  Bijiben *self = user_data;
  GError *error = NULL;

  self->priv->manager = biji_manager_new_finish (res, &error);
  g_application_release (G_APPLICATION (self));

  if (error != NULL)
    {
      g_warning ("Cannot initialize BijiManager: %s\n", error->message);
      g_clear_error (&error);
      return;
    }

  bijiben_new_window_internal (self, NULL);
}

static void
bijiben_startup (GApplication *application)
{
  Bijiben        *self;
  gchar          *storage_path, *default_color;
  GFile          *storage;
  GError         *error;
  gchar          *path, *uri;
  GdkRGBA         color = {0,0,0,0};


  G_APPLICATION_CLASS (bijiben_parent_class)->startup (application);
  self = BIJIBEN_APPLICATION (application);
  error = NULL;

  bjb_apply_style ();

  bjb_app_menu_set(application);

  storage_path = g_build_filename (g_get_user_data_dir (), "bijiben", NULL);
  storage = g_file_new_for_path (storage_path);

  // Create the bijiben dir to ensure.
  self->priv->first_run = TRUE;
  g_file_make_directory (storage, NULL, &error);

  // If fails it's not the first run
  if (error && error->code == G_IO_ERROR_EXISTS)
  {
    self->priv->first_run = FALSE;
    g_error_free (error);
  }

  else if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  else
  {
    self->priv->first_run = TRUE;
  }


  g_object_get (self->priv->settings, "color", &default_color, NULL);
  gdk_rgba_parse (&color, default_color);

  g_application_hold (application);
  biji_manager_new_async (storage, &color, manager_ready_cb, self);

  /* Automatic imports on startup */
  if (self->priv->first_run == TRUE)
  {
    path = g_build_filename (g_get_user_data_dir (), "tomboy", NULL);
    uri = g_filename_to_uri (path, NULL, NULL);
    bijiben_import_notes (self, uri);
    g_free (path);
    g_free (uri);

    path = g_build_filename (g_get_user_data_dir (), "gnote", NULL);
    uri = g_filename_to_uri (path, NULL, NULL);
    bijiben_import_notes (self, uri);
    g_free (path);
    g_free (uri);
  }

  g_free (default_color);
  g_free (storage_path);
  g_object_unref (storage);
}

static gboolean
bijiben_application_local_command_line (GApplication *application,
                                        gchar ***arguments,
                                        gint *exit_status)
{
  Bijiben *self;
  gboolean version = FALSE;
  gchar **remaining = NULL;
  GOptionContext *context;
  GError *error = NULL;
  gint argc = 0;
  gchar **argv = NULL;
  *exit_status = EXIT_SUCCESS;

  const GOptionEntry options[] = {
    { "version", 0, 0, G_OPTION_ARG_NONE, &version,
      N_("Show the application's version"), NULL},
    { "new-note", 0, 0, G_OPTION_ARG_NONE, &BIJIBEN_APPLICATION(application)->priv->new_note,
      N_("Create a new note"), NULL},
    { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_STRING_ARRAY, &remaining,
      NULL,  N_("[FILE...]") },
    { NULL }
  };

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
    g_error_free (error);

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
    g_error_free (error);

    *exit_status = EXIT_FAILURE;
    goto out;
  }

  self = BIJIBEN_APPLICATION (application);

  if (!self->priv->new_note && remaining != NULL)
  {
    gchar **args;

    for (args = remaining; *args; args++)
      if (!bijiben_open_path (self, *args, NULL))
        g_queue_push_head (&self->priv->files_to_open, g_strdup (*args));
  }

 out:
  g_strfreev (remaining);
  g_option_context_free (context);

  return TRUE;
}

static void
bijiben_finalize (GObject *object)
{
  Bijiben *self = BIJIBEN_APPLICATION (object);

  g_clear_object (&self->priv->manager);
  g_clear_object (&self->priv->settings);
  g_queue_foreach (&self->priv->files_to_open, (GFunc) g_free, NULL);
  g_queue_clear (&self->priv->files_to_open);

  G_OBJECT_CLASS (bijiben_parent_class)->finalize (object);
}

static void
bijiben_class_init (BijibenClass *klass)
{
  GApplicationClass *aclass = G_APPLICATION_CLASS (klass);
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  aclass->activate = bijiben_activate;
  aclass->open = bijiben_open;
  aclass->startup = bijiben_startup;
  aclass->local_command_line = bijiben_application_local_command_line;

  oclass->finalize = bijiben_finalize;

  g_type_class_add_private (klass, sizeof (BijibenPriv));
}

Bijiben *
bijiben_new (void)
{
  return g_object_new (BIJIBEN_TYPE_APPLICATION,
                       "application-id", "org.gnome.bijiben",
                       "flags", G_APPLICATION_HANDLES_OPEN,
                       NULL);
}

BijiManager *
bijiben_get_manager(Bijiben *self)
{
  return self->priv->manager ;
}

const gchar *
bijiben_get_bijiben_dir (void)
{
  return DATADIR;
}

BjbSettings * bjb_app_get_settings(gpointer application)
{
  return BIJIBEN_APPLICATION(application)->priv->settings ;
}
