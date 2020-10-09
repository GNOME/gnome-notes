/*
 * bjb-detached-window.c
 * Copyright (C) 2020 Jonathan Kang <jonathankang@gnome.org>
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

#include "bjb-detached-window.h"
#include "bjb-note-view.h"
#include "bjb-organize-dialog.h"
#include "bjb-share.h"

enum
{
  PROP_0,
  PROP_NOTE,
  NUM_PROPERTIES
};

struct _BjbDetachedWindow
{
  HdyApplicationWindow parent_instance;

  BijiNoteObj *note;

  GtkWidget *headerbar;
  GtkWidget *main_box;
  GtkWidget *title_entry;
  GtkWidget *last_update_item;
};

G_DEFINE_TYPE (BjbDetachedWindow, bjb_detached_window, HDY_TYPE_APPLICATION_WINDOW)

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

static void
on_note_renamed (BijiItem          *note,
                 BjbDetachedWindow *self)
{
  const char *str;

  str = biji_item_get_title (note);
  if (str == NULL || strlen(str) == 0)
    str = _("Untitled");
  gtk_entry_set_text (GTK_ENTRY (self->title_entry), str);
  hdy_header_bar_set_custom_title (HDY_HEADER_BAR (self->headerbar),
                                   self->title_entry);
  gtk_widget_show (self->title_entry);
}

static void
on_last_updated_cb (BijiItem          *note,
                    BjbDetachedWindow *self)
{
  g_autofree char *label = NULL;
  g_autofree char *time_str = NULL;

  time_str = biji_note_obj_get_last_change_date_string (self->note);
  /* Translators: %s is the note last recency description.
   * Last updated is placed as in left to right language
   * right to left languages might move %s
   *         '%s Last Updated'
   */
  label = g_strdup_printf (_("Last updated: %s"), time_str);
  gtk_label_set_text (GTK_LABEL (self->last_update_item), label);
}

static void
on_title_changed (BjbDetachedWindow *self,
                  GtkEntry          *title)
{
  const char *str = gtk_entry_get_text (title);

  if (str && *str)
    biji_note_obj_set_title (self->note, str);
}

static void
on_paste_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  BjbDetachedWindow *self = BJB_DETACHED_WINDOW (user_data);
  BijiNoteObj *note = self->note;

  if (!note)
    return;

  biji_webkit_editor_paste (BIJI_WEBKIT_EDITOR (biji_note_obj_get_editor (note)));
}

static void
on_undo_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  BjbDetachedWindow *self = BJB_DETACHED_WINDOW (user_data);
  BijiNoteObj *note = self->note;

  if (!note)
    return;

  biji_webkit_editor_undo (BIJI_WEBKIT_EDITOR (biji_note_obj_get_editor (note)));
}

static void
on_redo_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  BjbDetachedWindow *self = BJB_DETACHED_WINDOW (user_data);
  BijiNoteObj *note = self->note;

  if (!note)
    return;

  biji_webkit_editor_redo (BIJI_WEBKIT_EDITOR (biji_note_obj_get_editor (note)));
}

static void
on_view_notebooks_cb (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  BjbDetachedWindow *self = BJB_DETACHED_WINDOW (user_data);
  BijiNoteObj *note = self->note;
  g_autoptr (GList) list = NULL;

  if (!note)
    return;

  list = g_list_append (list, note);
  bjb_organize_dialog_new (GTK_WINDOW (self), list);
}

static void
on_email_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  BjbDetachedWindow *self = BJB_DETACHED_WINDOW (user_data);
  BijiNoteObj *note = self->note;

  if (!note)
    return;

  on_email_note_callback (note);
}

static void
on_trash_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  BjbDetachedWindow *self = BJB_DETACHED_WINDOW (user_data);
  BijiNoteObj *note = self->note;

  if (!note)
    return;

  /* Delete the note from notebook
   * The deleted note will emit a signal. */
  biji_item_trash (BIJI_ITEM (note));
}

static void
on_close (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
  GtkApplicationWindow *window;

  window = GTK_APPLICATION_WINDOW (user_data);

  gtk_window_close (GTK_WINDOW (window));
}

