/* bjb-provider.c
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


/*
 * TODO: the generic provider should at least
 * be the one emiting the signals
 * 
 * So 1. for each provider, the book connects to the same
 * & provider does not reinvent anything
 * 2. it might be a right place for running the approriate
 * threads - or even processes.
 * 
 */

#include "biji-provider.h"


struct BijiProviderPrivate_
{
  gpointer delete_me;
};

G_DEFINE_TYPE (BijiProvider, biji_provider, G_TYPE_OBJECT)





static void
biji_provider_finalize (GObject *object)
{
  //BijiProvider *self;

  //g_return_if_fail (BIJI_IS_PROVIDER (object));

  //self = BIJI_PROVIDER (object);

  G_OBJECT_CLASS (biji_provider_parent_class)->finalize (object);
}


static void
biji_provider_init (BijiProvider *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_PROVIDER, BijiProviderPrivate);
}


static void
biji_provider_class_init (BijiProviderClass *klass)
{
  GObjectClass *g_object_class;

  g_object_class = G_OBJECT_CLASS (klass);

  g_object_class->finalize = biji_provider_finalize;

  g_type_class_add_private ((gpointer)klass, sizeof (BijiProviderPrivate));
}
