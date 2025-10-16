/* bjb-memo-provider.h
 *
 * Copyright 2013 Pierre-Yves LUYTEN <py@luyten.fr>
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

#pragma once

#include <libedataserver/libedataserver.h>
#include <libecal/libecal.h>

#include "bjb-provider.h"

G_BEGIN_DECLS

G_DEFINE_AUTOPTR_CLEANUP_FUNC (ECalClient, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ECalComponent, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ECalComponentId, e_cal_component_id_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ECalComponentText, e_cal_component_text_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC (ECalComponentDateTime, e_cal_component_datetime_free)

#define BJB_TYPE_MEMO_PROVIDER (bjb_memo_provider_get_type ())
G_DECLARE_FINAL_TYPE (BjbMemoProvider, bjb_memo_provider, BJB, MEMO_PROVIDER, BjbProvider)

BjbProvider *bjb_memo_provider_new (ESource *source);

G_END_DECLS
