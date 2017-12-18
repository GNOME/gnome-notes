/* bjb-editor-toolbar.c
 * Copyright © 2012, 2013 Red Hat, Inc.
 * Copyright © 2013 Pierre-Yves LUYTEN <py@luyten.fr>
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

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libbiji/libbiji.h>

#include "bjb-application.h"
#include "bjb-editor-toolbar.h"
#include "bjb-window-base.h"

enum
{
  PROP_0,
  PROP_NOTE,
  PROP_NOTE_VIEW,
  NUM_PROPERTIES
};

struct _BjbEditorToolbar
{
  GtkActionBar   parent_instance;

  /* Note provide us the WebKitWebView editor */
  BjbNoteView   *view;
  BijiNoteObj   *note;

  GtkAccelGroup *accel;

  GtkWidget     *bold_button;
  GtkWidget     *italic_button;
  GtkWidget     *strike_button;

  GtkWidget     *bullets_button;
  GtkWidget     *list_button;
};

G_DEFINE_TYPE (BjbEditorToolbar, bjb_editor_toolbar, GTK_TYPE_ACTION_BAR)

static gboolean
on_release_event (GtkWidget        *widget,
                  GdkEvent         *event,
                  BjbEditorToolbar *self)
{
  gtk_widget_set_visible (GTK_WIDGET (self),
                          biji_note_obj_editor_has_selection (self->note));

  return FALSE;
}

static void
on_cut_clicked (GtkButton        *button,
                BjbEditorToolbar *self)
{
  biji_note_obj_editor_cut (self->note);

  gtk_widget_hide (GTK_WIDGET (self));
}

static void
on_copy_clicked (GtkButton        *button,
                 BjbEditorToolbar *self)
{
  biji_note_obj_editor_copy (self->note);
}

static void
on_paste_clicked (GtkButton        *button,
                  BjbEditorToolbar *self)
{
  biji_note_obj_editor_paste (self->note);

  gtk_widget_hide (GTK_WIDGET (self));
}

static void
on_bold_clicked (GtkButton        *button,
                 BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->note, BIJI_BOLD);
}

static void
on_italic_clicked (GtkButton        *button,
                   BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->note, BIJI_ITALIC);
}

static void
on_strike_clicked (GtkButton        *button,
                   BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->note, BIJI_STRIKE);
}

static void
on_bullets_clicked (GtkButton        *button,
                    BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->note, BIJI_BULLET_LIST);
}

static void
on_list_clicked (GtkButton        *button,
                 BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->note, BIJI_ORDER_LIST);
}

static void
on_link_clicked (GtkButton        *button,
                 BjbEditorToolbar *self)
{
  BjbSettings             *settings;
  const gchar             *link;
  GtkWidget               *window;
  BijiNoteObj             *result;
  GdkRGBA                  color;
  BijiManager             *manager;

  link = biji_note_obj_editor_get_selection (self->note);

  if (link == NULL)
    return;

  window = bjb_note_view_get_base_window (self->view);
  manager = bjb_window_base_get_manager(window);

  settings = bjb_app_get_settings (g_application_get_default ());
  result = biji_manager_note_new (manager,
                                    link,
                                    bjb_settings_get_default_location (settings));

  /* Change result color. */
  if (biji_note_obj_get_rgba (self->note, &color))
    biji_note_obj_set_rgba (result, &color);

  bijiben_new_window_for_note (g_application_get_default (), result);
}

static void
bjb_editor_toolbar_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  switch (property_id)
  {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bjb_editor_toolbar_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BjbEditorToolbar *self = BJB_EDITOR_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_NOTE:
      self->note = g_value_get_object (value);
      break;
    case PROP_NOTE_VIEW:
      self->view = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static void
bjb_editor_toolbar_constructed (GObject *object)
{
  BjbEditorToolbar *self;
  GtkWidget        *view;
  GtkWidget        *window;
  gboolean          can_format;

  G_OBJECT_CLASS (bjb_editor_toolbar_parent_class)->constructed (object);

  self = BJB_EDITOR_TOOLBAR (object);

  window = bjb_note_view_get_base_window (self->view);
  gtk_window_add_accel_group (GTK_WINDOW (window), self->accel);

  gtk_widget_add_accelerator (self->bold_button, "clicked", self->accel,
                              GDK_KEY_b, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (self->italic_button, "clicked", self->accel,
                              GDK_KEY_i, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (self->strike_button, "clicked", self->accel,
                              GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  view = biji_note_obj_get_editor (self->note);

  g_signal_connect (view,"button-release-event",
                    G_CALLBACK (on_release_event), self);

  g_signal_connect (view,"key-release-event",
                   G_CALLBACK (on_release_event), self);

  can_format = biji_note_obj_can_format (self->note);

  gtk_widget_set_sensitive (self->bold_button, can_format);
  gtk_widget_set_sensitive (self->italic_button, can_format);
  gtk_widget_set_sensitive (self->strike_button, can_format);

  gtk_widget_set_sensitive (self->bullets_button, can_format);
  gtk_widget_set_sensitive (self->list_button, can_format);
}

static void
bjb_editor_toolbar_finalize (GObject *object)
{
  BjbEditorToolbar *self = BJB_EDITOR_TOOLBAR (object);
  GtkWidget *window;

  window = bjb_note_view_get_base_window (self->view);
  gtk_window_remove_accel_group (GTK_WINDOW (window), self->accel);
  g_object_unref (self->accel);

  G_OBJECT_CLASS (bjb_editor_toolbar_parent_class)->finalize (object);
}

static void
bjb_editor_toolbar_class_init (BjbEditorToolbarClass *klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = G_OBJECT_CLASS (klass);
  object_class->get_property = bjb_editor_toolbar_get_property;
  object_class->set_property = bjb_editor_toolbar_set_property;
  object_class->constructed = bjb_editor_toolbar_constructed;
  object_class->finalize = bjb_editor_toolbar_finalize;

  g_object_class_install_property (object_class,
                                   PROP_NOTE,
                                   g_param_spec_object ("note",
                                                        "Note",
                                                        "Biji Note Obj",
                                                        BIJI_TYPE_NOTE_OBJ,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,
                                   PROP_NOTE_VIEW,
                                   g_param_spec_object ("note-view",
                                                        "Note View",
                                                        "Note View",
                                                        BJB_TYPE_NOTE_VIEW,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS));

  widget_class = GTK_WIDGET_CLASS (klass);
  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/bijiben/editor-toolbar.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, bold_button);
  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, italic_button);
  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, strike_button);
  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, bullets_button);
  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, list_button);

  gtk_widget_class_bind_template_callback (widget_class, on_cut_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_copy_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_paste_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_bold_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_italic_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_strike_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_bullets_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_list_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_link_clicked);
}

static void
bjb_editor_toolbar_init (BjbEditorToolbar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->accel = gtk_accel_group_new ();
}

GtkWidget *
bjb_editor_toolbar_new (BjbNoteView *bjb_note_view,
                        BijiNoteObj *biji_note_obj)
{
  return g_object_new (BJB_TYPE_EDITOR_TOOLBAR,
                       "note"     , biji_note_obj,
                       "note-view", bjb_note_view,
                       NULL);
}
