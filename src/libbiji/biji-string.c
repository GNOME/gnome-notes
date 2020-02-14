/* biji-string.c
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


#include "biji-string.h"

gchar *
biji_str_replace (const gchar *string,
                  const gchar *as_is,
                  const gchar *to_be)
{
  gchar **array;
  gchar *result = NULL;

  if (!string)
    return NULL;

  if (!as_is)
    return g_strdup (string);

  if (!to_be)
    return g_strdup (string);

  array = g_strsplit( string, as_is, -1);

  if (array)
  {
    result = g_strjoinv (to_be, array);
    g_strfreev (array);
  }

  return result;
}

gchar * biji_str_mass_replace (const gchar *string,
                               ...)
{
  va_list args;
  gchar *result = g_strdup (string);
  gchar *tmp;
  gchar *as_is = NULL;
  gchar *to_be = NULL;

  va_start (args, string);
  as_is = va_arg (args, gchar*);

  while (as_is)
  {
    to_be = va_arg (args, gchar*);

    if (to_be)
    {
      tmp = biji_str_replace ((const gchar*) result, as_is, to_be);

      if (tmp)
      {
        g_free (result);
        result = tmp;
      }

      as_is = va_arg (args, gchar*);
    }

    else
      as_is = NULL;
  }

  va_end (args);
  return result;
}

char *
biji_str_clean (const char *text)
{
  char *result = biji_str_replace ((const char *) text, "\t", " ");
  char *tmp    = biji_str_replace ((const char *) result, "\u00A0", " ");

  if (tmp)
    {
      g_free (result);
      result = tmp;
    }

  while (g_strstr_len (result, -1, "  "))
    {
      tmp = biji_str_replace ((const char *) result, "  ", " ");
      if (tmp)
        {
          g_free (result);
          result = tmp;
        }
    }
  return result;
}


