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
#include "bjb-window.h"

struct _BjbEditorToolbar
{
  GtkActionBar   parent_instance;

  /* Note provide us the WebKitWebView editor */
  BijiNoteObj   *note;

  GtkAccelGroup *accel;

  GtkWidget     *bold_button;
  GtkWidget     *italic_button;
  GtkWidget     *strike_button;

  GtkWidget     *bullets_button;
  GtkWidget     *list_button;

  GtkWidget     *indent_button;
  GtkWidget     *outdent_button;

  GtkWidget     *window;
};

enum {
  FORMAT_APPLIED,
  COPY_CLICKED,
  N_SIGNALS
};

static guint signals[N_SIGNALS];

G_DEFINE_TYPE (BjbEditorToolbar, bjb_editor_toolbar, GTK_TYPE_ACTION_BAR)

static void
on_format_clicked (BjbEditorToolbar *self,
                   GtkWidget        *button)
{
  BijiEditorFormat format = BIJI_NO_FORMAT;

  g_assert (BJB_IS_EDITOR_TOOLBAR (self));
  g_assert (GTK_IS_BUTTON (button));

  if (button == self->bold_button)
    format = BIJI_BOLD;
  else if (button == self->italic_button)
    format = BIJI_ITALIC;
  else if (button == self->strike_button)
    format = BIJI_STRIKE;
  else if (button == self->bullets_button)
    format = BIJI_BULLET_LIST;
  else if (button == self->list_button)
    format = BIJI_ORDER_LIST;
  else if (button == self->indent_button)
    format = BIJI_INDENT;
  else if (button == self->outdent_button)
    format = BIJI_OUTDENT;
  else
    g_return_if_reached ();

  g_signal_emit (self, signals[FORMAT_APPLIED], 0, format);
}

static void
on_link_clicked (GtkButton        *button,
                 BjbEditorToolbar *self)
{
  g_signal_emit (self, signals[COPY_CLICKED], 0);
}

static void
bjb_editor_toolbar_map (GtkWidget *widget)
{
  BjbEditorToolbar *self = BJB_EDITOR_TOOLBAR (widget);

  GTK_WIDGET_CLASS (bjb_editor_toolbar_parent_class)->map (widget);

  if (self->window)
    return;

  self->window = gtk_widget_get_toplevel (GTK_WIDGET (self));
  g_object_ref (self->window);

  gtk_window_add_accel_group (GTK_WINDOW (self->window), self->accel);
}

static void
bjb_editor_toolbar_constructed (GObject *object)
{
  BjbEditorToolbar *self;

  G_OBJECT_CLASS (bjb_editor_toolbar_parent_class)->constructed (object);

  self = BJB_EDITOR_TOOLBAR (object);

  gtk_widget_add_accelerator (self->bold_button, "clicked", self->accel,
                              GDK_KEY_b, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (self->italic_button, "clicked", self->accel,
                              GDK_KEY_i, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  gtk_widget_add_accelerator (self->strike_button, "clicked", self->accel,
                              GDK_KEY_s, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);
}

static void
bjb_editor_toolbar_finalize (GObject *object)
{
  BjbEditorToolbar *self = BJB_EDITOR_TOOLBAR (object);

  if (self->window)
    gtk_window_remove_accel_group (GTK_WINDOW (self->window), self->accel);

  g_object_unref (self->accel);
  g_clear_object (&self->window);

  G_OBJECT_CLASS (bjb_editor_toolbar_parent_class)->finalize (object);
}

static void
bjb_editor_toolbar_class_init (BjbEditorToolbarClass *klass)
{
  GObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = G_OBJECT_CLASS (klass);
  widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = bjb_editor_toolbar_constructed;
  object_class->finalize = bjb_editor_toolbar_finalize;

  widget_class->map = bjb_editor_toolbar_map;

  signals[FORMAT_APPLIED] =
    g_signal_new ("format-applied",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 1, G_TYPE_INT);

  signals[COPY_CLICKED] =
    g_signal_new ("copy-clicked",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/editor-toolbar.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, bold_button);
  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, italic_button);
  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, strike_button);
  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, bullets_button);
  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, list_button);
  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, indent_button);
  gtk_widget_class_bind_template_child (widget_class, BjbEditorToolbar, outdent_button);

  gtk_widget_class_bind_template_callback (widget_class, on_format_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_link_clicked);
}

static void
bjb_editor_toolbar_init (BjbEditorToolbar *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  self->accel = gtk_accel_group_new ();
}

void
bjb_editor_toolbar_set_note (BjbEditorToolbar *self,
                             BijiNoteObj      *note)
{
  gboolean can_format = FALSE;

  g_return_if_fail (BJB_IS_EDITOR_TOOLBAR (self));
  g_return_if_fail (!note || BIJI_IS_NOTE_OBJ (note));

  self->note = note;

  if (note && biji_note_obj_can_format (note))
    can_format = TRUE;

  gtk_widget_set_sensitive (self->bold_button, can_format);
  gtk_widget_set_sensitive (self->italic_button, can_format);
  gtk_widget_set_sensitive (self->strike_button, can_format);

  gtk_widget_set_sensitive (self->bullets_button, can_format);
  gtk_widget_set_sensitive (self->list_button, can_format);

  gtk_widget_set_sensitive (self->indent_button, can_format);
  gtk_widget_set_sensitive (self->outdent_button, can_format);
}

void
bjb_editor_toolbar_set_can_format (BjbEditorToolbar *self,
                                   gboolean          can_format)
{
  gtk_widget_set_sensitive (self->bold_button, can_format);
  gtk_widget_set_sensitive (self->italic_button, can_format);
  gtk_widget_set_sensitive (self->strike_button, can_format);

  gtk_widget_set_sensitive (self->bullets_button, can_format);
  gtk_widget_set_sensitive (self->list_button, can_format);

  gtk_widget_set_sensitive (self->indent_button, can_format);
  gtk_widget_set_sensitive (self->outdent_button, can_format);
}
