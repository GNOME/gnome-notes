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

#pragma once

#include <glib-object.h>
#include <glib/gi18n.h>  // translate providers type

#include "biji-info-set.h"
#include "biji-manager.h"

G_BEGIN_DECLS

#define BIJI_TYPE_PROVIDER (biji_provider_get_type ())

G_DECLARE_DERIVABLE_TYPE (BijiProvider, biji_provider, BIJI, PROVIDER, GObject)


typedef struct
{
  const gchar     *unique_id;  // anything unique, eg, goa_account_get_id
  const gchar     *datasource; // for tracker

  gchar           *name;       // eg, goa_account_get_provider_name
  GtkWidget       *icon;

  gchar           *domain;     // todo - distinguish several accounts
  gchar           *user;       // todo - distinguish several accounts

} BijiProviderInfo;

struct _BijiProviderClass
{
  GObjectClass parent_class;


/* Each Provider subclass instance owns its own BijiProviderInfo
 * to signal name, icon, or whatever useful info.
 * datasource is only needed for actual providers,
 * ie. persistent data source */

  const BijiProviderInfo*    (*get_info)              (BijiProvider *provider);


  /* When a provider is loaded, notify the manager to transmit the items */

  void                       (*notify_loaded)         (BijiProvider *provider,
                                                       GList *loaded_items,
                                                       BijiItemsGroup group);


  /* When created, the provider is supposed to load the items.
   * Loading archives might or might not happen at creation time.
   * This function ensures the provider loads its archived items.
   * The provider will notify when done */

  void                       (*load_archives)        (BijiProvider *provider);




  /* Create a single note and let the provider handle things.
   * Only works from raw text.
   * Does not allow to trick color or dates */

  BijiNoteObj*               (*create_new_note)       (BijiProvider *provider,
                                                       const gchar  *content);

  /* Creates a single note representing some existing data,
   * with title, content, dates, html, color.
   * The caller should set as many fields as possible.
   * The provider should ensure as much as possible to handle
   * NULL values properly, if any.
   * Providers will discard assigned values they do not handle.
   * Local provider is supposed to handle everything.
   *
   * Suggested path : if possible, the provider should preserve
   * the file name. If an import provider is asked to import a file,
   * first check the item does not exist yet
   * with same path (basename). ie. do not import twice the same note.
   *
   * the info set is not supposed to store any url.
   * otherwise, you might expect it to be lost.
   *
   * TODO: rebase startup code on local provider create note full when good enough
   * TODO: owncloud provider (to handle importing) */

  BijiNoteObj*               (*create_note_full)      (BijiProvider  *provider,
                                                       const gchar   *suggested_path,
                                                       BijiInfoSet   *info,
                                                       const gchar   *html,
                                                       const GdkRGBA *color);


   /* TODO : (*create_notebook). Add a flag into provider info? */
};



/* BijiProviderHelper is for convenience for callbacks
   atm it is used by local provider
 */


typedef struct
{
  BijiProvider *provider;
  BijiItemsGroup group;
} BijiProviderHelper;

BijiProviderHelper*        biji_provider_helper_new           (BijiProvider *provider,
                                                               BijiItemsGroup group);


void                       biji_provider_helper_free          (BijiProviderHelper *helper);



void                       biji_provider_load_archives        (BijiProvider *provider);



BijiManager              *biji_provider_get_manager             (BijiProvider *provider);


const BijiProviderInfo    *biji_provider_get_info             (BijiProvider *provider);


void                       biji_provider_abort                (BijiProvider *provider);

G_END_DECLS
