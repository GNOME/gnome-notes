/*
 * bijiben-shell-search-provider.c - Implementation of a GNOME Shell
 *   search provider
 *
 * Copyright © 2012, 2013 Red Hat, Inc.
 * Copyright © 2013 Pierre-Yves LUYTEN <py@luyten.fr>
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

/*
 * Adapted from Nautilus shell-search-provider
 *
 * Authors:
 * Original code : Cosimo Cecchi <cosimoc@gnome.org>
 * Bijiben : Pierre-Yves Luyten <py@luyten.fr>
 *
 */

#include <config.h>

#include <glib/gi18n.h>
#include <gio/gio.h>
#include <gio/gdesktopappinfo.h>
#include <gtk/gtk.h>

#include <tracker-sparql.h>

/* Strlen for timestamp */
#include <string.h>

#include "bijiben-shell-search-provider-generated.h"

#define SEARCH_PROVIDER_INACTIVITY_TIMEOUT 12000 /* milliseconds */

typedef struct {
  GApplication parent;

  BijibenShellSearchProvider2 *skeleton;
  TrackerSparqlConnection *connection;

} BijibenShellSearchProviderApp;

typedef GApplicationClass BijibenShellSearchProviderAppClass;

GType bijiben_shell_search_provider_app_get_type (void);

#define BIJIBEN_TYPE_SHELL_SEARCH_PROVIDER_APP bijiben_shell_search_provider_app_get_type()
#define BIJIBEN_SHELL_SEARCH_PROVIDER_APP(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJIBEN_TYPE_SHELL_SEARCH_PROVIDER_APP, BijibenShellSearchProviderApp))

G_DEFINE_TYPE (BijibenShellSearchProviderApp, bijiben_shell_search_provider_app, G_TYPE_APPLICATION)

static TrackerSparqlCursor *
bjb_perform_query (BijibenShellSearchProviderApp *self, gchar * query )
{
  TrackerSparqlCursor * result = NULL;
  GError *error = NULL ;

  result = tracker_sparql_connection_query (self->connection,
                                            query,
                                            NULL,
                                            &error);

  if (error)
    g_warning ("%s", error->message);

  return result ;
}

static GList *
biji_get_notes_with_strings (BijibenShellSearchProviderApp *self, gchar **needles)
{
  gint parser;
  GString *match;
  gchar *query, *needle;
  TrackerSparqlCursor *cursor;
  const gchar *uuid;
  GList *result = NULL;

  if (!needles)
    return result;

  match = g_string_new ("");

  /* AND is implicit into tracker */
  for (parser=0; needles[parser] != NULL; parser++)
  {
    match = g_string_append (match, needles[parser]);
    match = g_string_append (match, "* ");
  }

  needle = g_utf8_strdown (match->str, -1);

  query = g_strconcat (
       "SELECT ?urn WHERE {           ",
       "?urn a nfo:Note;              ",
       "nie:title ?title ;            ",
       "nie:plainTextContent ?content ",
       ". FILTER (fn:contains (fn:lower-case (?content),",
       "'", needle, "' ) ||            ",
       "fn:contains (fn:lower-case (?title), '", needle ,"'  ))}",
       NULL);

  g_string_free (match, TRUE);

  /* Go to the query */
  cursor = bjb_perform_query (self, query);

  if (!cursor)
    return result;

/* TODO
tracker_sparql_cursor_next_async    (TrackerSparqlCursor *self,
                                     GCancellable *cancellable,
                                     GAsyncReadyCallback _callback_,
                                     gpointer _user_data_);
*/

  while (tracker_sparql_cursor_next (cursor, NULL, NULL))
  {
    uuid = tracker_sparql_cursor_get_string (cursor, 0, 0);

    if (uuid)
      result = g_list_append (result, g_strdup(uuid));

  }

  g_free (query);
  g_object_unref (cursor);
  return result;
}

static void
add_string (gchar *single_result, GVariantBuilder *meta_result)
{
  g_variant_builder_add (meta_result, "s", single_result);
}

static void
handle_get_initial_result_set (BijibenShellSearchProvider2  *skeleton,
                               GDBusMethodInvocation        *invocation,
                               gchar                       **terms,
                               gpointer                      user_data)
{
  BijibenShellSearchProviderApp *self = user_data;
  GVariantBuilder builder;
  GList *result;

  g_application_hold (user_data);

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));
  result = biji_get_notes_with_strings (self, terms);
  g_list_foreach (result, (GFunc) add_string, &builder);
  g_list_free_full (result, (GDestroyNotify) g_free);

  g_dbus_method_invocation_return_value (invocation, g_variant_new ("(as)", &builder));

  g_application_release (user_data);
}

