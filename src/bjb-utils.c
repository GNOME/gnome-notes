/* gn-utils.c
 *
 * Copyright 2019 Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s):
 *   Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <glib/gi18n.h>
#include "bjb-utils.h"

/**
 * bjb_utils_get_human_time:
 * @unix_time: seconds since Epoch
 *
 * Get a human readable representation of the time
 * @unix_time.
 *
 * The time returned isn’t always in the same format.
 * Say if @unix_time represents the current day,
 * a string with time like “07:30” is returned.
 * Else if @unix_time represents a preceding day from
 * the same week the weekday name is returned, and so on.
 *
 * Returns: A new string.  Free with g_free().
 */
gchar *
bjb_utils_get_human_time (gint64 unix_time)
{
  g_autoptr(GDateTime) now = NULL;
  g_autoptr(GDateTime) utc_time = NULL;
  g_autoptr(GDateTime) local_time = NULL;
  gint year_now, month_now, day_now;
  gint year, month, day;

  g_return_val_if_fail (unix_time >= 0, g_strdup (_("Unknown")));

  now = g_date_time_new_now_local ();
  utc_time = g_date_time_new_from_unix_utc (unix_time);
  local_time = g_date_time_to_local (utc_time);

  g_date_time_get_ymd (now, &year_now, &month_now, &day_now);
  g_date_time_get_ymd (local_time, &year, &month, &day);

  if (year == year_now &&
      month == month_now)
    {
      /* Time in the format HH:MM */
      if (day == day_now)
        return g_date_time_format (local_time, "%R");

      if (day_now - day == 1)
        return g_strdup (_("Yesterday"));

      /* Localized day name */
      if (day_now - day <= 7)
        return g_date_time_format (local_time, "%A");

      return g_strdup (_("This month"));
    }

  /* Localized month name */
  if (year == year_now)
    return g_date_time_format (local_time, "%OB");

  /* Year */
  return g_date_time_format (local_time, "%Y");
}
