/* biji-string.h
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

#pragma once

#include <glib.h>

G_BEGIN_DECLS

/* Replaces inside string the as_is with to_be
 * Returns a newly allocated string */
gchar * biji_str_replace (const gchar *string, const gchar *as_is, const gchar *to_be);

/* Calls biji_str_replace as much as there are paired gchar* args
 * Returns a newly allocated string */
gchar * biji_str_mass_replace (const gchar *string, ...) G_GNUC_NULL_TERMINATED ;

/* Cleans extra spaces, including tabs and no-break spaces from strings
 * Returns a newly allocated string */
char *biji_str_clean (const char *string);

G_END_DECLS
