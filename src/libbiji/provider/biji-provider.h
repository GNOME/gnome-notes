/* bjb-provider.h
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

#ifndef BIJI_PROVIDER_H_
#define BIJI_PROVIDER_H_ 1

#include <glib-object.h>
#include <glib/gi18n.h>  // translate providers type

#include "../biji-note-book.h"

G_BEGIN_DECLS


#define BIJI_TYPE_PROVIDER             (biji_provider_get_type ())
#define BIJI_PROVIDER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_PROVIDER, BijiProvider))
#define BIJI_PROVIDER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_PROVIDER, BijiProviderClass))
#define BIJI_IS_PROVIDER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_PROVIDER))
#define BIJI_IS_PROVIDER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_PROVIDER))
#define BIJI_PROVIDER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_PROVIDER, BijiProviderClass))

typedef struct BijiProvider_         BijiProvider;
typedef struct BijiProviderClass_    BijiProviderClass;
typedef struct BijiProviderPrivate_  BijiProviderPrivate;


typedef struct
{
  const gchar     *unique_id;  // anything unique, eg, goa_account_get_id
  const gchar     *datasource; // for tracker

  gchar           *name;       // eg, goa_account_get_provider_name
  GtkWidget       *icon;

  gchar           *domain;     // todo - distinguish several accounts
  gchar           *user;       // todo - distinguish several accounts

} BijiProviderInfo;


struct BijiProvider_
{
  GObject parent;
  BijiProviderPrivate *priv;
};


struct BijiProviderClass_
{
  GObjectClass parent_class;


  const BijiProviderInfo*    (*get_info)              (BijiProvider *provider);
  

  void                       (*notify_loaded)         (BijiProvider *provider,
                                                       GList *loaded_items);


  BijiNoteObj*               (*create_note)           (BijiProvider *provider,
                                                       gchar        *content);
};





GType                      biji_provider_get_type             (void);


BijiNoteBook              *biji_provider_get_book             (BijiProvider *provider);


const BijiProviderInfo    *biji_provider_get_info             (BijiProvider *provider);

G_END_DECLS

#endif /* BIJI_PROVIDER_H_ */
