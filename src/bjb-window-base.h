#pragma once

#include <gtk/gtk.h>
#include <libbiji/libbiji.h>

#include "bjb-controller.h"

#define BJB_TYPE_WINDOW_BASE (bjb_window_base_get_type ())

G_DECLARE_FINAL_TYPE (BjbWindowBase, bjb_window_base, BJB, WINDOW_BASE, GtkApplicationWindow)

typedef enum {
  BJB_WINDOW_BASE_MAIN_VIEW,
  BJB_WINDOW_BASE_ARCHIVE_VIEW,
  BJB_WINDOW_BASE_SPINNER_VIEW,
  BJB_WINDOW_BASE_NOTE_VIEW,
  BJB_WINDOW_BASE_NO_NOTE,
  BJB_WINDOW_BASE_NO_RESULT,
  BJB_WINDOW_BASE_ERROR_TRACKER,
  BJB_WINDOW_BASE_NO_VIEW
} BjbWindowViewType;


GtkWindow             *bjb_window_base_new                (BijiNoteObj *note);


BjbController         *bjb_window_base_get_controller     (BjbWindowBase *window ) ;


void                   bjb_window_base_go_back            (BjbWindowBase *bwb);


void                   bjb_window_base_switch_to          (BjbWindowBase *bwb, BjbWindowViewType type);


void                   bjb_window_base_switch_to_item     (BjbWindowBase *bwb, BijiItem *item);


BjbWindowViewType      bjb_window_base_get_view_type      (BjbWindowBase *win);


BijiManager           *bjb_window_base_get_manager        (GtkWidget * win);


void                   bjb_window_base_set_entry(GtkWidget *win, gchar *search_entry) ;


gchar                 *bjb_window_base_get_entry(GtkWidget *win) ;


gpointer               bjb_window_base_get_main_view (BjbWindowBase *self);


BijiNoteObj           *bjb_window_base_get_note (BjbWindowBase *self);


gboolean               switch_window_fullscreen (void);


GtkWidget             *bjb_window_base_get_search_bar (BjbWindowBase *self);


gboolean               bjb_window_base_get_show_search_bar (BjbWindowBase *self);


void                   bjb_window_base_set_active (BjbWindowBase *self, gboolean active);



gboolean               bjb_window_base_is_detached (BjbWindowBase *self);
