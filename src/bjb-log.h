/* -*- mode: c; c-basic-offset: 2; indent-tabs-mode: nil; -*- */
/* bjb-log.h
 *
 * Copyright 2021 Mohammed Sadiq <sadiq@sadiqpk.org>
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

#ifndef BJB_LOG_LEVEL_TRACE
# define BJB_LOG_LEVEL_TRACE ((GLogLevelFlags)(1 << G_LOG_LEVEL_USER_SHIFT))
#endif

#define BJB_TRACE_MSG(fmt, ...)                         \
  g_log_structured (G_LOG_DOMAIN, BJB_LOG_LEVEL_TRACE,  \
                    "MESSAGE", "%s():%d: " fmt,         \
                    G_STRFUNC, __LINE__, ##__VA_ARGS__)
#define BJB_TRACE(fmt, ...)                             \
  g_log_structured (G_LOG_DOMAIN, BJB_LOG_LEVEL_TRACE,  \
                    "MESSAGE",  fmt, ##__VA_ARGS__)
#define BJB_DEBUG_MSG(fmt, ...)                         \
  g_log_structured (G_LOG_DOMAIN, G_LOG_LEVEL_DEBUG,    \
                    "MESSAGE", "%s():%d: " fmt,         \
                    G_STRFUNC, __LINE__, ##__VA_ARGS__)
#define BJB_TODO(_msg)                                  \
  g_log_structured (G_LOG_DOMAIN, BJB_LOG_LEVEL_TRACE,  \
                    "MESSAGE", "TODO: %s():%d: %s",     \
                    G_STRFUNC, __LINE__, _msg)

void bjb_log_init               (void);
void bjb_log_increase_verbosity (void);
int  bjb_log_get_verbosity      (void);
