/*
 * biji-import-provider.h
 *
 * Copyright 2013 Pierre-Yves Luyten <py@luyten.fr>
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


/* A special case of providers : importing data.
 * As the provider type is close enough
 * use this : all import func should be ported to use this.
 *
 * Once the ImportProvider does it's job,
 * it kills its own ref.
 */

/* As of today this is the direct class to be used by notemanager.
 * Later on manager might use different importProviders sub-classes */

#pragma once

#include "biji-provider.h"

G_BEGIN_DECLS

#define BIJI_TYPE_IMPORT_PROVIDER (biji_import_provider_get_type ())

G_DECLARE_FINAL_TYPE (BijiImportProvider, biji_import_provider, BIJI, IMPORT_PROVIDER, BijiProvider)

BijiProvider           *biji_import_provider_new                  (BijiManager *manager,
                                                                   const char  *target_provider,
                                                                   const char  *uri);

G_END_DECLS

