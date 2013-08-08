/* biji-info-set.h
 * Copyright (C) Pierre-Yves LUYTEN 2013 <py@luyten.fr>
 * 
 * libbiji is free software: you can redistribute it and/or modify it
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


/*
 * All fields come from tracker.
 * These fields are enough to create an object.
 * Ideally, this would store _all_ the serialized data
 * for local notes : bz #701828
 *
 * Ideally, this is also the cache system
 */


#ifndef _BIJI_INFO_SET_H_
#define _BIJI_INFO_SET_H_ 1


#include <glib-object.h>


typedef struct
{
  /* core (ie, cache). All but content are common to bijiIem. */
  gchar    *url;
  gchar    *title;
  gint64    mtime;
  gchar    *content;
  gint64    created;


  /* only when ensuring ressource */
  gchar    *datasource_urn;


  /* only infoset H callback when retrieving urn */
  gchar    *tracker_urn;


  /* deprecated */
  gpointer  user_data;

} BijiInfoSet;


BijiInfoSet * biji_info_set_new ();

void biji_info_set_free (BijiInfoSet *info);

#endif /* _BIJI_NOTE_ID_H_ */
