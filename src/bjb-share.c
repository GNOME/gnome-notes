/* biji-share.c
 * Copyright (C) Pierre-Yves LUYTEN 2012 <py@luyten.fr>
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

#include "bjb-share.h"
#include <stdio.h>

static gchar *
mail_str (const gchar * string )
{
  if (!string)
    return g_strdup ("''");

  return g_strdelimit (g_strdup (string), "\n", ' ');
}

gboolean
on_email_note_callback (BjbNote *note)
{
  GError *error = NULL;
  g_autofree gchar *title_mail = NULL;
  g_autofree gchar *text_mail = NULL;
  g_autofree char *content = NULL;
  g_autoptr(GDBusProxy) proxy = NULL;
  GVariantBuilder *arraybuilder;
  GVariant *dict;

  title_mail = mail_str (bjb_item_get_title (BJB_ITEM (note)));

  content = bjb_note_get_raw_content (note);

  if (!content)
    content = bjb_note_get_text_content (note);

  text_mail = mail_str (content);

  proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NULL,
                                         "org.freedesktop.portal.Desktop",
                                         "/org/freedesktop/portal/desktop",
                                         "org.freedesktop.portal.Email",
                                         NULL,
                                         &error);

  if (proxy == NULL)
    {
      fprintf (stderr, "Proxy creation failed: %s\n", error->message);
      g_error_free (error);
      return FALSE;
    }

  arraybuilder = g_variant_builder_new (G_VARIANT_TYPE ("a{sv}"));
  g_variant_builder_add (arraybuilder, "{sv}", "address", g_variant_new_string ("to@email.com"));
  g_variant_builder_add (arraybuilder, "{sv}", "subject", g_variant_new_string (title_mail));
  g_variant_builder_add (arraybuilder, "{sv}", "body", g_variant_new_string (text_mail));

  dict = g_variant_new ("(sa{sv})", "", arraybuilder);
  g_variant_builder_unref (arraybuilder);

  g_dbus_proxy_call_sync (proxy, "ComposeEmail", dict, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);
  if (error != NULL)
    {
      g_error ("%s", error->message);
      fprintf (stderr, "ComposeEmail portal call failed: %s", error->message);
      g_error_free (error);
      return FALSE;
    }

  return TRUE;
}

