/* bjb-note-view.c
 * Copyright © 2012 Pierre-Yves LUYTEN <py@luyten.fr>
 * Copyright © 2017 Iñigo Martínez <inigomartinez@gmail.com>
 *
 * bijiben is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * bijiben is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libbiji/libbiji.h>
#include <libgd/gd.h>

#include "bjb-application.h"
#include "bjb-editor-toolbar.h"
#include "bjb-main-view.h"
#include "bjb-note-view.h"
#include "bjb-window-base.h"

enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_NOTE,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

struct _BjbNoteView
{
  GtkOverlay         parent_instance;

  /* Data */
  GtkWidget         *window ;
  GtkWidget         *view;
  BijiNoteObj       *note ;

  /* UI */
  BijiWebkitEditor  *editor;
  GtkWidget         *box;
  GtkWidget         *edit_bar;

  GtkWidget         *last_update;
};

G_DEFINE_TYPE (BjbNoteView, bjb_note_view, GTK_TYPE_OVERLAY)

static void on_window_closed(GtkWidget *window,gpointer note);
static gboolean on_note_trashed (BijiNoteObj *note, BjbNoteView *view);
static void on_note_color_changed_cb (BijiNoteObj *note, BjbNoteView *self);

static void
bjb_note_view_disconnect (BjbNoteView *self)
{
  g_signal_handlers_disconnect_by_func (self->window, on_window_closed, self->note);
  g_signal_handlers_disconnect_by_func (self->note, on_note_trashed, self);
  g_signal_handlers_disconnect_by_func (self->note, on_note_color_changed_cb, self);
}


static void
bjb_note_view_finalize(GObject *object)
{
  BjbNoteView *self = BJB_NOTE_VIEW (object) ;

  bjb_note_view_disconnect (self);

  g_clear_object (&self->view);

  G_OBJECT_CLASS (bjb_note_view_parent_class)->finalize (object);
}

