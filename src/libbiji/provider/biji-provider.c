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
 * So 1. for each provider, the manager connects to the same
 * & provider does not reinvent anything
 * 2. it might be a right place for running the approriate
 * threads - or even processes.
 * 
 */

#include "biji-marshalers.h"

#include "biji-provider.h"


/* Properties */
enum {
  PROP_0,
  PROP_BOOK,
  PROVIDER_PROP
};


/* Signals */
enum {
  PROVIDER_LOADED,
  PROVIDER_ABORT,
  PROVIDER_SIGNALS
};


static guint biji_provider_signals[PROVIDER_SIGNALS] = { 0 };
static GParamSpec *properties[PROVIDER_PROP] = { NULL, };


struct BijiProviderPrivate_
{
  BijiManager        *manager;
};

G_DEFINE_TYPE (BijiProvider, biji_provider, G_TYPE_OBJECT)




BijiProviderHelper *
biji_provider_helper_new           (BijiProvider *provider,
                                    BijiItemsGroup group)
{
  BijiProviderHelper *retval;

  retval = g_slice_new (BijiProviderHelper);
  retval->provider = provider;
  retval->group = group;

  return retval;
}


void
biji_provider_helper_free          (BijiProviderHelper *helper)
{
  g_slice_free (BijiProviderHelper, helper);
}




BijiManager *
biji_provider_get_manager                (BijiProvider *provider)
{
  return provider->priv->manager;
}


const BijiProviderInfo *
biji_provider_get_info                (BijiProvider *provider)
{
  return BIJI_PROVIDER_GET_CLASS (provider)->get_info (provider);
}



void
biji_provider_load_archives        (BijiProvider *provider)
{
  return BIJI_PROVIDER_GET_CLASS (provider)->load_archives (provider);
}


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
  self->priv->manager = NULL;
}


static void
biji_provider_notify_loaded (BijiProvider *self,
                             GList *items,
                             BijiItemsGroup group)
{
  g_signal_emit (self,
                 biji_provider_signals[PROVIDER_LOADED],
                 0,
                 items,
                 group);
}


void
biji_provider_abort (BijiProvider *self)
{
  g_signal_emit (self, biji_provider_signals[PROVIDER_ABORT], 0);
}


static void
biji_provider_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BijiProvider *self = BIJI_PROVIDER (object);


  switch (property_id)
    {
    case PROP_BOOK:
      self->priv->manager = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_provider_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BijiProvider *self = BIJI_PROVIDER (object);

  switch (property_id)
    {
    case PROP_BOOK:
      g_value_set_object (value, self->priv->manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}


static void
biji_provider_class_init (BijiProviderClass *klass)
{
  GObjectClass *g_object_class;
  BijiProviderClass *provider_class;

  g_object_class = G_OBJECT_CLASS (klass);
  provider_class = BIJI_PROVIDER_CLASS (klass);

  g_object_class->finalize = biji_provider_finalize;
  g_object_class->get_property = biji_provider_get_property;
  g_object_class->set_property = biji_provider_set_property;
  provider_class->notify_loaded = biji_provider_notify_loaded;

  biji_provider_signals[PROVIDER_LOADED] =
    g_signal_new ("loaded",
                  G_OBJECT_CLASS_TYPE (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _biji_marshal_VOID__POINTER_ENUM,
                  G_TYPE_NONE,
                  2,
                  G_TYPE_POINTER,
                  G_TYPE_INT);

  biji_provider_signals[PROVIDER_ABORT] =
    g_signal_new ("abort",
		  G_OBJECT_CLASS_TYPE (klass),
		  G_SIGNAL_RUN_LAST,
		  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE,
		  0);


  properties[PROP_BOOK] =
    g_param_spec_object("manager",
                        "Note Book",
                        "The Note Book",
                        BIJI_TYPE_MANAGER,
                        G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);


  g_object_class_install_properties (g_object_class, PROVIDER_PROP, properties);

  g_type_class_add_private ((gpointer)klass, sizeof (BijiProviderPrivate));
}
