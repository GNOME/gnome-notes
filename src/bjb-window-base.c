
#include <glib/gprintf.h>
#include <stdlib.h>

#include <libbiji/libbiji.h>
#include <libgd/gd.h>

#include "bjb-bijiben.h"
#include "bjb-empty-results-box.h"
#include "bjb-window-base.h"
#include "bjb-main-toolbar.h"
#include "bjb-main-view.h"
#include "bjb-note-view.h"


#define BJB_WIDTH 780
#define BJB_HEIGHT 600
#define BIJIBEN_MAIN_WIN_TITLE "Bijiben"

#define BJB_DEFAULT_FONT "Serif 10"

enum {
  BJB_WIN_BASE_VIEW_CHANGED,
  BJB_WIN_BASE_SIGNALS
};

static guint bjb_win_base_signals [BJB_WIN_BASE_SIGNALS] = { 0 };


struct _BjbWindowBasePriv
{
  GtkApplication       *app ;
  BjbController        *controller;
  gchar                *entry; // FIXME, remove this


  GtkWidget            *vbox;
  BjbMainToolbar       *main_toolbar;
  BjbSearchToolbar     *search_bar;


  GdStack              *stack;
  BjbWindowViewType     current_view;
  BjbMainView          *view;
  BjbNoteView          *note_view;
  GtkWidget            *spinner;
  GtkWidget            *no_note;


  /* when a note is opened */
  BijiNoteObj          *note;
  GtkWidget            *note_overlay;


  PangoFontDescription *font ;
};

/* Gobject */
G_DEFINE_TYPE (BjbWindowBase, bjb_window_base, GTK_TYPE_APPLICATION_WINDOW);


static GObject *
bjb_window_base_constructor (GType                  gtype,
                             guint                  n_properties,
                             GObjectConstructParam  *properties)
{
  GObject *obj;
  {
    obj = G_OBJECT_CLASS (bjb_window_base_parent_class)->constructor (gtype, n_properties, properties);
  }
  return obj;
}


static void
bjb_window_base_finalize (GObject *object)
{
  BjbWindowBase *self = BJB_WINDOW_BASE (object);
  BjbWindowBasePriv *priv = self->priv;

  g_clear_object (&priv->controller);

  G_OBJECT_CLASS (bjb_window_base_parent_class)->finalize (object);
}

/* Just disconnect to avoid crash, the finalize does the real
 * job */
static void
bjb_window_base_destroy (gpointer a, BjbWindowBase * self)
{
  bjb_controller_disconnect (self->priv->controller);
}

/* Gobj */
static void
bjb_window_base_constructed (GObject *obj)
{
  BjbWindowBase *self = BJB_WINDOW_BASE (obj);
  BjbWindowBasePriv *priv;
  const gchar *icons_path;
  gchar *full_path;
  GList *icons = NULL;
  GdkPixbuf *bjb ;
  GError *error = NULL;

  priv = self->priv;
  priv->note = NULL;

  gtk_window_set_default_size (GTK_WINDOW (self), BJB_WIDTH, BJB_HEIGHT);
  gtk_window_set_position (GTK_WINDOW (self),GTK_WIN_POS_CENTER);
  gtk_window_set_title (GTK_WINDOW (self), BIJIBEN_MAIN_WIN_TITLE);

  /* Icon for window. TODO - Should be BjbApp */
  icons_path = bijiben_get_bijiben_dir ();
  full_path = g_build_filename (icons_path,
                                "icons",
                                "hicolor",
                                "48x48",
                                "apps",
                                "bijiben.png",
                                NULL);

  bjb = gdk_pixbuf_new_from_file (full_path, &error);
  g_free (full_path);
    
  if ( error )
  {
    g_message("%s", error->message);
    g_error_free(error);
  }
    
  icons = g_list_prepend(icons,bjb);
  gtk_window_set_default_icon_list(icons);
  g_list_foreach (icons, (GFunc) g_object_unref, NULL);
  g_list_free (icons);

  /*  We probably want to offer a no entry window at first (startup) */
  priv->entry = NULL ;
  priv->font = pango_font_description_from_string (BJB_DEFAULT_FONT);

  /* Signals */
  g_signal_connect(GTK_WIDGET(self),"destroy",
                   G_CALLBACK(bjb_window_base_destroy),self);
}

