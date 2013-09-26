/* biji-date-time.h
 * Copyright (C) Pierre-Yves LUYTEN 2011-2013 <py@luyten.fr>
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

#ifndef _BIJI_DATE_TIME_H
#define _BIJI_DATE_TIME_H


#include <glib-object.h>
#include <glib/gprintf.h>



gchar            *biji_get_time_diff_with_time            (glong sec_since_epoch);



gint64            iso8601_to_gint64                       (gchar *iso8601);


#endif /* _BIJI_DATE_TIME_H */
