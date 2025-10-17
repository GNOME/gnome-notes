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
#include <adwaita.h>

#include "bjb-application.h"
#include "bjb-editor-toolbar.h"
#include "bjb-settings.h"
#include "items/bjb-xml-note.h"
#include "providers/bjb-provider.h"
#include "bjb-note-view.h"

struct _BjbNoteView
{
  GtkBox       parent_instance;

  /* UI */
  GtkWidget *main_stack;
  GtkWidget *status_page;
  GtkWidget *editor_box;
  GtkWidget *editor_toolbar;

  /* Data */
  GtkWidget         *view;
  BjbNote           *note;

  gboolean           is_self_change;
  gulong             modified_id;
  guint              save_id;
};

typedef struct _NoteData
{
  BjbNoteView *self;
  BjbItem     *note;
} NoteData;

G_DEFINE_TYPE (BjbNoteView, bjb_note_view, GTK_TYPE_BOX)

static void
note_data_free (gpointer user_data)
{
  NoteData *data = user_data;

  if (!data)
    return;

  g_set_weak_pointer (&data->self, NULL);
  g_clear_object (&data->note);
  g_free (data);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (NoteData, note_data_free)

static gboolean
note_view_save_item (gpointer user_data)
{
  NoteData *data = user_data;
  BjbNoteView *self;
  BjbProvider *provider;
  BjbItem *note;

  g_assert (data);

  self = data->self;
  note = data->note;
  g_assert (!self || BJB_IS_NOTE_VIEW (self));
  g_assert (BJB_IS_ITEM (note));

  provider = g_object_get_data (G_OBJECT (note), "provider");
  g_assert (BJB_IS_PROVIDER (provider));

  if (self)
    self->save_id = 0;

  bjb_provider_save_item_async (provider, note, NULL, NULL, NULL);

  return G_SOURCE_REMOVE;
}

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
  /* GApplication *app; */
  /* BjbSettings *settings; */
  /* BijiManager *manager; */
  /* BijiNoteObj *new_note; */
  /* const char *content; */
  /* GdkRGBA color; */

  /* g_assert (BJB_IS_NOTE_VIEW (self)); */

  /* content = biji_webkit_editor_get_selection (BIJI_WEBKIT_EDITOR (self->view)); */

  /* if (!content || !*content) */
  /*   return; */

  /* app = g_application_get_default (); */
  /* manager = bijiben_get_manager (BJB_APPLICATION (app)); */

  /* todo */
  /* settings = bjb_app_get_settings (app); */
  /* new_note = biji_manager_note_new (manager, content, */
  /*                                   bjb_settings_get_default_location (settings)); */

  /* if (self->note && bjb_item_get_rgba (BJB_ITEM (self->note), &color)) */
  /*   bjb_item_set_rgba (BJB_ITEM (new_note), &color); */

  /* bijiben_new_window_for_note (app, BJB_NOTE (new_note)); */
}

static void
on_note_color_changed_cb (BjbNoteView *self)
{
  GdkRGBA color;

  g_assert (BJB_IS_NOTE_VIEW (self));

  bjb_item_get_rgba (BJB_ITEM (self->note), &color);
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
bjb_note_view_finalize (GObject *object)
{
  BjbNoteView *self = (BjbNoteView *)object;

  if (self->save_id && self->note &&
      bjb_item_is_modified (BJB_ITEM (self->note)))
    {
      NoteData *data;

      data = g_new0 (NoteData, 1);
      g_set_object (&data->note, (BjbItem *)self->note);
      note_view_save_item (data);
    }

  g_clear_handle_id (&self->save_id, g_source_remove);

  g_clear_object (&self->note);
  g_clear_object (&self->view);

  G_OBJECT_CLASS (bjb_note_view_parent_class)->finalize (object);
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

  g_type_ensure (BJB_TYPE_EDITOR_TOOLBAR);
}

static void
bjb_note_view_init (BjbNoteView *self)
{
  BjbSettings *settings;

  gtk_widget_init_template (GTK_WIDGET (self));

  settings = bjb_settings_get_default ();
  g_signal_connect_object (settings, "notify::font",
                           G_CALLBACK (view_font_changed_cb),
                           self, G_CONNECT_SWAPPED);
  view_font_changed_cb (self, NULL, settings);
}

static void
note_view_note_modified_cb (BjbNoteView *self)
{
  NoteData *data;

  g_assert (BJB_IS_NOTE_VIEW (self));

  if (!self->note || self->is_self_change ||
      !bjb_item_is_modified (BJB_ITEM (self->note)) ||
      self->save_id)
    return;

  data = g_new0 (NoteData, 1);
  g_set_weak_pointer (&data->self, self);
  g_set_object (&data->note, (BjbItem *)self->note);

  self->save_id = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT, 10,
                                              note_view_save_item,
                                              data, note_data_free);
}

