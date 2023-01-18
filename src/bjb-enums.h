/* bjb-enums.h
 *
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

#include <glib.h>

G_BEGIN_DECLS

typedef enum
{
  BJB_STATUS_UNKNOWN,
  BJB_STATUS_DISCONNECTED,
  BJB_STATUS_CONNECTING,
  BJB_STATUS_CONNECTED
} BjbStatus;

typedef enum
{
  BJB_FEATURE_NONE           = 0,
  BJB_FEATURE_COLOR          = 1 <<  0,
  /* If note supports bold, italic, underline, strikethrough */
  BJB_FEATURE_FORMAT         = 1 << 1,
  BJB_FEATURE_TRASH          = 1 << 2,
  /* A note can have only one notebook */
  BJB_FEATURE_NOTEBOOK       = 1 << 3,
  /* A note can have multiple tags */
  BJB_FEATURE_TAG            = 1 << 4,
  /* Creation Date */
  BJB_FEATURE_CDATE          = 1 << 5,
  /* Modification Date */
  BJB_FEATURE_MDATE          = 1 << 6,
} BjbFeature;

G_END_DECLS
