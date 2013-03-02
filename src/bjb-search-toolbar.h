
#ifndef BJB_SEARCH_TOOLBAR_H
#define BJB_SEARCH_TOOLBAR_H

#include <libgd/gd-revealer.h>

G_BEGIN_DECLS

#define BJB_TYPE_SEARCH_TOOLBAR (bjb_search_toolbar_get_type ())

#define BJB_SEARCH_TOOLBAR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BJB_TYPE_SEARCH_TOOLBAR, BjbSearchToolbar))

#define BJB_SEARCH_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), BJB_TYPE_SEARCH_TOOLBAR, BjbSearchToolbarClass))

#define BJB_IS_SEARCH_TOOLBAR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BJB_TYPE_SEARCH_TOOLBAR))

#define BJB_IS_SEARCH_TOOLBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BJB_TYPE_SEARCH_TOOLBAR))

#define BJB_SEARCH_TOOLBAR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BJB_TYPE_SEARCH_TOOLBAR, BjbSearchToolbarClass))

typedef struct _BjbSearchToolbar        BjbSearchToolbar;
typedef struct _BjbSearchToolbarClass   BjbSearchToolbarClass;
typedef struct _BjbSearchToolbarPrivate BjbSearchToolbarPrivate;

struct _BjbSearchToolbar
{
  GtkToolbar parent_instance;
  BjbSearchToolbarPrivate *priv;
};

struct _BjbSearchToolbarClass
{
  GtkToolbarClass parent_class;
};

GType bjb_search_toolbar_get_type (void) G_GNUC_CONST;

BjbSearchToolbar * bjb_search_toolbar_new (GtkWidget *window, BjbController *controller);

GdRevealer * bjb_search_toolbar_get_revealer (BjbSearchToolbar *self);

void bjb_search_toolbar_disconnect (BjbSearchToolbar *self);

void bjb_search_toolbar_connect (BjbSearchToolbar *self);

G_END_DECLS

#endif /* BJB_SEARCH_TOOLBAR_H */