/* TODO : start from previous result , simply with GList */
static void
handle_get_subsearch_result_set (BijibenShellSearchProvider2  *skeleton,
                                 GDBusMethodInvocation        *invocation,
                                 gchar                       **previous_results,
                                 gchar                       **terms,
                                 gpointer                      user_data)
{
  BijibenShellSearchProviderApp *self = user_data;
  GVariantBuilder builder;
  GList *result;

  g_application_hold (user_data);

  g_variant_builder_init (&builder, G_VARIANT_TYPE ("as"));
  result = biji_get_notes_with_strings (self, terms);
  g_list_foreach (result, (GFunc) add_string, &builder);
  g_list_free_full (result, (GDestroyNotify) g_free);

  g_dbus_method_invocation_return_value (invocation, g_variant_new ("(as)", &builder));

  g_application_release (user_data);
}

static void
add_single_note_meta (BijibenShellSearchProviderApp *self,
                      gchar *note__id,
                      GVariantBuilder *results)
{
  gchar *query;
  const gchar *url;
  const gchar *result;
  TrackerSparqlCursor *cursor;

  query = g_strdup_printf ("SELECT nie:url(<%s>) nie:title(<%s>) WHERE { }",
                           note__id, note__id);


  cursor = bjb_perform_query (self, query);
  g_free (query);

  if (tracker_sparql_cursor_next (cursor, NULL, NULL))
  {
    g_variant_builder_open (results, G_VARIANT_TYPE ("a{sv}"));

    /* NIE:URL (id) */
    url = tracker_sparql_cursor_get_string (cursor, 0, 0);
    g_variant_builder_add (results, "{sv}", "id", g_variant_new_string (url));

    /* NIE:TITLE (name) is the title pushed by libbiji */
    result = tracker_sparql_cursor_get_string (cursor, 1, 0);
    if (result == NULL || result[0] == '\0')
      result = _("Untitled");
    g_variant_builder_add (results, "{sv}", "name", g_variant_new_string (result));
    g_variant_builder_close (results);
  }

  g_object_unref (cursor);
}

static void
handle_get_result_metas (BijibenShellSearchProvider2  *skeleton,
                         GDBusMethodInvocation        *invocation,
                         gchar                       **results,
                         gpointer                      user_data)
{
  BijibenShellSearchProviderApp *self = user_data;
  GVariantBuilder retval;
  gint parser;

  g_application_hold (user_data);

  g_variant_builder_init (&retval, G_VARIANT_TYPE ("aa{sv}"));

  for (parser=0; results[parser] != NULL; parser++)
    add_single_note_meta (self, results[parser], &retval);

  g_dbus_method_invocation_return_value (invocation, g_variant_new ("(aa{sv})", &retval));

  g_application_release (user_data);
}


static void
handle_activate_result (BijibenShellSearchProvider2  *skeleton,
                        GDBusMethodInvocation        *invocation,
                        gchar                        *result,
                        gchar                       **terms,
                        guint                         timestamp,
                        gpointer                      user_data)
{
  GError *error = NULL;
  gboolean ok;
  GAppInfo *app;
  GdkAppLaunchContext *context;
  GList *uris = NULL;

  g_application_hold (user_data);

  app = G_APP_INFO (g_desktop_app_info_new ("org.gnome.Notes.desktop"));

  context = gdk_display_get_app_launch_context (gdk_display_get_default ());
  gdk_app_launch_context_set_timestamp (context, timestamp);

  uris = g_list_prepend (uris, result);
  ok = g_app_info_launch_uris (app, uris,
                               G_APP_LAUNCH_CONTEXT (context), &error);

  if (!ok)
    {
      g_dbus_method_invocation_return_gerror (invocation, error);
      g_error_free (error);
    }
  else
    {
      bijiben_shell_search_provider2_complete_activate_result (skeleton,
                                                               invocation);
    }

  g_list_free (uris);
  g_object_unref (context);
  g_object_unref (app);

  g_application_release (user_data);
}

/* static void */
/* shell_search_load_providers_cb (GObject      *object, */
/*                                 GAsyncResult *result, */
/*                                 gpointer      user_data) */
/* { */
/*   GTask *task = user_data; */
/*   g_autoptr(GError) error = NULL; */

/*   g_assert (G_IS_TASK (task)); */

/*   biji_manager_load_providers_finish (BIJI_MANAGER (object), result, &error); */

/*   if (error) */
/*     g_warning ("Error loading providers: %s", error->message); */

/*   g_task_return_boolean (task, TRUE); */
/* } */

static void
search_provider_app_dbus_unregister (GApplication    *application,
                                     GDBusConnection *connection,
                                     const gchar     *object_path)
{
  BijibenShellSearchProviderApp *self = BIJIBEN_SHELL_SEARCH_PROVIDER_APP (application);

  if (g_dbus_interface_skeleton_has_connection (G_DBUS_INTERFACE_SKELETON (self->skeleton),
                                                connection))
    g_dbus_interface_skeleton_unexport_from_connection (G_DBUS_INTERFACE_SKELETON (self->skeleton),
                                                        connection);

  G_APPLICATION_CLASS(bijiben_shell_search_provider_app_parent_class)->dbus_unregister (application,
                                                                                        connection,
                                                                                        object_path);
}

