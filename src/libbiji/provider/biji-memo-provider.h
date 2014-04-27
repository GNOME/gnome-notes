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

#ifndef _BIJI_MEMO_PROVIDER_H_
#define _BIJI_MEMO_PROVIDER_H_ 1


#include <libedataserver/libedataserver.h> /* ESourceRegistry */
#include <libecal/libecal.h>               /* ECalClient      */

#include "../biji-manager.h"
#include "biji-provider.h"



G_BEGIN_DECLS

#define BIJI_TYPE_MEMO_PROVIDER             (biji_memo_provider_get_type ())
#define BIJI_MEMO_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_MEMO_PROVIDER, BijiMemoProvider))
#define BIJI_MEMO_PROVIDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_MEMO_PROVIDER, BijiMemoProviderClass))
#define BIJI_IS_MEMO_PROVIDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_MEMO_PROVIDER))
#define BIJI_IS_MEMO_PROVIDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_MEMO_PROVIDER))
#define BIJI_MEMO_PROVIDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_MEMO_PROVIDER, BijiMemoProviderClass))

typedef struct _BijiMemoProviderClass BijiMemoProviderClass;
typedef struct _BijiMemoProvider BijiMemoProvider;
typedef struct _BijiMemoProviderPrivate BijiMemoProviderPrivate;




struct _BijiMemoProvider
{
  BijiProvider             parent_instance;
  BijiMemoProviderPrivate *priv;
};



struct _BijiMemoProviderClass
{
  BijiProviderClass parent_class;
};



GType           biji_memo_provider_get_type     (void) G_GNUC_CONST;


BijiProvider   *biji_memo_provider_new          (BijiManager *manager,
                                                 ESource     *source);



gboolean        icaltime_from_time_val          (glong          t,
                                                 icaltimetype *out);


gboolean        time_val_from_icaltime          (icaltimetype *itt,
                                                 glong *result);


G_END_DECLS

#endif /* _BIJI_MEMO_PROVIDER_H_ */

