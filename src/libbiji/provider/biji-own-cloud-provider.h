/* bjb-own-cloud-provider.h
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


#ifndef BIJI_OWN_CLOUD_PROVIDER_H_
#define BIJI_OWN_CLOUD_PROVIDER_H_ 1

#include "biji-provider.h"

G_BEGIN_DECLS


#define BIJI_TYPE_OWN_CLOUD_PROVIDER             (biji_own_cloud_provider_get_type ())
#define BIJI_OWN_CLOUD_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_OWN_CLOUD_PROVIDER, BijiOwnCloudProvider))
#define BIJI_OWN_CLOUD_PROVIDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_OWN_CLOUD_PROVIDER, BijiOwnCloudProviderClass))
#define BIJI_IS_OWN_CLOUD_PROVIDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_OWN_CLOUD_PROVIDER))
#define BIJI_IS_OWN_CLOUD_PROVIDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_OWN_CLOUD_PROVIDER))
#define BIJI_OWN_CLOUD_PROVIDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_OWN_CLOUD_PROVIDER, BijiOwnCloudProviderClass))

typedef struct BijiOwnCloudProvider_         BijiOwnCloudProvider;
typedef struct BijiOwnCloudProviderClass_    BijiOwnCloudProviderClass;
typedef struct BijiOwnCloudProviderPrivate_  BijiOwnCloudProviderPrivate;

struct BijiOwnCloudProvider_
{
  BijiProvider parent;
  BijiOwnCloudProviderPrivate *priv;
};


struct BijiOwnCloudProviderClass_
{
  BijiProviderClass parent_class;
};


GType             biji_own_cloud_provider_get_type      (void);



BijiProvider     *biji_own_cloud_provider_new           (BijiManager *manager,
                                                         GoaObject *object);


GFile            *biji_own_cloud_provider_get_folder    (BijiOwnCloudProvider *provider);


gchar            *biji_own_cloud_provider_get_readable_path (BijiOwnCloudProvider *p);


G_END_DECLS

#endif /* BIJI_OWN_CLOUD_PROVIDER_H_ */
