/* biji-timeout.c
 * Copyright (C) Pierre-Yves LUYTEN 2012 <py@luyten.fr>
 * Copyright 2017 Mohammed Sadiq <sadiq@sadiqpk.org>
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
 *     Ported from gnote
 */

#include <gio/gio.h>

#include "biji-timeout.h"

/* Signal */
enum {
  BIJI_TIME_OUT,
  BIJI_TIME_SIGNALS
};

static guint biji_time_signals [BIJI_TIME_SIGNALS] = { 0 };

struct _BijiTimeout
{
  GObject parent_instance;

  guint timeout_id;
  guint quit;
};

G_DEFINE_TYPE (BijiTimeout, biji_timeout, G_TYPE_OBJECT);

static void
biji_timeout_init (BijiTimeout *self)
{
}

static void
biji_timeout_finalize (GObject *object)
{
  BijiTimeout  *self = BIJI_TIMEOUT (object);
  GApplication *app  = g_application_get_default ();

  biji_timeout_cancel (self);

  if (self->quit != 0 && g_signal_handler_is_connected (app, self->quit))
    g_signal_handler_disconnect (app, self->quit);

  G_OBJECT_CLASS (biji_timeout_parent_class)->finalize (object);
}

static void
biji_timeout_class_init (BijiTimeoutClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = biji_timeout_finalize;

  biji_time_signals[BIJI_TIME_OUT] = g_signal_new ("timeout" ,
                                                  G_OBJECT_CLASS_TYPE (klass),
                                                  G_SIGNAL_RUN_LAST,
                                                  0, 
                                                  NULL, 
                                                  NULL,
                                                  g_cclosure_marshal_VOID__VOID,
                                                  G_TYPE_NONE,
                                                  0);
}

BijiTimeout * biji_timeout_new (void)
{
  return g_object_new (BIJI_TYPE_TIMEOUT,
                       NULL);
}

static gboolean
biji_timeout_expired (BijiTimeout *self)
{
  g_signal_emit (self, biji_time_signals[BIJI_TIME_OUT], 0);
  self->timeout_id = 0;
  return FALSE;
}

static gboolean
biji_timeout_callback (BijiTimeout *self)
{
  if (self)
    return biji_timeout_expired (self);

  return FALSE;
}

void
biji_timeout_cancel (BijiTimeout *self)
{
  if (self->timeout_id != 0)
    g_source_remove (self->timeout_id);

  self->timeout_id = 0;
}

void
biji_timeout_reset (BijiTimeout *self, guint millis)
{
  biji_timeout_cancel (self);

  self->timeout_id = g_timeout_add (
       millis, (GSourceFunc) biji_timeout_callback, self);

  /* Ensure to perform timeout if main loop ends */
  self->quit = g_signal_connect_swapped (g_application_get_default(), "shutdown",
                                         G_CALLBACK (biji_timeout_expired), self);
}