static void
bjb_window_base_init (BjbWindowBase *self)
{
  BjbWindowBasePriv *priv;
  priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                      BJB_TYPE_WINDOW_BASE,
                                      BjbWindowBasePriv);
  self->priv = priv;

  /* Default window has no note opened */
  priv->note_view = NULL;

  priv->vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (self), priv->vbox);
}

static void
bjb_window_base_class_init (BjbWindowBaseClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = bjb_window_base_constructed;
  gobject_class->constructor = bjb_window_base_constructor;
  gobject_class->finalize = bjb_window_base_finalize ;

  g_type_class_add_private (klass, sizeof (BjbWindowBasePriv));

  bjb_win_base_signals[BJB_WIN_BASE_VIEW_CHANGED] = g_signal_new ("view-changed" ,
                                                    G_OBJECT_CLASS_TYPE (klass),
                                                    G_SIGNAL_RUN_LAST,
                                                    0,
                                                    NULL,
                                                    NULL,
                                                    g_cclosure_marshal_VOID__VOID,
                                                    G_TYPE_NONE,
                                                    0);
}

GtkWindow *
bjb_window_base_new(void)
{
  BjbWindowBase       *retval;
  BjbWindowBasePriv   *priv;
  GdRevealer          *revealer;

  retval = g_object_new (BJB_TYPE_WINDOW_BASE,
                         "application", g_application_get_default(),
                         "hide-titlebar-when-maximized", TRUE,
                         "modal", TRUE,
                         NULL);

  /* Rather dirty to finish UI there */

  priv = retval->priv;

  priv->controller = bjb_controller_new 
    (bijiben_get_book (BIJIBEN_APPLICATION(g_application_get_default())),
     GTK_WINDOW (retval),
     priv->entry );

  /* Shared toolbar */
  priv->view = bjb_main_view_new (GTK_WIDGET (retval), priv->controller);
  priv->main_toolbar = bjb_main_toolbar_new (priv->view, priv->controller);
  gtk_box_pack_start (GTK_BOX (priv->vbox), GTK_WIDGET (priv->main_toolbar), FALSE, FALSE, 0);

  /* Search entry toolbar */
  priv->search_bar = bjb_search_toolbar_new (GTK_WIDGET (retval), priv->controller);
  revealer = bjb_search_toolbar_get_revealer (priv->search_bar);
  gtk_box_pack_start (GTK_BOX (priv->vbox), GTK_WIDGET (revealer), FALSE, FALSE, 0);

  /* UI : stack for different views */
  priv->stack = GD_STACK (gd_stack_new ());
  gtk_box_pack_start (GTK_BOX (priv->vbox), GTK_WIDGET (priv->stack), TRUE, TRUE, 0);

  priv->spinner = gtk_spinner_new ();
  gd_stack_add_named (priv->stack, priv->spinner, "spinner");
  gd_stack_set_visible_child_name (priv->stack, "spinner");
  gtk_widget_show (priv->spinner);
  gtk_spinner_start (GTK_SPINNER (priv->spinner));

  priv->no_note = bjb_empty_results_box_new ();
  gd_stack_add_named (priv->stack, priv->no_note, "empty");

  gd_stack_add_named (priv->stack, GTK_WIDGET (priv->view), "main-view");
  gtk_widget_show_all (GTK_WIDGET (retval));

  return GTK_WINDOW (retval);
}

BjbController *
bjb_window_base_get_controller ( BjbWindowBase *window )
{
  return window->priv->controller ;
}

PangoFontDescription *
window_base_get_font(GtkWidget *window)
{
  BjbWindowBase *b = BJB_WINDOW_BASE(window);
  return b->priv->font ;
}

BijiNoteObj *
bjb_window_base_get_note (BjbWindowBase *self)
{
  g_return_val_if_fail (BJB_IS_WINDOW_BASE (self), NULL);

  return self->priv->note;
}

static void
destroy_note_if_needed (BjbWindowBase *bwb)
{
  bwb->priv->note = NULL;

  if (bwb->priv->note_view && GTK_IS_WIDGET (bwb->priv->note_view))
    g_clear_pointer (&(bwb->priv->note_overlay), gtk_widget_destroy);
}

