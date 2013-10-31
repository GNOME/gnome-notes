/* libiji.h
 * Copyright (C) Pierre-Yves LUYTEN 2012 <py@luyten.fr>
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

#ifndef _LIB_BIJI_H
#define _LIB_BIJI_H

#define _LIBBIJI_INSIDE_H


#include "biji-date-time.h"
#include "biji-item.h"
#include "biji-marshalers.h"
#include "biji-manager.h"
#include "biji-notebook.h"
#include "biji-note-obj.h"
#include "biji-string.h"
#include "biji-tracker.h"

#ifdef BUILD_ZEITGEIST
#include "biji-zeitgeist.h"
#endif /* BUILD_ZEITGEIST */

#include "deserializer/biji-lazy-deserializer.h"
#include "editor/biji-webkit-editor.h"
#include "provider/biji-provider.h"


#undef _LIBBIJI_INSIDE_H
#endif /*_LIB_BIJI_H*/
