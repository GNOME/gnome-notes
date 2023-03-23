/* biji-date-time.c
 * Copyright (C) Pierre-Yves LUYTEN 2011 <py@luyten.fr>
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include <glib/gi18n.h>

#include "biji-date-time.h"

gint64
iso8601_to_gint64 (const char *iso8601)
{
  g_autoptr(GDateTime) dt = g_date_time_new_from_iso8601 (iso8601, NULL);
  if (dt == NULL)
    {
      return 0;
    }

  return g_date_time_to_unix (dt);
}