static void
bjb_note_view_get_property (GObject      *object,
                            guint        prop_id,
                            GValue       *value,
                            GParamSpec   *pspec)
{
  BjbNoteView *self = BJB_NOTE_VIEW (object);

  switch (prop_id)
  {
    case PROP_WINDOW:
      g_value_set_object (value, self->window);
      break;
    case PROP_NOTE:
      g_value_set_object (value, self->note);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bjb_note_view_set_property ( GObject        *object,
                             guint          prop_id,
                             const GValue   *value,
                             GParamSpec     *pspec)
{
  BjbNoteView *self = BJB_NOTE_VIEW (object);

  switch (prop_id)
  {
    case PROP_WINDOW:
      self->window = g_value_get_object(value);
      break;
    case PROP_NOTE:
      self->note = g_value_get_object(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bjb_note_view_init (BjbNoteView *self)
{
}

static void
on_window_closed(GtkWidget *window,gpointer note)
{
  if ( window == NULL )
  {
    /* Note is deleted */
  }
}

/* Callbacks */

static void
just_switch_to_main_view(BjbNoteView *self)
{
  GtkWindow     *window;

  /* Avoid stupid crash */
  bjb_note_view_disconnect (self);

  window = GTK_WINDOW(self->window);
  bjb_window_base_switch_to (BJB_WINDOW_BASE (window),
                             BJB_WINDOW_BASE_MAIN_VIEW);
}

static gboolean
on_note_trashed (BijiNoteObj *note, BjbNoteView *view)
{
  just_switch_to_main_view (view);
  return TRUE;
}


static void
on_note_color_changed_cb (BijiNoteObj *note, BjbNoteView *self)
{
  const gchar *font_color;
  gchar *span, *text;
  GdkRGBA color;

  g_return_if_fail (BIJI_IS_NOTE_OBJ (note));

  biji_note_obj_get_rgba (note, &color);

  webkit_web_view_set_background_color (WEBKIT_WEB_VIEW (self->view), &color);

  if (color.red < 0.5)
    font_color = "white";
  else
    font_color = "black";

  /* Translators: %s is the note last recency description.
   * Last updated is placed as in left to right language
   * right to left languages might move %s
   *         '%s <b>Last Updated</b>'
   */
  text = g_strdup_printf (_("<b>Last updated</b> %s"),
                          biji_note_obj_get_last_change_date_string
			            (note));
  span = g_strdup_printf ("<span color='%s'>%s</span>", font_color, text);
  gtk_label_set_markup (GTK_LABEL (self->last_update), span);

  g_free (text);
  g_free (span);
}


/* Number of days since last updated
 * Instead we might want to play with a func to have a date
 * Also this might be integrated in text view */
static void
bjb_note_view_last_updated_actor_new (BjbNoteView *self)
{
  self->last_update = gtk_label_new ("");
  gtk_label_set_use_markup (GTK_LABEL (self->last_update), TRUE);
  on_note_color_changed_cb (self->note, self);
}


static void
bjb_note_view_constructed (GObject *obj)
{
  BjbNoteView            *self = BJB_NOTE_VIEW (obj);
  BjbSettings            *settings;
  gchar                  *default_font;
  GdkRGBA                 color;


  default_font = NULL;
  settings = bjb_app_get_settings(g_application_get_default());


  /* view new from note deserializes the note-content. */
  self->view = biji_note_obj_open (self->note);

  g_signal_connect(self->note,"deleted",
                   G_CALLBACK(on_note_trashed),self);
  g_signal_connect(self->note,"trashed",
                   G_CALLBACK(on_note_trashed),self);

  g_signal_connect(self->window,"destroy",
                   G_CALLBACK(on_window_closed), self->note);

  self->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_container_add (GTK_CONTAINER (self), self->box);
  gtk_widget_show (self->box);

  /* Text Editor (WebKitMainView) */
  gtk_box_pack_start (GTK_BOX (self->box), GTK_WIDGET(self->view), TRUE, TRUE, 0);
  gtk_widget_show (self->view);

  /* Apply the gsettings font */

  if (bjb_settings_use_system_font (settings))
     default_font = bjb_settings_get_system_font (settings);

  else
     default_font = g_strdup (bjb_settings_get_default_font (settings));

  if (default_font != NULL)
  {
    biji_webkit_editor_set_font (BIJI_WEBKIT_EDITOR (self->view), default_font);
    g_free (default_font);
  }


  /* User defined color */

  if (!biji_note_obj_get_rgba(self->note, &color))
  {
    if (gdk_rgba_parse (&color, bjb_settings_get_default_color (settings)))
      biji_note_obj_set_rgba (self->note, &color);
  }

  g_signal_connect (self->note, "color-changed",
                    G_CALLBACK (on_note_color_changed_cb), self);


  /* Edition Toolbar for text selection */
  self->edit_bar = bjb_editor_toolbar_new (self, self->note);
  gtk_box_pack_start (GTK_BOX (self->box), self->edit_bar, FALSE, TRUE, 0);
  gtk_widget_hide (self->edit_bar);

  /* Last updated row */
  bjb_note_view_last_updated_actor_new (self);
  gtk_widget_set_halign (self->last_update, GTK_ALIGN_END);
  gtk_widget_set_margin_end (self->last_update, 50);
  gtk_widget_set_valign (self->last_update, GTK_ALIGN_END);
  gtk_widget_set_margin_bottom (self->last_update, 50);
  gtk_overlay_add_overlay (GTK_OVERLAY (self), self->last_update);
  gtk_widget_show (self->last_update);
}

BjbNoteView *
bjb_note_view_new (GtkWidget *win, BijiNoteObj* note)
{
  return g_object_new (BJB_TYPE_NOTE_VIEW,
                       "window",win,
                       "note",note,
                       NULL);
}

static void
bjb_note_view_class_init (BjbNoteViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bjb_note_view_finalize;
  object_class->constructed = bjb_note_view_constructed;
  object_class->get_property = bjb_note_view_get_property;
  object_class->set_property = bjb_note_view_set_property;

  properties[PROP_WINDOW] = g_param_spec_object ("window",
                                                 "Window",
                                                 "Parent Window",
                                                 GTK_TYPE_WIDGET,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT |
                                                 G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class,PROP_WINDOW,properties[PROP_WINDOW]);

  properties[PROP_NOTE] = g_param_spec_object ("note",
                                               "Note",
                                               "Note",
                                               BIJI_TYPE_NOTE_OBJ,
                                               G_PARAM_READWRITE |
                                               G_PARAM_CONSTRUCT |
                                               G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class,PROP_NOTE,properties[PROP_NOTE]);
}

GtkWidget *
bjb_note_view_get_base_window (BjbNoteView *self)
{
  return self->window;
}



void
bjb_note_view_grab_focus     (BjbNoteView *self)
{
  gtk_widget_set_can_focus (self->view, TRUE);
  gtk_widget_grab_focus (self->view);
}
