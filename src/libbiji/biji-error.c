/*
 * biji-error.c
 *
 * Copyright 2013 Pierre-Yves Luyten <py@luyten.fr>
 *
 * Bijiben is free software: you can redistribute it and/or modify it
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

#include "biji-error.h"




static const GDBusErrorEntry dbus_error_entries[] =
{
  {BIJI_ERROR_TRACKER, "org.gnome.Biji.Error.Tracker"}
};



GQuark
biji_error_quark (void)
{
  G_STATIC_ASSERT (G_N_ELEMENTS (dbus_error_entries) == BIJI_ERROR_NUM_ENTRIES);

  static volatile gsize quark_volatile = 0;
  g_dbus_error_register_error_domain ("biji-error-quark",
                                      &quark_volatile,
                                      dbus_error_entries,
                                      G_N_ELEMENTS (dbus_error_entries));

  return (GQuark) quark_volatile;
}


GError *
biji_error_new (BijiErrorType type,
                gchar *message)
{
  return g_error_new_literal (BIJI_ERROR, type, message);
}