void
bjb_window_base_switch_to (BjbWindowBase *bwb, BjbWindowViewType type)
{
  BjbWindowBasePriv *priv = bwb->priv;
  priv->current_view = type;

  if (type != BJB_WINDOW_BASE_NOTE_VIEW)
    destroy_note_if_needed (bwb);

  switch (type)
  {

    /* Precise the window does not display any specific note
     * Refresh the model
     * Ensure the main view receives the proper signals */

    case BJB_WINDOW_BASE_MAIN_VIEW:
      bjb_search_toolbar_connect (priv->search_bar);
      bjb_main_view_connect_signals (priv->view);
      gd_stack_set_visible_child_name (priv->stack, "main-view");
      break;


    case BJB_WINDOW_BASE_SPINNER_VIEW:
      gd_stack_set_visible_child_name (priv->stack, "spinner");
      break;


    case BJB_WINDOW_BASE_NO_NOTE:
      bjb_empty_results_box_set_type (BJB_EMPTY_RESULTS_BOX (priv->no_note),
                                      BJB_EMPTY_RESULTS_NO_NOTE);
      gtk_widget_show (priv->no_note);
      gd_stack_set_visible_child_name (priv->stack, "empty");
      break;


    case BJB_WINDOW_BASE_NO_RESULT:
      bjb_empty_results_box_set_type (BJB_EMPTY_RESULTS_BOX (priv->no_note),
                                      BJB_EMPTY_RESULTS_NO_RESULTS);
      gtk_widget_show (priv->no_note);
      gd_stack_set_visible_child_name (priv->stack, "empty");
      break;


    case BJB_WINDOW_BASE_NOTE_VIEW:
      gtk_widget_show_all (GTK_WIDGET (priv->note_overlay));
      gd_stack_set_visible_child_name (priv->stack, "note-view");
      break;


    default:
      return;
  }

  g_signal_emit (G_OBJECT (bwb), bjb_win_base_signals[BJB_WIN_BASE_VIEW_CHANGED],0);
}

void
bjb_window_base_switch_to_note (BjbWindowBase *bwb, BijiNoteObj *note)
{
  BjbWindowBasePriv *priv = bwb->priv;
  GtkWidget *w = GTK_WIDGET (bwb);

  bjb_search_toolbar_disconnect (priv->search_bar);
  bjb_search_toolbar_fade_out (priv->search_bar);
  destroy_note_if_needed (bwb);

  priv->note = note;
  priv->note_overlay = gtk_overlay_new ();

  gd_stack_add_named (priv->stack, priv->note_overlay, "note-view");
  priv->note_view = bjb_note_view_new (w, priv->note_overlay, note);

  g_object_add_weak_pointer (G_OBJECT (priv->note_view),
                             (gpointer *) &priv->note_view);

  bjb_window_base_switch_to (bwb, BJB_WINDOW_BASE_NOTE_VIEW);
  gtk_widget_show_all (w);
}

BjbWindowViewType
bjb_window_base_get_view_type (BjbWindowBase *win)
{
  return win->priv->current_view;
}

BijiNoteBook *
bjb_window_base_get_book(GtkWidget * win)
{
  return bijiben_get_book (BIJIBEN_APPLICATION (g_application_get_default()));
}

void
bjb_window_base_set_entry(GtkWidget *win, gchar *search_entry)
{
  BjbWindowBase *bmw = BJB_WINDOW_BASE(win);
  bmw->priv->entry = search_entry ; 
}

void bjb_window_base_delete_entry(GtkWidget *win)
{
  BJB_WINDOW_BASE(win)->priv->entry = NULL ;
}

gchar *
bjb_window_base_get_entry(GtkWidget *win)
{
  BjbWindowBase *bmw = BJB_WINDOW_BASE(win);
  return bmw->priv->entry ;
}

gpointer
bjb_window_base_get_main_view (BjbWindowBase *self)
{
  return (gpointer) self->priv->view;
}

gboolean
bjb_window_base_get_show_search_bar (BjbWindowBase *self)
{
  /* There is no search bar at startup,
   * when main toolbar is first built... */
  if (!self->priv->search_bar)
    return FALSE;

  return bjb_search_toolbar_is_shown (self->priv->search_bar);
}

gboolean
bjb_window_base_set_show_search_bar (BjbWindowBase *self,
                                     gboolean show)
{
  if (show)
    bjb_search_toolbar_fade_in (self->priv->search_bar);

  else
    bjb_search_toolbar_fade_out (self->priv->search_bar);

  return TRUE;
}

gboolean
bjb_window_base_toggle_search_button (BjbWindowBase *self,
                                      gboolean active)
{
  bjb_main_toolbar_set_search_toggle_state (self->priv->main_toolbar,
                                            active);

  return TRUE;
}
