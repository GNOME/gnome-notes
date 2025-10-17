/* bjb-manager.c
 *
 * Copyright 2012 Pierre-Yves LUYTEN <py@luyten.fr>
 * Copyright 2023 Mohammed Sadiq <sadiq@sadiqpk.org>
 * Copyright 2023 Purism SPC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s):
 *   Mohammed Sadiq <sadiq@sadiqpk.org>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#define G_LOG_DOMAIN "bjb-manager"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <goa/goa.h>
#include <libedataserver/libedataserver.h>
#include <libecal/libecal.h>

#include "items/bjb-plain-note.h"
#include "providers/bjb-provider.h"
#include "providers/bjb-local-provider.h"
#include "providers/bjb-memo-provider.h"
#include "providers/bjb-nc-provider.h"
#include "bjb-manager.h"

struct _BjbManager
{
  GObject              parent_instance;

  ESourceRegistry     *eds_registry;
  GoaClient           *goa_client;
  BjbProvider         *local_provider;
  GListStore          *providers;

  GListStore          *list_of_notes;
  GtkFlattenListModel *notes;

  gboolean             is_loading;
  gboolean             loaded;
};


G_DEFINE_TYPE (BjbManager, bjb_manager, G_TYPE_OBJECT)

enum {
  ITEM_REMOVED,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

static void
manager_provider_item_removed_cb (BjbManager  *self,
                                  BjbItem     *item,
                                  BjbProvider *provider)
{
  g_assert (BJB_IS_MANAGER (self));
  g_assert (BJB_IS_ITEM (item));
  g_assert (BJB_IS_PROVIDER (provider));

  g_signal_emit (self, signals[ITEM_REMOVED], 0, provider, item);
}

static void
manager_memo_provider_connect_cb (GObject      *object,
                                  GAsyncResult *result,
                                  gpointer      user_data)
{
  g_autoptr(BjbManager) self = user_data;
  g_autoptr(GError) error = NULL;

  bjb_provider_connect_finish (BJB_PROVIDER (object), result, &error);

  if (error)
    g_warning ("Error loading provider: %s", error->message);
}

static void
manager_nextcloud_provider_connect_cb (GObject      *object,
                                       GAsyncResult *result,
                                       gpointer      user_data)
{
  g_autoptr(BjbManager) self = user_data;
  g_autoptr(GError) error = NULL;

  bjb_provider_connect_finish (BJB_PROVIDER (object), result, &error);

  if (error)
    g_warning ("Error loading provider: %s", error->message);
}

static void
load_eds_cb (GObject      *object,
             GAsyncResult *result,
             gpointer      user_data)
{
  g_autoptr(BjbManager) self = user_data;
  g_autoptr(GError) error = NULL;
  GList *sources;

  self->eds_registry = e_source_registry_new_finish (result, &error);
  sources = e_source_registry_list_sources (self->eds_registry,
                                            E_SOURCE_EXTENSION_MEMO_LIST);
  for (GList *node = sources; node != NULL; node = node->next)
    {
      g_autoptr(BjbProvider) provider = NULL;

      provider = bjb_memo_provider_new (node->data);
      g_signal_connect_object (provider, "item-removed",
                               G_CALLBACK (manager_provider_item_removed_cb),
                               self, G_CONNECT_SWAPPED);
      g_list_store_append (self->providers, provider);
      g_list_store_append (self->list_of_notes,
                           bjb_provider_get_notes (provider));

      bjb_provider_connect_async (provider,
                                  NULL,
                                  manager_memo_provider_connect_cb,
                                  g_object_ref (self));
    }

  g_list_free_full (sources, g_object_unref);
}

static void
manager_goa_account_added_cb (BjbManager *self,
                              GoaObject  *object,
                              GoaClient  *client)
{
  GoaAccount *account;
  const char *type;

  g_assert (BJB_IS_MANAGER (self));
  g_assert (GOA_IS_CLIENT (client));
  g_assert (GOA_IS_OBJECT (object));

  account = goa_object_peek_account (object);
  type = goa_account_get_provider_type (account);

  if (g_strcmp0 (type, "owncloud") == 0 &&
      !goa_account_get_calendar_disabled (account))
    {
      g_autoptr(BjbProvider) provider = NULL;

      provider = bjb_nc_provider_new (object);
      g_signal_connect_object (provider, "item-removed",
                               G_CALLBACK (manager_provider_item_removed_cb),
                               self, G_CONNECT_SWAPPED);
      g_list_store_append (self->providers, provider);
      g_list_store_append (self->list_of_notes,
                           bjb_provider_get_notes (provider));
      bjb_provider_connect_async (provider,
                                  NULL,
                                  manager_nextcloud_provider_connect_cb,
                                  g_object_ref (self));
    }
}

static void
manager_goa_account_removed_cb (BjbManager *self,
                                GoaObject  *object,
                                GoaClient  *client)
{
  GListModel *providers;
  guint n_items;

  g_assert (BJB_IS_MANAGER (self));
  g_assert (GOA_IS_CLIENT (client));
  g_assert (GOA_IS_OBJECT (object));

  providers = G_LIST_MODEL (self->providers);
  n_items = g_list_model_get_n_items (providers);

  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(BjbProvider) provider = NULL;

      provider = g_list_model_get_item (providers, i);

      if (BJB_IS_NC_PROVIDER (provider) &&
          bjb_nc_provider_matches_goa (BJB_NC_PROVIDER (provider), object))
        {
          guint position;

          if (g_list_store_find (self->providers, provider, &position))
            g_list_store_remove (self->providers, position);
          break;
        }
    }
}

static void
load_goa_cb (GObject      *object,
             GAsyncResult *result,
             gpointer      user_data)
{
  g_autoptr(BjbManager) self = user_data;
  g_autoptr(GError) error = NULL;
  GList *accounts;

  self->goa_client = goa_client_new_finish (result, &error);

  if (error)
    {
      g_warning ("Failed loading goa: %s", error->message);
      return;
    }

  g_signal_connect_object (self->goa_client, "account-added",
                           G_CALLBACK (manager_goa_account_added_cb),
                           self,
                           G_CONNECT_SWAPPED);
  g_signal_connect_object (self->goa_client, "account-removed",
                           G_CALLBACK (manager_goa_account_removed_cb),
                           self,
                           G_CONNECT_SWAPPED);

  accounts = goa_client_get_accounts (self->goa_client);

  for (GList *node = accounts; node; node = node->next)
    manager_goa_account_added_cb (self, node->data, self->goa_client);

  g_list_free_full (accounts, g_object_unref);
}

static void
manager_local_provider_connect_cb (GObject      *object,
                                   GAsyncResult *result,
                                   gpointer      user_data)
{
  g_autoptr(BjbManager) self = user_data;

  g_list_store_append (self->providers, self->local_provider);
  g_list_store_append (self->list_of_notes,
                       bjb_provider_get_notes (self->local_provider));
  g_list_store_append (self->list_of_notes,
                       bjb_provider_get_trash_notes (self->local_provider));
}

static void
bjb_manager_finalize (GObject *object)
{
  BjbManager *self = (BjbManager *)object;

  g_clear_object (&self->list_of_notes);
  g_clear_object (&self->notes);

  g_clear_object (&self->eds_registry);
  g_clear_object (&self->goa_client);
  g_clear_object (&self->local_provider);
  g_clear_object (&self->providers);

  G_OBJECT_CLASS (bjb_manager_parent_class)->finalize (object);
}

static void
bjb_manager_class_init (BjbManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bjb_manager_finalize;

  signals [ITEM_REMOVED] =
    g_signal_new ("item-removed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 2,
                  BJB_TYPE_PROVIDER, BJB_TYPE_ITEM);

}

static void
bjb_manager_init (BjbManager *self)
{
  self->providers = g_list_store_new (BJB_TYPE_PROVIDER);
  self->local_provider = bjb_local_provider_new (NULL);
  g_signal_connect_object (self->local_provider, "item-removed",
                           G_CALLBACK (manager_provider_item_removed_cb),
                           self, G_CONNECT_SWAPPED);

  self->list_of_notes = g_list_store_new (G_TYPE_LIST_MODEL);
  self->notes = gtk_flatten_list_model_new (g_object_ref (G_LIST_MODEL (self->list_of_notes)));
}

BjbManager *
bjb_manager_get_default (void)
{
  static BjbManager *self;

  if (!self)
    g_set_weak_pointer (&self, g_object_new (BJB_TYPE_MANAGER, NULL));

  return self;
}

void
bjb_manager_load (BjbManager *self)
{
  g_return_if_fail (BJB_IS_MANAGER (self));

  if (self->loaded || self->is_loading)
    return;

  self->is_loading = TRUE;

  g_debug ("Loading Providers");
  e_source_registry_new (NULL, load_eds_cb, g_object_ref (self));
  goa_client_new (NULL, load_goa_cb, g_object_ref (self));

  bjb_provider_connect_async (self->local_provider,
                              NULL,
                              manager_local_provider_connect_cb,
                              g_object_ref (self));
}

GListModel *
bjb_manager_get_providers (BjbManager *self)
{
  g_return_val_if_fail (BJB_IS_MANAGER (self), NULL);

  return G_LIST_MODEL (self->providers);
}


GListModel *
bjb_manager_get_notes (BjbManager *self)
{
  g_return_val_if_fail (BJB_IS_MANAGER (self), NULL);

  return G_LIST_MODEL (self->notes);
}
