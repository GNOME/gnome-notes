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

#include "bjb-application.h"
#include "bjb-editor-toolbar.h"
#include "bjb-note-view.h"

enum
{
  PROP_0,
  PROP_NOTE,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

struct _BjbNoteView
{
  GtkOverlay         parent_instance;

  /* Data */
  GtkWidget         *view;
  BijiNoteObj       *note ;

  /* UI */
  BijiWebkitEditor  *editor;
  GtkWidget         *box;
  GtkWidget         *edit_bar;
  GtkWidget         *label;
  GtkWidget         *stack;
};

G_DEFINE_TYPE (BjbNoteView, bjb_note_view, GTK_TYPE_OVERLAY)

static void on_note_color_changed_cb (BijiNoteObj *note, BjbNoteView *self);

void
bjb_note_view_set_detached (BjbNoteView *self,
                            gboolean     detached)
{
  if (detached)
  {
    gtk_stack_set_visible_child (GTK_STACK (self->stack), self->label);
  }
  else
  {
    gtk_stack_set_visible_child (GTK_STACK (self->stack), self->box);
  }
}

static void
bjb_note_view_disconnect (BjbNoteView *self)
{
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
on_note_color_changed_cb (BijiNoteObj *note, BjbNoteView *self)
{
  GdkRGBA color;

  g_return_if_fail (BIJI_IS_NOTE_OBJ (note));

  biji_note_obj_get_rgba (note, &color);

  webkit_web_view_set_background_color (WEBKIT_WEB_VIEW (self->view), &color);
}

static void
view_font_changed_cb (BjbNoteView *self,
                      GParamSpec  *pspec,
                      BjbSettings *settings)
{
  g_autofree char *default_font = NULL;

  g_assert (BJB_IS_NOTE_VIEW (self));
  g_assert (BJB_IS_SETTINGS (settings));

  default_font = bjb_settings_get_font (settings);

  if (default_font != NULL)
    biji_webkit_editor_set_font (BIJI_WEBKIT_EDITOR (self->view), default_font);

  biji_webkit_editor_set_text_size (BIJI_WEBKIT_EDITOR (self->view),
                                    bjb_settings_get_text_size (settings));
}

static void
bjb_note_view_constructed (GObject *obj)
{
  BjbNoteView            *self = BJB_NOTE_VIEW (obj);
  BjbSettings            *settings;
  g_autofree gchar       *default_font = NULL;
  GdkRGBA                 color;
  BjbTextSizeType         text_size;

  settings = bjb_app_get_settings(g_application_get_default());

  g_signal_connect_object (settings, "notify::font",
                           G_CALLBACK (view_font_changed_cb),
                           self, G_CONNECT_SWAPPED);


  /* view new from note deserializes the note-content. */
  self->view = biji_note_obj_open (self->note);

  self->stack = gtk_stack_new ();
  gtk_widget_show (self->stack);
  gtk_container_add (GTK_CONTAINER (self), self->stack);

  /* Label used to indicate that note is opened in another window. */
  self->label = gtk_label_new (_("This note is being viewed in another window."));
  gtk_widget_show (self->label);
  gtk_stack_add_named (GTK_STACK (self->stack), self->label, "label");

  self->box = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_show (self->box);
  gtk_stack_add_named (GTK_STACK (self->stack), self->box, "note-box");
  gtk_stack_set_visible_child (GTK_STACK (self->stack), self->box);

  /* Text Editor (WebKitMainView) */
  gtk_box_pack_start (GTK_BOX (self->box), GTK_WIDGET(self->view), TRUE, TRUE, 0);
  gtk_widget_show (self->view);

  /* Apply the gsettings font */
  default_font = bjb_settings_get_font (settings);

  if (default_font != NULL)
    biji_webkit_editor_set_font (BIJI_WEBKIT_EDITOR (self->view), default_font);

  /* Apply the gsettings text size */
  text_size = bjb_settings_get_text_size (settings);
  biji_webkit_editor_set_text_size (BIJI_WEBKIT_EDITOR (self->view), text_size);

  /* User defined color */

  if (!biji_note_obj_get_rgba(self->note, &color))
  {
    if (gdk_rgba_parse (&color, bjb_settings_get_default_color (settings)))
      biji_note_obj_set_rgba (self->note, &color);
  }

  g_signal_connect (self->note, "color-changed",
                    G_CALLBACK (on_note_color_changed_cb), self);


  /* Edition Toolbar for text selection */
  if (!biji_note_obj_is_trashed (BIJI_NOTE_OBJ (self->note)))
    {
      self->edit_bar = g_object_new (BJB_TYPE_EDITOR_TOOLBAR, NULL);
      bjb_editor_toolbar_set_note (BJB_EDITOR_TOOLBAR (self->edit_bar), self->note);

      gtk_box_pack_start (GTK_BOX (self->box), self->edit_bar, FALSE, TRUE, 0);
    }
}

BjbNoteView *
bjb_note_view_new (BijiNoteObj *note)
{
  return g_object_new (BJB_TYPE_NOTE_VIEW,
                       "note", note,
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

  properties[PROP_NOTE] = g_param_spec_object ("note",
                                               "Note",
                                               "Note",
                                               BIJI_TYPE_NOTE_OBJ,
                                               G_PARAM_READWRITE |
                                               G_PARAM_CONSTRUCT |
                                               G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class,PROP_NOTE,properties[PROP_NOTE]);
}
