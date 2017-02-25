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

/* Offset for toolbar related to cursor.
 * (Pixels)
 *
 * X offset might be replaced by something like -(toolbar size/2)
 * Y offset might not be replaced                    */
#define EDITOR_TOOLBAR_X_OFFSET -120;
#define EDITOR_TOOLBAR_Y_OFFSET   -15;

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libbiji/libbiji.h>

#include "bjb-bijiben.h"
#include "bjb-editor-toolbar.h"
#include "bjb-window-base.h"

#define CENTER_BUTTONS_SPACING 6

enum
{
  PROP_0,
  PROP_NOTE,
  PROP_BJB_NOTE_VIEW,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

struct _BjbEditorToolbarPrivate
{
  /* Note provide us the WebKitWebView editor */
  BjbNoteView        *view;
  BijiNoteObj        *note;

  GtkAccelGroup      *accel;

  GtkWidget          *box_actions;

  GtkWidget          *box_center;
  GtkWidget          *box_style;
  GtkWidget          *box_points;

  /* Do not use toggle buttons. uggly there.
   * Paste : the user might want to paste overriding selected text.
   * Other : when no selection the user won't try to bold "null".*/
  GtkWidget          *toolbar_cut;
  GtkWidget          *toolbar_copy;
  GtkWidget          *toolbar_paste;
  GtkWidget          *toolbar_bold;
  GtkWidget          *toolbar_italic;
  GtkWidget          *toolbar_strike;
  GtkWidget          *toolbar_bullet;
  GtkWidget          *toolbar_list;
  GtkWidget          *toolbar_link;
};

G_DEFINE_TYPE (BjbEditorToolbar, bjb_editor_toolbar, GTK_TYPE_ACTION_BAR);

static void
bjb_editor_toolbar_init (BjbEditorToolbar *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (
      self,
      BJB_TYPE_EDITOR_TOOLBAR,
      BjbEditorToolbarPrivate);

  self->priv->accel = gtk_accel_group_new ();
}


static void
bjb_editor_toolbar_get_property (GObject  *object,
                                 guint     property_id,
                                 GValue   *value,
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
bjb_editor_toolbar_set_property (GObject  *object,
                                 guint     property_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
  BjbEditorToolbar *self = BJB_EDITOR_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_BJB_NOTE_VIEW:
      self->priv->view = g_value_get_object (value);
      break;
    case PROP_NOTE:
      self->priv->note = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}

static gboolean
on_release_event (GtkWidget        *widget,
                  GdkEvent         *event,
                  BjbEditorToolbar *self)
{
  gtk_widget_set_visible (GTK_WIDGET (self),
                          biji_note_obj_editor_has_selection (self->priv->note));

  return FALSE;
}

static void
on_cut_clicked (GtkWidget        *button,
                BjbEditorToolbar *self)
{
  biji_note_obj_editor_cut (self->priv->note);
}

static void
on_copy_clicked (GtkWidget        *button,
                 BjbEditorToolbar *self)
{
  biji_note_obj_editor_copy (self->priv->note);
}

static void
on_paste_clicked (GtkWidget        *button,
                  BjbEditorToolbar *self)
{
  biji_note_obj_editor_paste (self->priv->note);
}

static void
bold_button_callback (GtkWidget        *button,
                      BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->priv->note, BIJI_BOLD);
}

static void
italic_button_callback (GtkWidget        *button,
                        BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->priv->note, BIJI_ITALIC);
}

static void
strike_button_callback (GtkWidget        *button,
                        BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->priv->note, BIJI_STRIKE);
}

static void
on_bullet_clicked (GtkWidget        *button,
                   BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->priv->note, BIJI_BULLET_LIST);
}

static void
on_list_clicked (GtkWidget        *button,
                 BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->priv->note, BIJI_ORDER_LIST);
}

static void
link_callback (GtkWidget        *button,
               BjbEditorToolbar *self)
{
  BjbSettings             *settings;
  gchar                   *link;
  GtkWidget               *window;
  BijiNoteObj             *result;
  GdkRGBA                  color;
  BijiManager             *manager;
  BjbEditorToolbarPrivate *priv = self->priv;

  link = biji_note_obj_editor_get_selection (priv->note);

  if (link == NULL)
    return;

  window = bjb_note_view_get_base_window (priv->view);
  manager = bjb_window_base_get_manager(window);

  settings = bjb_app_get_settings (g_application_get_default ());
  result = biji_manager_note_new (manager,
                                    link,
                                    bjb_settings_get_default_location (settings));

  /* Change result color. */
  if (biji_note_obj_get_rgba (priv->note, &color))
    biji_note_obj_set_rgba (result, &color);

  bijiben_new_window_for_note(g_application_get_default(), result);
}

