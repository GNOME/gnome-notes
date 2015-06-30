#ifndef _BIJI_MANAGER_H_
#define _BIJI_MANAGER_H_

#include <glib-object.h>
#include <tracker-sparql.h>

#if BUILD_ZEITGEIST
#include <zeitgeist.h>
#endif /* BUILD_ZEIGEIST */

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


#define BIJI_TYPE_MANAGER             (biji_manager_get_type ())
#define BIJI_MANAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_MANAGER, BijiManager))
#define BIJI_MANAGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_MANAGER, BijiManagerClass))
#define BIJI_IS_MANAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_MANAGER))
#define BIJI_IS_MANAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_MANAGER))
#define BIJI_MANAGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_MANAGER, BijiManagerClass))

typedef struct _BijiManagerClass BijiManagerClass;
typedef struct _BijiManager BijiManager;

typedef struct _BijiManagerPrivate BijiManagerPrivate;

struct _BijiManagerClass
{
  GObjectClass parent_class;
};

struct _BijiManager
{
  GObject parent_instance;
  BijiManagerPrivate *priv ;
};



GType biji_manager_get_type (void) G_GNUC_CONST;



BijiManager     *biji_manager_new                   (GFile *location,
                                                    GdkRGBA *color,
                                                    GError **error);

void             biji_manager_new_async             (GFile *location,
                                                     GdkRGBA *color,
                                                     GAsyncReadyCallback callback,
                                                     gpointer user_data);

BijiManager     *biji_manager_new_finish            (GAsyncResult *res,
                                                     GError **error);


void             biji_manager_import_uri            (BijiManager *manager,
                                                     gchar *target_provider_id,
                                                     gchar *uri);

GList           *biji_manager_get_providers         (BijiManager *manager); /* <ProviderInfo*> */


#if BUILD_ZEITGEIST
ZeitgeistLog    *biji_manager_get_zg_log            (BijiManager *manager);
#endif /* BUILD_ZEITGEIST */

TrackerSparqlConnection
                *biji_manager_get_tracker_connection (BijiManager *manager);



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



void             biji_manager_empty_bin              (BijiManager        *manager);



BijiNoteObj     *biji_manager_note_new              (BijiManager *manager,
                                                     gchar        *str,
                                                     gchar        *provider_id);


BijiNoteObj     *biji_manager_note_new_full         (BijiManager *manager,
                                                     gchar        *provider_id,
                                                     gchar        *suggested_path,
                                                     BijiInfoSet  *info,
                                                     gchar        *html,
                                                     GdkRGBA      *color);


G_END_DECLS

#endif /* _BIJI_MANAGER_H_ */
