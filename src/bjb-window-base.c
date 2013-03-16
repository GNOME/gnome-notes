
#include <glib/gprintf.h>
#include <stdlib.h>

#include <libbiji/libbiji.h>
#include <libgd/gd.h>

#include "bjb-bijiben.h"
#include "bjb-window-base.h"
#include "bjb-main-view.h"
#include "bjb-note-view.h"


#define BJB_WIDTH 880
#define BJB_HEIGHT 600
#define BIJIBEN_MAIN_WIN_TITLE "Bijiben"

#define BJB_DEFAULT_FONT "Serif 10"

/* As the main window remains, it owns the data */
struct _BjbWindowBasePriv
{
  /* To register new windows and access the data */
  GtkApplication       *app ;
  BjbController        *controller;

  /* UI
   * The Notebook always has a main view.
   * When editing a note, it _also_ has a note view */
  GdStack              *stack;
  BjbWindowViewType     current_view;
  BjbMainView          *view;
  gchar                *entry;

  /* when a note is opened */
  BijiNoteObj          *note;
  BjbNoteView          *note_view;
  GtkWidget            *note_overlay;

  /* To avoid loiding several times */
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

  /* UI : basic notebook */
  priv->stack = GD_STACK (gd_stack_new ());
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (priv->stack));

  /* Signals */
  g_signal_connect(GTK_WIDGET(self),"destroy",
                   G_CALLBACK(bjb_window_base_destroy),self);
}

static void
bjb_window_base_init (BjbWindowBase *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                            BJB_TYPE_WINDOW_BASE,
                                            BjbWindowBasePriv);

  /* Default window has no note opened */
  self->priv->note_view = NULL;
}

static void
bjb_window_base_class_init (BjbWindowBaseClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = bjb_window_base_constructed;
  gobject_class->constructor = bjb_window_base_constructor;
  gobject_class->finalize = bjb_window_base_finalize ;

  g_type_class_add_private (klass, sizeof (BjbWindowBasePriv));
}

GtkWindow *
bjb_window_base_new(void)
{
  BjbWindowBase *retval;
  BjbWindowBasePriv *priv;

  retval = g_object_new(BJB_TYPE_WINDOW_BASE,
                        "application", g_application_get_default(),
                        "hide-titlebar-when-maximized", TRUE,
                        NULL);

  /* Rather dirty to finish UI there. maybe bjb_w_b_set_controller */

  priv = retval->priv;

  priv->controller = bjb_controller_new 
    (bijiben_get_book (BIJIBEN_APPLICATION(g_application_get_default())),
     priv->entry );

  priv->view = bjb_main_view_new (GTK_WIDGET (retval), priv->controller);
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

void
bjb_window_base_set_note (BjbWindowBase *self, BijiNoteObj *note)
{
  g_return_if_fail (BJB_IS_WINDOW_BASE (self));

  self->priv->note = note;
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
  if (bwb->priv->note_view && GTK_IS_WIDGET (bwb->priv->note_view))
  {
    g_clear_pointer (&(bwb->priv->note_overlay), gtk_widget_destroy);
  }
}

void
bjb_window_base_switch_to (BjbWindowBase *bwb, BjbWindowViewType type)
{
  BjbWindowBasePriv *priv = bwb->priv;

  /* Precise the window does not display any specific note
   * Refresh the model
   * Ensure the main view receives the proper signals */
  if (type == BJB_MAIN_VIEW)
  {
    priv->note = NULL;
    bjb_main_view_connect_signals (priv->view);
    gd_stack_set_visible_child_name (priv->stack, "main-view");

    destroy_note_if_needed (bwb);
  }

  else
  {
    gtk_widget_show_all (GTK_WIDGET (priv->note_overlay));
    gd_stack_set_visible_child_name (priv->stack, "note-view");
  }
}

void
bjb_window_base_switch_to_note (BjbWindowBase *bwb, BijiNoteObj *note)
{
  BjbWindowBasePriv *priv = bwb->priv;
  GtkWidget *w = GTK_WIDGET (bwb);

  destroy_note_if_needed (bwb);

  priv->note_overlay = gtk_overlay_new ();
  gd_stack_add_named (priv->stack, priv->note_overlay, "note-view");
  priv->note_view = bjb_note_view_new (w, priv->note_overlay, note);
  g_object_add_weak_pointer (G_OBJECT (priv->note_view),
                             (gpointer *) &priv->note_view);

  bjb_window_base_set_note (bwb, priv->note);
  bjb_window_base_switch_to (bwb, BJB_NOTE_VIEW);
  gtk_widget_show_all (w);
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
