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
#include <handy.h>

#include "bjb-application.h"
#include "bjb-editor-toolbar.h"
#include "bjb-note-view.h"

struct _BjbNoteView
{
  GtkOverlay         parent_instance;

  /* UI */
  GtkWidget *main_stack;
  GtkWidget *status_page;
  GtkWidget *editor_box;
  GtkWidget *editor_toolbar;

  /* Data */
  GtkWidget         *view;
  BijiNoteObj       *note ;
};

G_DEFINE_TYPE (BjbNoteView, bjb_note_view, GTK_TYPE_OVERLAY)

static void on_note_color_changed_cb (BijiNoteObj *note, BjbNoteView *self);

static void
note_view_format_applied_cb (BjbNoteView      *self,
                             BijiEditorFormat  format)
{
  g_assert (BJB_IS_NOTE_VIEW (self));

  if (self->view)
    biji_webkit_editor_apply_format (BIJI_WEBKIT_EDITOR (self->view), format);
}

static void
note_view_copy_clicked_cb (BjbNoteView *self)
{
  GApplication *app;
  BjbSettings *settings;
  BijiManager *manager;
  BijiNoteObj *new_note;
  const char *content;
  GdkRGBA color;

  g_assert (BJB_IS_NOTE_VIEW (self));

  content = biji_webkit_editor_get_selection (BIJI_WEBKIT_EDITOR (self->view));

  if (!content || !*content)
    return;

  app = g_application_get_default ();
  manager = bijiben_get_manager (BJB_APPLICATION (app));

  settings = bjb_app_get_settings (app);
  new_note = biji_manager_note_new (manager, content,
                                    bjb_settings_get_default_location (settings));

  if (biji_note_obj_get_rgba (self->note, &color))
    biji_note_obj_set_rgba (new_note, &color);

  bijiben_new_window_for_note (app, new_note);
}

void
bjb_note_view_set_detached (BjbNoteView *self,
                            gboolean     detached)
{
  if (detached)
  {
    gtk_stack_set_visible_child (GTK_STACK (self->main_stack), self->status_page);
    hdy_status_page_set_title (HDY_STATUS_PAGE (self->status_page),
                               _("This note is being viewed in another window"));
  }
  else
  {
    gtk_stack_set_visible_child (GTK_STACK (self->main_stack), self->editor_box);
  }
}

static void
bjb_note_view_disconnect (BjbNoteView *self)
{
  if (self->note)
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

  if (!self->view)
    return;

  default_font = bjb_settings_get_font (settings);

  if (default_font != NULL)
    biji_webkit_editor_set_font (BIJI_WEBKIT_EDITOR (self->view), default_font);

  biji_webkit_editor_set_text_size (BIJI_WEBKIT_EDITOR (self->view),
                                    bjb_settings_get_text_size (settings));
}

static void
bjb_note_view_class_init (BjbNoteViewClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bjb_note_view_finalize;

  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Notes"
                                               "/ui/bjb-note-view.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbNoteView, main_stack);
  gtk_widget_class_bind_template_child (widget_class, BjbNoteView, status_page);
  gtk_widget_class_bind_template_child (widget_class, BjbNoteView, editor_box);
  gtk_widget_class_bind_template_child (widget_class, BjbNoteView, editor_toolbar);

  gtk_widget_class_bind_template_callback (widget_class, note_view_format_applied_cb);
  gtk_widget_class_bind_template_callback (widget_class, note_view_copy_clicked_cb);
}

static void
bjb_note_view_init (BjbNoteView *self)
{
  BjbSettings *settings;

  gtk_widget_init_template (GTK_WIDGET (self));

  settings = bjb_app_get_settings (g_application_get_default ());
  g_signal_connect_object (settings, "notify::font",
                           G_CALLBACK (view_font_changed_cb),
                           self, G_CONNECT_SWAPPED);
  view_font_changed_cb (self, NULL, settings);
}

void
bjb_note_view_set_note (BjbNoteView *self,
                        BijiNoteObj *note)
{
  gboolean can_format = FALSE;

  g_return_if_fail (BJB_IS_NOTE_VIEW (self));
  g_return_if_fail (!note || BIJI_IS_NOTE_OBJ (note));

  if (self->note == note)
    return;

  if (note)
    can_format = biji_note_obj_can_format (note);
  bjb_editor_toolbar_set_can_format (BJB_EDITOR_TOOLBAR (self->editor_toolbar), can_format);

  if (note)
    gtk_widget_set_visible (self->editor_toolbar, !bjb_item_is_trashed (BJB_ITEM (note)));
  bjb_note_view_disconnect (self);

  self->note = note;
  if (self->view)
    gtk_widget_destroy (self->view);
  g_clear_object (&self->view);

  if (note)
    {
      GdkRGBA color;

      /* Text Editor (WebKitMainView) */
      self->view = (GtkWidget *)biji_webkit_editor_new (note);
      gtk_widget_show (self->view);
      gtk_box_pack_start (GTK_BOX (self->editor_box), GTK_WIDGET(self->view), TRUE, TRUE, 0);

      if (!biji_note_obj_get_rgba (self->note, &color))
        {
          BjbSettings *settings;

          settings = bjb_app_get_settings (g_application_get_default ());

          if (gdk_rgba_parse (&color, bjb_settings_get_default_color (settings)))
            biji_note_obj_set_rgba (self->note, &color);
        }

      g_signal_connect (self->note, "color-changed",
                        G_CALLBACK (on_note_color_changed_cb), self);

      gtk_widget_show (self->view);

      gtk_stack_set_visible_child (GTK_STACK (self->main_stack), self->editor_box);
    }
  else
    {
      gtk_stack_set_visible_child (GTK_STACK (self->main_stack), self->status_page);
      hdy_status_page_set_title (HDY_STATUS_PAGE (self->status_page),
                                 _("No note selected"));
    }
}

BijiWebkitEditor *
bjb_note_view_get_editor (BjbNoteView *self)
{
  g_return_val_if_fail (BJB_IS_NOTE_VIEW (self), NULL);

  return (BijiWebkitEditor *)self->view;
}
