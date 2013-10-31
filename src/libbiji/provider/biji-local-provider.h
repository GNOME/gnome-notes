/*
 * biji-local-provider.h
 * Copyright (C) Pierre-Yves LUYTEN 2013 <py@luyten.fr>
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



#ifndef BIJI_LOCAL_PROVIDER_H_
#define BIJI_LOCAL_PROVIDER_H_ 1


#include "../biji-manager.h"
#include "biji-provider.h"


G_BEGIN_DECLS


#define BIJI_TYPE_LOCAL_PROVIDER             (biji_local_provider_get_type ())
#define BIJI_LOCAL_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_LOCAL_PROVIDER, BijiLocalProvider))
#define BIJI_LOCAL_PROVIDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_LOCAL_PROVIDER, BijiLocalProviderClass))
#define BIJI_IS_LOCAL_PROVIDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_LOCAL_PROVIDER))
#define BIJI_IS_LOCAL_PROVIDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_LOCAL_PROVIDER))
#define BIJI_LOCAL_PROVIDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_LOCAL_PROVIDER, BijiLocalProviderClass))

typedef struct BijiLocalProvider_         BijiLocalProvider;
typedef struct BijiLocalProviderClass_    BijiLocalProviderClass;
typedef struct BijiLocalProviderPrivate_  BijiLocalProviderPrivate;


struct BijiLocalProvider_
{
  BijiProvider parent;
  BijiLocalProviderPrivate *priv;
};


struct BijiLocalProviderClass_
{
  BijiProviderClass parent_class;
};


GType                biji_local_provider_get_type                    (void);


BijiProvider        *biji_local_provider_new                         (BijiManager *manager,
                                                                      GFile *location);


G_END_DECLS

#endif /* BIJI_LOCAL_PROVIDER_H_ */
