/*
 * bijiben.c
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>
#include <stdlib.h>

#include <libbiji/libbiji.h>

#include "bjb-app-menu.h"
#include "bjb-bijiben.h"
#include "bjb-settings.h"
#include "bjb-main-view.h"
#include "bjb-note-view.h"
#include "bjb-window-base.h"

struct _BijibenPriv
{
  BijiNoteBook *book;
  BjbSettings *settings;



  /* Controls. 1st run is not used yet. to_open is for startup */

  gboolean     first_run;
  gboolean     is_loaded;
  GQueue       *to_open;
};

G_DEFINE_TYPE (Bijiben, bijiben, GTK_TYPE_APPLICATION);

static void
bijiben_new_window_internal (Bijiben *app,
                             GFile *file,
                             BijiNoteObj *note_obj,
                             GError *error);

static void
on_window_activated_cb   (BjbWindowBase *window,
                          gboolean win_is_available,
                          Bijiben *self)
{
  BijibenPriv *priv;
  gchar *path;
  BijiItem *item;

  item = NULL;
  priv = self->priv;
  priv->is_loaded = TRUE;

  while ((path = g_queue_pop_head (priv->to_open)))
  {

    item = biji_note_book_get_item_at_path (priv->book, path);

    if (item != NULL)
    {

      // fixme - bjb_window_base_switch_to_item...
      if (win_is_available)
         bjb_window_base_switch_to_note (window, item);
      
      else
         bijiben_new_window_internal (self, NULL, item, NULL);
    }

    // TODO : the file is not a note. We should try to serialize it.
    else {}

    g_free (path);
  }
}


static void
bijiben_new_window_internal (Bijiben *self,
                             GFile *file,
                             BijiNoteObj *note_obj,
                             GError *error)
{
  BjbWindowBase *window;
  BijiNoteObj* note;
  gchar *path;


  note = NULL;
  path = NULL;

  window = BJB_WINDOW_BASE (bjb_window_base_new ());
  g_signal_connect (window, "activated",
                    G_CALLBACK (on_window_activated_cb), self);


  if (error!= NULL)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    bjb_window_base_switch_to (window, BJB_WINDOW_BASE_ERROR_TRACKER);
    goto out;
  }


  if (file != NULL)
  {
    path = g_file_get_path (file);
    note = biji_note_book_get_item_at_path (self->priv->book, path);
  }

  else if (note_obj != NULL)
  {
    note = note_obj;
  }


  bjb_window_base_switch_to (window, BJB_WINDOW_BASE_MAIN_VIEW);

  if (note != NULL)
      bjb_window_base_switch_to_note (window, note);

out:
  if (path != NULL)
    g_free (path);

  gtk_widget_show_all (GTK_WIDGET (window));
}

void
bijiben_new_window_for_note (GApplication *app,
                             BijiNoteObj *note)
{
  bijiben_new_window_internal (BIJIBEN_APPLICATION (app), NULL, note, NULL);
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

  self = BIJIBEN_APPLICATION (application);

  for (i = 0; i < n_files; i++)
  {
    if (self->priv->is_loaded == TRUE)
      bijiben_new_window_internal (BIJIBEN_APPLICATION (application), files[i], NULL, NULL);
    
    else
      g_queue_push_head (self->priv->to_open, g_file_get_path (files[i]));
  }
}

static void
bijiben_init (Bijiben *self)
{
  BijibenPriv *priv;


  priv = self->priv =
    G_TYPE_INSTANCE_GET_PRIVATE (self, BIJIBEN_TYPE_APPLICATION, BijibenPriv);

  priv->settings = bjb_settings_new ();
  priv->to_open = g_queue_new ();
  priv->is_loaded = FALSE;
}

/* Import. TODO : move to libbiji */

#define ATTRIBUTES_FOR_NOTEBOOK "standard::content-type,standard::name"

static BijiNoteObj *
abort_note (BijiNoteObj *rejected)
{
  g_object_unref (rejected);
  return NULL;
}

static BijiNoteObj *
copy_note (GFileInfo *info, GFile *container)
{
  BijiNoteObj *retval = NULL;
  const gchar *name;
  gchar *path;
  Bijiben *self;
  BijiNoteBook *book;

  self = BIJIBEN_APPLICATION (g_application_get_default ());
  book = self->priv->book;

  /* First make sure it's a note */
  name = g_file_info_get_name (info);
  if (!g_str_has_suffix (name, ".note"))
    return NULL;

  /* Deserialize it */
  path = g_build_filename (g_file_get_path (container), name, NULL);
  retval = biji_note_get_new_from_file (book, path);
  g_free (path);

  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (retval), NULL);

  /* Not a Template */
  if (biji_note_obj_is_template (retval))
    return abort_note (retval);

  /* Assign the new path */
  path = g_build_filename (g_get_user_data_dir (), "bijiben", name, NULL);
  g_object_set (retval, "path", path, NULL);
  g_free (path);

  return retval;
}

static void
release_enum_cb (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_file_enumerator_close_finish (G_FILE_ENUMERATOR (source), res, NULL);
  g_object_unref (source);
}

