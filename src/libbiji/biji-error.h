/*
 * biji-error.h
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

#ifndef _BIJI_ERROR_H
#define _BIJI_ERROR_H


#include <gio/gio.h>


G_BEGIN_DECLS


typedef enum
{
  BIJI_ERROR_TRACKER,           /* org.gnome.Biji.Error.Tracker */
  BIJI_ERROR_SOURCE
} BijiErrorType;



#define BIJI_ERROR_NUM_ENTRIES  (BIJI_ERROR_TRACKER + 1)


#define BIJI_ERROR (biji_error_quark ())


GQuark               biji_error_quark                       (void);


GError              *biji_error_new                        (BijiErrorType type,
                                                            gchar *message);


G_END_DECLS

#endif /* _BIJI_ERROR_H */
