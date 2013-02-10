
#include <glib/gprintf.h>
#include <stdlib.h>

#include <libbiji/libbiji.h>

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
  GtkWidget            *notebook;
  BjbWindowViewType     current_view;
  BjbMainView          *view;
  ClutterActor         *stage, *note_stage, *frame;
  gchar                *entry;

  /* when a note is opened */
  BijiNoteObj          *note;

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
  g_clear_object (&priv->view);
  G_OBJECT_CLASS (bjb_window_base_parent_class)->finalize (object);
}

static void
bjb_window_base_class_init (BjbWindowBaseClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = bjb_window_base_constructor;
  gobject_class->finalize = bjb_window_base_finalize ;

  g_type_class_add_private (klass, sizeof (BjbWindowBasePriv));
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
bjb_window_base_init (BjbWindowBase *self) 
{
  BjbWindowBasePriv *priv;
  const gchar *icons_path;
  gchar *full_path;
  GList *icons = NULL;
  GdkPixbuf *bjb ;
  GError *error = NULL;
  GtkClutterEmbed *embed;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE(self,
                                           BJB_TYPE_WINDOW_BASE,
                                           BjbWindowBasePriv);
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
  priv->notebook = gtk_notebook_new ();
  gtk_container_add (GTK_CONTAINER (self), priv->notebook);
  gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), FALSE);
  gtk_notebook_set_show_border (GTK_NOTEBOOK (priv->notebook), FALSE);

  /* Page for overview */
  embed = GTK_CLUTTER_EMBED (gtk_clutter_embed_new());
  gtk_clutter_embed_set_use_layout_size (embed, TRUE);
  gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                            GTK_WIDGET (embed), gtk_label_new ("main-view"));
  priv->stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embed));

  /* Page for note */
  embed = GTK_CLUTTER_EMBED (gtk_clutter_embed_new());
  gtk_clutter_embed_set_use_layout_size (embed, TRUE);
  gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook),
                            GTK_WIDGET (embed), gtk_label_new ("note-view"));
  priv->note_stage = gtk_clutter_embed_get_stage (GTK_CLUTTER_EMBED (embed));

  /* Signals */
  g_signal_connect(GTK_WIDGET(self),"destroy",
                   G_CALLBACK(bjb_window_base_destroy),self);
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
  priv = retval->priv;

  priv->controller = bjb_controller_new 
    (bijiben_get_book (BIJIBEN_APPLICATION(g_application_get_default())),
     priv->entry );

  /* UI : notes view. But some settings could allow other default. */
  priv->view = bjb_main_view_new ( GTK_WIDGET(retval),priv->controller);
  priv->frame = bjb_main_view_get_actor(priv->view);

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
bjb_window_base_set_frame(BjbWindowBase *bwb,ClutterActor *frame)
{
  /* TODO removing frame should finalize
   * or we can implement some interface
   * (bool) (hide_frame) (bwb) */
  if ( bwb->priv->frame )
  {
    clutter_actor_destroy(bwb->priv->frame);
    bwb->priv->frame = NULL ;
  }

  if ( CLUTTER_IS_ACTOR( bwb->priv->frame) )
  {
    bwb->priv->frame = frame ;
    clutter_actor_add_child (bwb->priv->stage, frame);
  }
}

ClutterActor *
bjb_window_base_get_frame(BjbWindowBase *bwb)
{
  return bwb->priv->frame ;
}

ClutterActor *
bjb_window_base_get_stage (BjbWindowBase *bwb, BjbWindowViewType type)
{
  if (type == NOTE_VIEW)
    return bwb->priv->note_stage;

  return bwb->priv->stage;
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

void
bjb_window_base_switch_to (BjbWindowBase *bwb, BjbWindowViewType type)
{
  BjbWindowBasePriv *priv = bwb->priv;

  /* Precise the window does not display any specific note
   * Refresh the model
   * Ensure the main view receives the proper signals */
  if (type == MAIN_VIEW)
  {
    priv->note = NULL;
    bjb_main_view_connect_signals (priv->view);
    bjb_controller_update_view (priv->controller);
  }

  gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), type);
}

BijiNoteBook *
bjb_window_base_get_book(GtkWidget * win)
{
   BjbWindowBase *b = BJB_WINDOW_BASE(win);

   if ( b->priv )
   {
     return bijiben_get_book(BIJIBEN_APPLICATION(g_application_get_default()));
   }

   else
   {
       g_message("Can't get notes");
       return NULL ;
   }
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
