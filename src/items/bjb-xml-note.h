/* bjb-xml-note.h
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

#include <glib-object.h>

#include "bjb-tag-store.h"
#include "bjb-note.h"

G_BEGIN_DECLS

#define BJB_TYPE_XML_NOTE (bjb_xml_note_get_type ())
G_DECLARE_FINAL_TYPE (BjbXmlNote, bjb_xml_note, BJB, XML_NOTE, BjbNote)

BjbItem    *bjb_xml_note_new_from_data    (const char  *uid,
                                           BjbTagStore *tag_store);

G_END_DECLS
