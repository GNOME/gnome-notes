#ifndef _BJB_WINDOW_BASE_H
#define _BJB_WINDOW_BASE_H

#include <gtk/gtk.h>
#include <libbiji/libbiji.h>

#include "bjb-settings.h"
#include "bjb-controller.h"

#define BJB_TYPE_WINDOW_BASE                  (bjb_window_base_get_type ())
#define BJB_WINDOW_BASE(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), BJB_TYPE_WINDOW_BASE, BjbWindowBase))
#define BJB_IS_WINDOW_BASE(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BJB_TYPE_WINDOW_BASE))
#define BJB_WINDOW_BASE_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST ((klass), BJB_TYPE_WINDOW_BASE, BjbWindowBaseClass))
#define BJB_IS_WINDOW_BASE_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE ((klass), BJB_TYPE_WINDOW_BASE))
#define BJB_WINDOW_BASE_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS ((obj), BJB_TYPE_WINDOW_BASE, BjbWindowBaseClass))

typedef struct _BjbWindowBase        BjbWindowBase;
typedef struct _BjbWindowBaseClass   BjbWindowBaseClass;

typedef struct _BjbWindowBasePriv BjbWindowBasePriv;

struct _BjbWindowBaseClass
{
  GtkApplicationWindowClass parent_class;
};


struct _BjbWindowBase
{
  GtkApplicationWindow parent_instance ;
  BjbWindowBasePriv *priv;
};

typedef enum {
  BJB_MAIN_VIEW,
  BJB_NOTE_VIEW,
  BJB_NO_VIEW
} BjbWindowViewType;

GType bjb_window_base_get_type (void);

GtkWindow * bjb_window_base_new(void);

// Accessor 

BjbController * bjb_window_base_get_controller ( BjbWindowBase *window ) ;

PangoFontDescription *bjb_window_base_get_font(GtkWidget *window);

void bjb_window_base_switch_to (BjbWindowBase *bwb, BjbWindowViewType type);

void bjb_window_base_switch_to_note (BjbWindowBase *bwb, BijiNoteObj *note);

BijiNoteBook * bjb_window_base_get_book(GtkWidget * win);

void bjb_window_base_set_entry(GtkWidget *win, gchar *search_entry) ;

void bjb_window_base_delete_entry(GtkWidget *win);

gchar * bjb_window_base_get_entry(GtkWidget *win) ;

gpointer bjb_window_base_get_main_view (BjbWindowBase *self);

BijiNoteObj * bjb_window_base_get_note (BjbWindowBase *self);

void bjb_window_base_set_note (BjbWindowBase *self, BijiNoteObj *note);

gboolean switch_window_fullscreen();

#endif /* _BJB_WINDOW_BASE_H */
