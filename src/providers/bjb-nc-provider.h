/* bjb-nc-provider.h
 *
 * Copyright 2023 Mohammed Sadiq <sadiq@sadiqpk.org>
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

#pragma once

#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <goa/goa.h>

#include "bjb-provider.h"

G_BEGIN_DECLS

#define BJB_TYPE_NC_PROVIDER (bjb_nc_provider_get_type ())
G_DECLARE_FINAL_TYPE (BjbNcProvider, bjb_nc_provider, BJB, NC_PROVIDER, BjbProvider)

BjbProvider  *bjb_nc_provider_new         (GoaObject *object);
gboolean      bjb_nc_provider_matches_goa (BjbNcProvider *self,
                                           GoaObject     *object);

G_END_DECLS
