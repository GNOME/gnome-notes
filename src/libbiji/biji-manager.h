#pragma once

#include <glib-object.h>
#include <tracker-sparql.h>

#include "biji-info-set.h"
#include "../items/bjb-tag.h"


#include <libedataserver/libedataserver.h> /* ESourceRegistry */

#define GOA_API_IS_SUBJECT_TO_CHANGE
#include <goa/goa.h>



G_BEGIN_DECLS

/* The flag tells if view should reload the whole model or not */
typedef enum
{
  BIJI_MANAGER_CHANGE_FLAG,
  BIJI_MANAGER_MASS_CHANGE,        // Startup, mass import.. rather rebuild the whole.
  BIJI_MANAGER_ITEM_ADDED,         // Single item added
  BIJI_MANAGER_ITEM_TRASHED,       // Single item trashed
  BIJI_MANAGER_ITEM_RESTORED,       // Single item restored
  BIJI_MANAGER_ITEM_DELETED,       // Single item deleted
  BIJI_MANAGER_ITEM_ICON_CHANGED,  // Single item icon
  BIJI_MANAGER_NOTE_AMENDED,       // Single note amended (title, content)
} BijiManagerChangeFlag;


typedef enum
{
  BIJI_LIVING_ITEMS,
  BIJI_ARCHIVED_ITEMS
} BijiItemsGroup;

#define BIJI_TYPE_MANAGER (biji_manager_get_type ())

G_DECLARE_FINAL_TYPE (BijiManager, biji_manager, BIJI, MANAGER, GObject)

GListModel      *biji_manager_get_notes             (BijiManager         *manager,
                                                     BijiItemsGroup       group);
GListModel      *biji_manager_get_notebooks         (BijiManager         *self);
BjbItem         *biji_manager_find_notebook         (BijiManager         *self,
                                                     const char          *uuid);
G_END_DECLS
