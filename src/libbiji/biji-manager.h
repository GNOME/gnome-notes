#pragma once

#include <glib-object.h>
#include <tracker-sparql.h>

#include "biji-info-set.h"
#include "biji-note-obj.h"


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

BijiManager     *biji_manager_new                   (GFile         *location,
                                                     GdkRGBA       *color);
void            biji_manager_load_providers_async   (BijiManager   *self,
                                                     GAsyncReadyCallback callback,
                                                     gpointer       user_data);
gboolean        biji_manager_load_providers_finish  (BijiManager   *self,
                                                     GAsyncResult  *result,
                                                     GError       **error);

void             biji_manager_import_uri            (BijiManager *manager,
                                                     const gchar *target_provider_id,
                                                     const gchar *uri);

void             biji_manager_set_provider          (BijiManager *manager,
                                                     const gchar *provider_id);

GListModel      *biji_manager_get_providers         (BijiManager *manager);
TrackerSparqlConnection
                *biji_manager_get_tracker_connection (BijiManager *manager);
gpointer         biji_manager_get_tracker            (BijiManager *self);



void             biji_manager_get_default_color     (BijiManager *manager,
                                                     GdkRGBA *color);


gchar           *biji_manager_get_unique_title      (BijiManager *manager,
                                                     const gchar *title);


gboolean         biji_manager_add_item                (BijiManager *manager,
                                                       BijiItem *item,
                                                       BijiItemsGroup group,
                                                       gboolean notify);


void             biji_manager_notify_changed        (BijiManager           *manager,
                                                     BijiItemsGroup         group,
                                                     BijiManagerChangeFlag  flag,
                                                     BijiItem              *item);


BijiItem        *biji_manager_get_item_at_path      (BijiManager *manager,
                                                     const gchar *path);

/* Get all items, either notes or notebooks
 * Free the GList, not its content */


GList           *biji_manager_get_items             (BijiManager         *manager,
                                                     BijiItemsGroup       group);
GListModel      *biji_manager_get_notebooks         (BijiManager         *self);
BijiItem        *biji_manager_find_notebook         (BijiManager         *self,
                                                     const char          *uuid);

BijiNoteObj     *biji_manager_note_new              (BijiManager *manager,
                                                     const gchar *str,
                                                     const gchar *provider_id);


BijiNoteObj     *biji_manager_note_new_full         (BijiManager   *manager,
                                                     const gchar   *provider_id,
                                                     const gchar   *suggested_path,
                                                     BijiInfoSet   *info,
                                                     const gchar   *html,
                                                     const GdkRGBA *color);


G_END_DECLS
