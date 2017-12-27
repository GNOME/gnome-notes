/* biji-timeout.h
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

#pragma once

#include <glib-object.h>

G_BEGIN_DECLS

#define BIJI_TYPE_TIMEOUT (biji_timeout_get_type ())

G_DECLARE_FINAL_TYPE (BijiTimeout, biji_timeout, BIJI, TIMEOUT, GObject)

BijiTimeout * biji_timeout_new (void);

void biji_timeout_reset (BijiTimeout *self, guint millisec);

void biji_timeout_cancel (BijiTimeout *self);

void biji_timeout_free (BijiTimeout *self);

G_END_DECLS