static void
bjb_editor_toolbar_constructed (GObject *obj)
{
  BjbEditorToolbar        *self;
  BjbEditorToolbarPrivate *priv;
  GtkWidget               *view;
  GtkWidget               *window;
  gboolean                 can_format;

  G_OBJECT_CLASS (bjb_editor_toolbar_parent_class)->constructed (obj);

  self = BJB_EDITOR_TOOLBAR (obj);
  priv = self->priv;
  window = bjb_note_view_get_base_window (priv->view);
  can_format = biji_note_obj_can_format (priv->note);
  gtk_window_add_accel_group (GTK_WINDOW (window), priv->accel);

  /* Action Bar */
  gtk_style_context_add_class (gtk_widget_get_style_context (GTK_WIDGET (self)),
                               GTK_STYLE_CLASS_OSD);

  priv->box_actions = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (priv->box_actions);
  gtk_action_bar_pack_start (GTK_ACTION_BAR (self), priv->box_actions);

  /* Cut */
  priv->toolbar_cut = gtk_button_new_with_label (_("Cut"));
  gtk_button_set_use_underline (GTK_BUTTON (priv->toolbar_cut), TRUE);
  gtk_widget_show (priv->toolbar_cut);
  gtk_box_pack_start (GTK_BOX (priv->box_actions), priv->toolbar_cut, TRUE, TRUE, 0);

  /* Copy */
  priv->toolbar_copy = gtk_button_new_with_label (_("Copy"));
  gtk_button_set_use_underline (GTK_BUTTON (priv->toolbar_copy), TRUE);
  gtk_widget_show (priv->toolbar_copy);
  gtk_box_pack_start (GTK_BOX (priv->box_actions), priv->toolbar_copy, TRUE, TRUE, 0);

  /* 'n paste */
  priv->toolbar_paste = gtk_button_new_with_label (_("Paste"));
  gtk_button_set_use_underline (GTK_BUTTON (priv->toolbar_paste), TRUE);
  gtk_widget_show (priv->toolbar_paste);
  gtk_box_pack_start (GTK_BOX (priv->box_actions), priv->toolbar_paste, TRUE, TRUE, 0);

  priv->box_center = gtk_box_new (GTK_ORIENTATION_HORIZONTAL,CENTER_BUTTONS_SPACING);
  gtk_widget_show (priv->box_center);
  gtk_action_bar_set_center_widget (GTK_ACTION_BAR (self), priv->box_center);

  priv->box_style = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (priv->box_style);
  gtk_box_pack_start (GTK_BOX (priv->box_center), priv->box_style, FALSE, TRUE, 0);

  /* Bold */
  priv->toolbar_bold = gtk_button_new_from_icon_name ("format-text-bold-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_button_set_use_underline (GTK_BUTTON (priv->toolbar_bold), TRUE);
  gtk_widget_set_sensitive (priv->toolbar_bold, can_format);
  gtk_widget_show (priv->toolbar_bold);
  gtk_widget_set_tooltip_text (priv->toolbar_bold, _("Bold"));
  gtk_box_pack_start (GTK_BOX (priv->box_style), priv->toolbar_bold, TRUE, TRUE, 0);

  /* Italic */
  priv->toolbar_italic = gtk_button_new_from_icon_name ("format-text-italic-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_button_set_use_underline (GTK_BUTTON (priv->toolbar_italic), TRUE);
  gtk_widget_set_sensitive (priv->toolbar_italic, can_format);
  gtk_widget_show (priv->toolbar_italic);
  gtk_widget_set_tooltip_text (priv->toolbar_italic, _("Italic"));
  gtk_box_pack_start (GTK_BOX (priv->box_style), priv->toolbar_italic, TRUE, TRUE, 0);

  /* Strike */
  priv->toolbar_strike = gtk_button_new_from_icon_name ("format-text-strikethrough-symbolic", GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_button_set_use_underline (GTK_BUTTON (priv->toolbar_strike), TRUE);
  gtk_widget_set_sensitive (priv->toolbar_strike, can_format);
  gtk_widget_show (priv->toolbar_strike);
  gtk_widget_set_tooltip_text (priv->toolbar_strike, _("Strike"));
  gtk_box_pack_start (GTK_BOX (priv->box_style), priv->toolbar_strike, TRUE, TRUE, 0);

  priv->box_points = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_show (priv->box_points);
  gtk_box_pack_start (GTK_BOX (priv->box_center), priv->box_points, FALSE, TRUE, 0);

  /* Bullet */
  priv->toolbar_bullet = gtk_button_new_with_label (_("* "));
  gtk_button_set_use_underline (GTK_BUTTON (priv->toolbar_bullet), TRUE);
  gtk_widget_set_sensitive (priv->toolbar_bullet, can_format);
  gtk_widget_show (priv->toolbar_bullet);
  gtk_box_pack_start (GTK_BOX (priv->box_points), priv->toolbar_bullet, TRUE, TRUE, 0);

  /* Link */
  priv->toolbar_link = gtk_button_new_from_icon_name ("insert-link", GTK_ICON_SIZE_LARGE_TOOLBAR);
  gtk_button_set_use_underline (GTK_BUTTON (priv->toolbar_link), TRUE);
  gtk_widget_show (priv->toolbar_link);
  gtk_widget_set_tooltip_text (priv->toolbar_link, _("Copy selection to a new note"));
  gtk_action_bar_pack_end (GTK_ACTION_BAR (self), priv->toolbar_link);

  /* text selected --> fade in , and not selected --> fade out */
  view = biji_note_obj_get_editor (priv->note);

  g_signal_connect(view,"button-release-event",
                   G_CALLBACK(on_release_event),self);

  g_signal_connect(view,"key-release-event",
                   G_CALLBACK(on_release_event),self);

  /* buttons */

  g_signal_connect (priv->toolbar_cut,"clicked",
                    G_CALLBACK(on_cut_clicked), self);

  g_signal_connect (priv->toolbar_copy,"clicked",
                    G_CALLBACK(on_copy_clicked), self);

  g_signal_connect (priv->toolbar_paste,"clicked",
                    G_CALLBACK(on_paste_clicked), self);

  g_signal_connect (priv->toolbar_bullet,"clicked",
                    G_CALLBACK(on_bullet_clicked), self);

  g_signal_connect (priv->toolbar_list,"clicked",
                    G_CALLBACK(on_list_clicked), self);

  g_signal_connect (priv->toolbar_bold,"clicked",
                    G_CALLBACK(bold_button_callback), self);
  gtk_widget_add_accelerator (priv->toolbar_bold,
                              "clicked", priv->accel, GDK_KEY_b,
                              GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  g_signal_connect (priv->toolbar_italic,"clicked",
                    G_CALLBACK(italic_button_callback), self);
  gtk_widget_add_accelerator (priv->toolbar_italic,
                              "clicked", priv->accel, GDK_KEY_i,
                              GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  g_signal_connect (priv->toolbar_strike,"clicked",
                    G_CALLBACK(strike_button_callback), self);
  gtk_widget_add_accelerator (priv->toolbar_strike,
                              "clicked", priv->accel, GDK_KEY_s,
                              GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE);

  g_signal_connect (priv->toolbar_link,"clicked",
                    G_CALLBACK(link_callback), self);
}

static void
bjb_editor_toolbar_finalize (GObject *obj)
{
  BjbEditorToolbar *self = BJB_EDITOR_TOOLBAR (obj);
  BjbEditorToolbarPrivate *priv = self->priv;
  GtkWidget *window;

  window = bjb_note_view_get_base_window (priv->view);
  gtk_window_remove_accel_group (GTK_WINDOW (window), priv->accel);
  g_object_unref (priv->accel);

  G_OBJECT_CLASS (bjb_editor_toolbar_parent_class)->finalize (obj);
}

static void
bjb_editor_toolbar_class_init (BjbEditorToolbarClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = bjb_editor_toolbar_get_property ;
  object_class->set_property = bjb_editor_toolbar_set_property ;
  object_class->constructed = bjb_editor_toolbar_constructed ;
  object_class->finalize = bjb_editor_toolbar_finalize;



  properties[PROP_BJB_NOTE_VIEW] = g_param_spec_object ("bjbnoteview",
                                                        "bjbnoteview",
                                                        "bjbnoteview",
                                                        BJB_TYPE_NOTE_VIEW,
                                                        G_PARAM_READWRITE  |
                                                        G_PARAM_CONSTRUCT |
                                                        G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class,PROP_BJB_NOTE_VIEW,properties[PROP_BJB_NOTE_VIEW]);

  properties[PROP_NOTE] = g_param_spec_object ("note",
                                               "Note",
                                               "Biji Note Obj",
                                                BIJI_TYPE_NOTE_OBJ,
                                                G_PARAM_READWRITE  |
                                                G_PARAM_CONSTRUCT |
                                                G_PARAM_STATIC_STRINGS);

  g_object_class_install_property (object_class,PROP_NOTE,properties[PROP_NOTE]);

  g_type_class_add_private (class, sizeof (BjbEditorToolbarPrivate));
}


GtkWidget *
bjb_editor_toolbar_new (BjbNoteView    *bjb_note_view,
                        BijiNoteObj    *biji_note_obj)
{
  return g_object_new (BJB_TYPE_EDITOR_TOOLBAR,
                       "bjbnoteview" , bjb_note_view,
                       "note"        , biji_note_obj,
                       NULL);
}
