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

#include "libbiji.h"

const gchar *
biji_get_time_diff_with_time (glong sec_since_epoch)
{
  GDate *now = g_date_new ();
  GDate *date = g_date_new ();
  gchar *str;
  gint   diff;

  g_return_val_if_fail (sec_since_epoch >= 0, _("Unknown"));

  g_date_set_time_t (date, sec_since_epoch);
  g_date_set_time_t (now, time (NULL));
  diff = g_date_days_between (date, now);

  if (diff == 0)
    {
      str = _("Today");
    }
  else if (diff == 1)
    {
      str = _("Yesterday");
    }
  else if (diff < 7 &&
           g_date_get_weekday (date) < g_date_get_weekday (now))
    {
      str = _("This week");
    }
  else if (diff < 0 || g_date_get_year (date) != g_date_get_year (now))
    {
      str = _("Unknown");
    }
  else if (diff < 31 &&
           g_date_get_month (date) == g_date_get_month (now))
    {
      str = _("This month");
    }
  else
    {
      str = _("This year");
    }

  g_date_free (date);
  g_date_free (now);

  return str;
}



gint64
iso8601_to_gint64 (gchar *iso8601)
{
  GTimeVal time = {0,0};

  g_time_val_from_iso8601 (iso8601, &time);
  return (gint64) time.tv_sec;
}