static gboolean
search_provider_app_dbus_register (GApplication     *application,
                                   GDBusConnection  *connection,
                                   const gchar      *object_path,
                                   GError          **error)
{
  BijibenShellSearchProviderApp *self = BIJIBEN_SHELL_SEARCH_PROVIDER_APP (application);

  if (!G_APPLICATION_CLASS(bijiben_shell_search_provider_app_parent_class)->dbus_register (application,
                                                                                           connection,
                                                                                           object_path,
                                                                                           error))
    return FALSE;

  self->skeleton = bijiben_shell_search_provider2_skeleton_new ();

  g_signal_connect (self->skeleton, "handle-get-initial-result-set",
                    G_CALLBACK (handle_get_initial_result_set), self);
  g_signal_connect (self->skeleton, "handle-get-subsearch-result-set",
                    G_CALLBACK (handle_get_subsearch_result_set), self);
  g_signal_connect (self->skeleton, "handle-get-result-metas",
                    G_CALLBACK (handle_get_result_metas), self);
  g_signal_connect (self->skeleton, "handle-activate-result",
                    G_CALLBACK (handle_activate_result), self);

  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (self->skeleton),
                                           connection, object_path, error);
}

static void
search_provider_app_dispose (GObject *obj)
{
  BijibenShellSearchProviderApp *self = BIJIBEN_SHELL_SEARCH_PROVIDER_APP (obj);

  g_clear_object (&self->connection);

  G_OBJECT_CLASS (bijiben_shell_search_provider_app_parent_class)->dispose (obj);
}

static void
search_provider_app_startup (GApplication *app)
{
  G_APPLICATION_CLASS (bijiben_shell_search_provider_app_parent_class)->startup (app);

  /* hold indefinitely if we're asked to persist*/
  if (g_getenv ("BIJIBEN_SEARCH_PROVIDER_PERSIST") != NULL)
    g_application_hold (app);
}

static void
bijiben_shell_search_provider_app_init (BijibenShellSearchProviderApp *self)
{
  GError *error = NULL;
  char *storage_path;
  GFile *storage;
  /* GdkRGBA color = { 0, 0, 0, 0 }; */
  g_autoptr(GTask) task = NULL;
#ifdef TRACKER_PRIVATE_STORE
  g_autofree char *filename = NULL;
  g_autoptr (GFile) data_location = NULL;

  filename = g_build_filename (g_get_user_cache_dir (),
                               g_get_application_name (),
                               "tracker3",
                               NULL);
  data_location = g_file_new_for_path (filename);

  self->connection = tracker_sparql_connection_new (TRACKER_SPARQL_CONNECTION_FLAGS_READONLY,
                                                    data_location,
                                                    tracker_sparql_get_ontology_nepomuk (),
                                                    NULL,
                                                    &error);

#else
  self->connection = tracker_sparql_connection_get (NULL, &error);
#endif /* TRACKER_PRIVATE_STORE */

  if (error)
  {
     g_warning ("Unable to connect to Tracker: %s", error->message);
     g_clear_error (&error);
  }

  storage_path = g_build_filename (g_get_user_data_dir (), "bijiben", NULL);
  storage = g_file_new_for_path (storage_path);
  g_free (storage_path);

  /* self->manager = biji_manager_new (storage, &color); */
  g_object_unref (storage);

  task = g_task_new (self, NULL, NULL, NULL);
  /* biji_manager_load_providers_async (self->manager, shell_search_load_providers_cb, task); */

  /* Wait until the task is completed */
  while (!g_task_get_completed (task))
    g_main_context_iteration (NULL, TRUE);

  if (error)
  {
     g_warning ("Unable to create BijiManager: %s", error->message);
     g_clear_error (&error);
  }
}

static void
bijiben_shell_search_provider_app_class_init (BijibenShellSearchProviderAppClass *klass)
{
  GApplicationClass *aclass = G_APPLICATION_CLASS (klass);
  GObjectClass *oclass = G_OBJECT_CLASS (klass);

  aclass->startup = search_provider_app_startup;
  aclass->dbus_register = search_provider_app_dbus_register;
  aclass->dbus_unregister = search_provider_app_dbus_unregister;
  oclass->dispose = search_provider_app_dispose;
}

static GApplication *
bijiben_shell_search_provider_app_new (void)
{
  return g_object_new (bijiben_shell_search_provider_app_get_type (),
                       "application-id", "org.gnome.Notes.SearchProvider",
                       "flags", G_APPLICATION_IS_SERVICE,
                       "inactivity-timeout", SEARCH_PROVIDER_INACTIVITY_TIMEOUT,
                       NULL);
}

int
main (int   argc,
      char *argv[])
{
  GApplication *app;
  gint res;

  gtk_init ();

  app = bijiben_shell_search_provider_app_new ();
  res = g_application_run (app, argc, argv);
  g_object_unref (app);

  return res;
}