void
bjb_note_view_set_note (BjbNoteView *self,
                        BjbNote     *note)
{
  g_return_if_fail (BJB_IS_NOTE_VIEW (self));
  g_return_if_fail (!note || BJB_IS_ITEM (note));

  if (self->note == note)
    return;

  if (self->save_id && self->note &&
      bjb_item_is_modified (BJB_ITEM (self->note)))
    {
      g_autoptr(NoteData) data = NULL;

      data = g_new0 (NoteData, 1);
      g_set_weak_pointer (&data->self, self);
      g_set_object (&data->note, (BjbItem *)self->note);
      note_view_save_item (data);
    }

  g_clear_signal_handler (&self->modified_id, self->note);
  g_clear_handle_id (&self->save_id, g_source_remove);

  bjb_editor_toolbar_set_can_format (BJB_EDITOR_TOOLBAR (self->editor_toolbar),
                                     BJB_IS_XML_NOTE (note));

  if (note)
    gtk_widget_set_visible (self->editor_toolbar, !bjb_item_is_trashed (BJB_ITEM (note)));

  if (self->view)
    gtk_box_remove (GTK_BOX (self->editor_box), self->view);

  g_set_object (&self->note, note);
  g_clear_object (&self->view);

  if (note)
    {
      GdkRGBA color;

      /* Text Editor (WebKitMainView) */
      self->view = (GtkWidget *)biji_webkit_editor_new (note);
      gtk_widget_set_vexpand (self->view, TRUE);
      gtk_box_prepend (GTK_BOX (self->editor_box), self->view);

      self->modified_id = g_signal_connect_object (self->note, "notify::modified",
                                                   G_CALLBACK (note_view_note_modified_cb),
                                                   self, G_CONNECT_SWAPPED);
      if (!bjb_item_get_rgba (BJB_ITEM (note), &color))
        {
          BjbSettings *settings;

          settings = bjb_settings_get_default();

          self->is_self_change = TRUE;
          if (gdk_rgba_parse (&color, bjb_settings_get_default_color (settings)))
            bjb_item_set_rgba (BJB_ITEM (note), &color);
          bjb_item_unset_modified (BJB_ITEM (note));
          self->is_self_change = FALSE;
        }

      g_signal_connect_object (self->note, "notify::color",
                               G_CALLBACK (on_note_color_changed_cb),
                               self,
                               G_CONNECT_SWAPPED);

      gtk_widget_set_visible (self->view, TRUE);

      gtk_stack_set_visible_child (GTK_STACK (self->main_stack), self->editor_box);
    }
  else
    {
      gtk_stack_set_visible_child (GTK_STACK (self->main_stack), self->status_page);
      adw_status_page_set_title (ADW_STATUS_PAGE (self->status_page),
                                 _("No note selected"));
    }
}

BijiWebkitEditor *
bjb_note_view_get_editor (BjbNoteView *self)
{
  g_return_val_if_fail (BJB_IS_NOTE_VIEW (self), NULL);

  return (BijiWebkitEditor *)self->view;
}