/* Some notes might have been added previously */
static void
go_through_notes_cb (GFileEnumerator *enumerator, GAsyncResult *res, Bijiben *self)
{
  GList *notes_info;
  GList *notes_proposal = NULL;
  GList *l;
  GFile *container;

  /* Sanitize title & color */
  gchar *unique_title, *default_color;
  BjbSettings *settings;
  GdkRGBA color;

  container = g_file_enumerator_get_container (enumerator);
  notes_info = g_file_enumerator_next_files_finish (enumerator, res, NULL);
  g_file_enumerator_close_async (enumerator, G_PRIORITY_DEFAULT, NULL,
                                 release_enum_cb, self);

  /* Get the GList of notes and load them */
  for ( l=notes_info; l !=NULL; l = l->next)
  {
    GFileInfo *info = G_FILE_INFO (l->data);
    BijiNoteObj *iter = copy_note (info, container);
    if (iter)
      notes_proposal = g_list_prepend (notes_proposal, iter);
  }

  for (l = notes_proposal; l != NULL; l = l->next)
  {
    BijiNoteObj *note = l->data;
    const gchar *path = biji_item_get_uuid (BIJI_ITEM (note));

    /* Don't add an already imported note */
    if (biji_note_book_get_item_at_path (self->priv->book, path))
    {
      abort_note (note);
    }

    /* Sanitize, append & save */
    else
    {
      /* Title */
      unique_title = biji_note_book_get_unique_title (self->priv->book,
                                                      biji_item_get_title (BIJI_ITEM (note)));
      biji_note_obj_set_title (note, unique_title);
      g_free (unique_title);

      /* Color */
      settings = bjb_app_get_settings (self);
      g_object_get (G_OBJECT (settings), "color", &default_color, NULL);
      if (gdk_rgba_parse (&color, default_color))
        biji_note_obj_set_rgba (note, &color);

      g_free (default_color);

      biji_note_book_add_item (self->priv->book, BIJI_ITEM (note), FALSE);
      biji_note_obj_save_note (note);
    }

  }

  /* NoteBook will notify for all opened windows */

  g_list_free_full (notes_info, g_object_unref);
  g_list_free (notes_proposal);
  biji_note_book_notify_changed (self->priv->book, BIJI_BOOK_MASS_CHANGE, NULL);
}

static void
list_notes_to_copy (GObject *src_obj, GAsyncResult *res, Bijiben *self)
{
  GFileEnumerator *enumerator;
  GError *error = NULL;

  enumerator = g_file_enumerate_children_finish (G_FILE (src_obj), res, &error);

  if (error)
  {
    g_warning ("Enumerator failed : %s", error->message);
    g_error_free (error);
    g_file_enumerator_close_async (enumerator, G_PRIORITY_DEFAULT, NULL, release_enum_cb, self);
  }

  else
  {
    g_file_enumerator_next_files_async (enumerator, G_MAXINT, G_PRIORITY_DEFAULT, NULL,
                                        (GAsyncReadyCallback) go_through_notes_cb, self);
  }
}


void
bijiben_import_notes (Bijiben *self, gchar *location)
{
  GFile *to_import = g_file_new_for_path (location);

  g_file_enumerate_children_async (to_import, ATTRIBUTES_FOR_NOTEBOOK,
                                   G_FILE_QUERY_INFO_NONE, G_PRIORITY_DEFAULT,
                                   NULL, (GAsyncReadyCallback) list_notes_to_copy, self);

  g_object_unref (to_import);
}


/* Just filter on ownCloud accounts
 * TODO : settings to force activate & inactivate
 * but, up to libbiji to survey GoaObject
 */
static void
on_client_got (GObject *source_object,
               GAsyncResult *res,
               gpointer user_data)
{
  Bijiben *self;
  GoaClient *client;
  GError *error;
  GList *accounts, *l;
  GoaObject *object;
  GoaAccount *account;
  const gchar *type;

  self = BIJIBEN_APPLICATION (user_data);
  error = NULL;
  client =  goa_client_new_finish  (res, &error);

  if (error)
  {
     g_warning ("%s", error->message);
     g_error_free (error);
     return;
  }

  accounts = goa_client_get_accounts (client);

  for (l=accounts; l!=NULL; l=l->next)
  {
    object = GOA_OBJECT (l->data);
    account =  goa_object_get_account (object);

    if (GOA_IS_ACCOUNT (account))
    {
      type = goa_account_get_provider_type (account);


      /* We do not need to store any object here.
       * account_get_id can be used to talk with libbji */

      if (g_strcmp0 (type, "owncloud") ==0)
      {
        g_message ("Loading account %s", goa_account_get_id (account));
        biji_note_book_add_goa_object (self->priv->book, object);
      }

      else
      {
        g_object_unref (object);
      }
    } 
  }

  g_list_free (accounts);
}

static void
bijiben_startup (GApplication *application)
{
  Bijiben        *self;
  gchar          *storage_path, *default_color;
  GFile          *storage;
  GError         *error;
  GdkRGBA         color = {0,0,0,0};


  G_APPLICATION_CLASS (bijiben_parent_class)->startup (application);
  self = BIJIBEN_APPLICATION (application);
  error = NULL;


  if (gtk_clutter_init (NULL, NULL) != CLUTTER_INIT_SUCCESS)
  {
    g_warning ("Unable to initialize Clutter");
    return;
  }


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

  error = NULL;
  self->priv->book = biji_note_book_new (storage, &color, &error);
  if (error)
    goto out;

  /* Goa */
  goa_client_new  (NULL, on_client_got, self); // cancellable

  /* Create the first window */
  out:
  bijiben_new_window_internal (BIJIBEN_APPLICATION (application), NULL, NULL, error);
  g_free (default_color);
  g_free (storage_path);
  g_object_unref (storage);
}

static void
bijiben_finalize (GObject *object)
{
  Bijiben *self = BIJIBEN_APPLICATION (object);

  g_clear_object (&self->priv->book);
  g_clear_object (&self->priv->settings);
  g_queue_free (self->priv->to_open);

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

BijiNoteBook *
bijiben_get_book(Bijiben *self)
{
  return self->priv->book ;
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
