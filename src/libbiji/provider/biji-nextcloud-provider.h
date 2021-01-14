/* biji-nextcloud-provider.h
 *
 * Copyright 2020 Isaque Galdino <igaldino@gmail.com>
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
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include "biji-provider.h"

G_BEGIN_DECLS

#define BIJI_TYPE_NEXTCLOUD_PROVIDER (biji_nextcloud_provider_get_type ())

G_DECLARE_FINAL_TYPE (BijiNextcloudProvider, biji_nextcloud_provider, BIJI, NEXTCLOUD_PROVIDER, BijiProvider)

BijiProvider *biji_nextcloud_provider_new (BijiManager *manager,
                                           GoaObject   *goa);

const char *biji_nextcloud_provider_get_baseurl (BijiNextcloudProvider *self);

const char *biji_nextcloud_provider_get_username (BijiNextcloudProvider *self);

const char *biji_nextcloud_provider_get_password (BijiNextcloudProvider *self);

G_END_DECLS

