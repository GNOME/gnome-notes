/*
 * Bijiben
 * Copyright (C) Pierre-Yves Luyten 2012 <py@luyten.fr>
 *
 * Bijiben is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * WebkitWebView is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "../items/bjb-note.h"

G_BEGIN_DECLS

#define BIJI_TYPE_LAZY_DESERIALIZER (biji_lazy_deserializer_get_type ())

G_DECLARE_FINAL_TYPE (BijiLazyDeserializer, biji_lazy_deserializer, BIJI, LAZY_DESERIALIZER, GObject)

gboolean biji_lazy_deserialize (BjbNote *note);

G_END_DECLS
