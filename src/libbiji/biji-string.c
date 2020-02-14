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

/**
 * bjb_strjoinv: based on g_strjoinv
 * @separator: (nullable): a string to insert between each of the
 *     strings, or %NULL
 * @str_array: a %NULL-terminated array of strings to join
 * @max_tokens: the maximum number of pieces to join from @str_array.
 *     If this is less than 1, @str_array is joined completely.
 *
 * Joins a number of strings together to form one long string, with the
 * optional @separator inserted between each of them. The returned string
 * should be freed with g_free().
 *
 * If @str_array has no items, the return value will be an
 * empty string. If @str_array contains a single item, @separator will not
 * appear in the resulting string.
 *
 * Returns: a newly-allocated string containing the strings joined
 *     together, with @separator between them.
 */
char *
bjb_strjoinv (const char  *separator,
              char       **str_array,
              gint         max_tokens)
{
  char  *string;
  char  *ptr;
  gsize  i;
  gsize  len;
  gsize  separator_len;

  g_return_val_if_fail (str_array != NULL, NULL);

  if (*str_array)
    {
      if (max_tokens < 1)
        {
          max_tokens = g_strv_length (str_array);
        }

      if (separator == NULL)
        {
          separator = "";
        }
      separator_len = strlen (separator);

      /* First part, getting length */
      len = 1 + strlen (str_array[0]);
      for (i = 1; str_array[i] != NULL && i < max_tokens; i++)
        {
          len += strlen (str_array[i]);
        }
      len += separator_len * (i - 1);

      /* Second part, building string */
      string = g_new (gchar, len);
      ptr = g_stpcpy (string, *str_array);
      for (i = 1; str_array[i] != NULL && i < max_tokens; i++)
        {
          ptr = g_stpcpy (ptr, separator);
          ptr = g_stpcpy (ptr, str_array[i]);
        }
      }
  else
    {
      string = g_strdup ("");
    }

  return string;
}


