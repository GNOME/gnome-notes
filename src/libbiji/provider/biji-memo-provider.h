/*
 * biji-memo-provider.h
 * Copyright (C) Pierre-Yves LUYTEN 2014 <py@luyten.fr>
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

#include <libedataserver/libedataserver.h> /* ESourceRegistry */
#include <libecal/libecal.h>               /* ECalClient      */

#include "biji-manager.h"
#include "biji-provider.h"



G_BEGIN_DECLS

#define BIJI_TYPE_MEMO_PROVIDER (biji_memo_provider_get_type ())

G_DECLARE_FINAL_TYPE (BijiMemoProvider, biji_memo_provider, BIJI, MEMO_PROVIDER, BijiProvider)

BijiProvider   *biji_memo_provider_new          (BijiManager *manager,
                                                 ESource     *source);



ICalTime *      icaltime_from_time_val          (glong t);


gboolean        time_val_from_icaltime          (ICalTime *itt,
                                                 glong *result);


G_END_DECLS
