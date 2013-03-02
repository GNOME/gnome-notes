#ifndef _MAIN_VIEW_H_
#define _MAIN_VIEW_H_

#include <glib-object.h>
#include <gtk/gtk.h>
#include <libbiji/libbiji.h>

#include "bjb-controller.h"

G_BEGIN_DECLS

#define BJB_TYPE_MAIN_VIEW             (bjb_main_view_get_type ())
#define BJB_MAIN_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BJB_TYPE_MAIN_VIEW, BjbMainView))
#define BJB_MAIN_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BJB_TYPE_MAIN_VIEW, BjbMainViewClass))
#define BJB_IS_MAIN_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BJB_TYPE_MAIN_VIEW))
#define BJB_IS_MAIN_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BJB_TYPE_MAIN_VIEW))
#define BJB_MAIN_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BJB_TYPE_MAIN_VIEW, BjbMainViewClass))

typedef struct _BjbMainViewClass BjbMainViewClass;
typedef struct _BjbMainView BjbMainView;

typedef struct _BjbMainViewPriv BjbMainViewPriv;

struct _BjbMainViewClass
{
  GtkBoxClass parent_class ;
};

struct _BjbMainView
{
  GtkBox parent_instance ;
  BjbMainViewPriv *priv;
};

GType bjb_main_view_get_type (void) G_GNUC_CONST;

BjbMainView * bjb_main_view_new(GtkWidget *win, BjbController *controller);

void bjb_main_view_connect_signals (BjbMainView *self);

GtkWidget *bjb_main_view_get_window(BjbMainView *view);

void action_new_window_callback(GtkAction *action, gpointer bjb_main_view);

/* Currently it simply looks for first selected note found. Maybe most common color?
 * Or use default color when several ones including at least one default color?*/
gboolean bjb_main_view_get_selected_notes_color (BjbMainView *view, GdkRGBA *color);

void action_color_selected_notes (GtkWidget *w,BjbMainView *view);

void action_tag_selected_notes (GtkWidget *w, BjbMainView *view);

void action_delete_selected_notes(GtkWidget *w,BjbMainView *view);

gboolean bjb_main_view_get_iter_at_note (BjbMainView  *view,
                                         BijiNoteObj  *note,
                                         GtkTreeIter  *retval);

void update_notes_with_tag_search(BjbMainView *view, gchar *tag);

void update_notes_with_string_search(BjbMainView *view, gchar *needle);

void switch_to_note_view(BjbMainView *view,BijiNoteObj *note) ;

void bjb_main_view_update_model (BjbMainView *view);

G_END_DECLS

#endif /* _MAIN_VIEW_H_ */