static GActionEntry win_entries[] = {
  { "paste", on_paste_cb, NULL, NULL, NULL },
  { "undo", on_undo_cb, NULL, NULL, NULL },
  { "redo", on_redo_cb, NULL, NULL, NULL },
  { "view-notebooks", on_view_notebooks_cb, NULL, NULL, NULL },
  { "email", on_email_cb, NULL, NULL, NULL },
  { "trash", on_trash_cb, NULL, NULL, NULL },
  { "close", on_close },
};

static void
bjb_detached_window_constructed (GObject *object)
{
  gboolean is_maximized;
  int width, height;
  int pos_x, pos_y;
  BjbNoteView *note_view;
  BjbDetachedWindow *self = BJB_DETACHED_WINDOW (object);
  g_autoptr(GSettings) settings;

  G_OBJECT_CLASS (bjb_detached_window_parent_class)->constructed (object);

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   win_entries,
                                   G_N_ELEMENTS (win_entries),
                                   self);

  settings = g_settings_new ("org.gnome.Notes");

  g_settings_get (settings, "window-size", "(ii)", &width, &height);
  g_settings_get (settings, "window-position", "(ii)", &pos_x, &pos_y);
  gtk_window_set_default_size (GTK_WINDOW (self), width, height);

  is_maximized = g_settings_get_boolean (settings, "window-maximized");
  if (is_maximized)
    gtk_window_maximize (GTK_WINDOW (self));
  else if (pos_x >= 0)
    gtk_window_move (GTK_WINDOW (self), pos_x, pos_y);

  on_note_renamed (BIJI_ITEM (self->note), self);
  g_signal_connect (self->note, "renamed", G_CALLBACK (on_note_renamed), self);

  on_last_updated_cb (BIJI_ITEM (self->note), self);
  g_signal_connect (self->note, "changed", G_CALLBACK (on_last_updated_cb), self);

  note_view = bjb_note_view_new (GTK_WIDGET (self), self->note);
  gtk_box_pack_end (GTK_BOX (self->main_box), GTK_WIDGET (note_view), TRUE, TRUE, 0);
  gtk_widget_show (GTK_WIDGET (note_view));
}

static void
bjb_detached_window_finalize (GObject *object)
{
  BjbDetachedWindow *self = BJB_DETACHED_WINDOW (object);

  g_object_unref (self->note);

  G_OBJECT_CLASS (bjb_detached_window_parent_class)->finalize (object);
}

static void
bjb_detached_window_get_property (GObject    *object,
                                  guint       property_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BjbDetachedWindow *self = BJB_DETACHED_WINDOW (object);

  switch (property_id)
  {
  case PROP_NOTE:
    g_value_set_object (value, self->note);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bjb_detached_window_set_property (GObject      *object,
                                  guint         property_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BjbDetachedWindow *self = BJB_DETACHED_WINDOW (object);

  switch (property_id)
  {
  case PROP_NOTE:
    self->note = g_value_dup_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bjb_detached_window_class_init (BjbDetachedWindowClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->constructed = bjb_detached_window_constructed;
  object_class->finalize = bjb_detached_window_finalize;
  object_class->get_property = bjb_detached_window_get_property;
  object_class->set_property = bjb_detached_window_set_property;

  properties[PROP_NOTE] = g_param_spec_object ("note",
                                               "NoteObj",
                                               "Currently opened note",
                                               BIJI_TYPE_NOTE_OBJ,
                                               G_PARAM_READWRITE |
                                               G_PARAM_CONSTRUCT |
                                               G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/bjb-detached-window.ui");
  gtk_widget_class_bind_template_child (widget_class, BjbDetachedWindow, headerbar);
  gtk_widget_class_bind_template_child (widget_class, BjbDetachedWindow, main_box);
  gtk_widget_class_bind_template_child (widget_class, BjbDetachedWindow, title_entry);
  gtk_widget_class_bind_template_child (widget_class, BjbDetachedWindow, last_update_item);
  gtk_widget_class_bind_template_callback (widget_class, on_title_changed);
}

static void
bjb_detached_window_init (BjbDetachedWindow *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

BjbDetachedWindow *
bjb_detached_window_new (BijiNoteObj *note)
{
  return g_object_new (BJB_TYPE_DETACHED_WINDOW,
                       "application", g_application_get_default (),
                       "note", note,
                       NULL);
}
