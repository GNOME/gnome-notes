/* bjb-side-view.h
 *
 * Copyright 2012, 2013 Pierre-Yves Luyten <py@luyten.fr>
 * Copyright 2020 Jonathan Kang <jonathankang@gnome.org>
 * Copyright 2021, 2025 Mohammed Sadiq <sadiq@sadiqpk.org>
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

#include <adwaita.h>

#include "items/bjb-item.h"

G_BEGIN_DECLS

#define BJB_TYPE_SIDE_VIEW (bjb_side_view_get_type ())
G_DECLARE_FINAL_TYPE (BjbSideView, bjb_side_view, BJB, SIDE_VIEW, AdwNavigationPage)

GtkWidget        *bjb_side_view_new                     (void);
BjbItem          *bjb_side_view_get_selected_note       (BjbSideView *self);

G_END_DECLS
