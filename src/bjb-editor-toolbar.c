/* bjb-editor-toolbar.c
 * Copyright © 2012, 2013 Red Hat, Inc.
 * Copyright © 2013 Pierre-Yves LUYTEN <py@luyten.fr>
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

  GtkWidget          *widget;
  GtkAccelGroup      *accel;

  /* If user rigth-clicks we want to keep the toolbar visible
   * untill user changes his mind */
  gboolean           glued;

  /* Do not use toggle buttons. uggly there.
   * Paste : the user might want to paste overriding selected text.
   * Other : when no selection the user won't try to bold "null".*/
  GtkWidget          *box;
  GtkWidget          *toolbar_cut;
  GtkWidget          *toolbar_copy;
  GtkWidget          *toolbar_paste;
  GtkWidget          *toolbar_bold;
  GtkWidget          *toolbar_italic;
  GtkWidget          *toolbar_strike;
  GtkWidget          *toolbar_link;
};

G_DEFINE_TYPE (BjbEditorToolbar, bjb_editor_toolbar, G_TYPE_OBJECT);

static void
bjb_editor_toolbar_fade_in (BjbEditorToolbar *self)
{
  BjbEditorToolbarPrivate *priv = self->priv;
  gtk_widget_show (priv->widget);
}



static void
bjb_editor_toolbar_fade_out (BjbEditorToolbar *self)
{
  gtk_widget_hide (self->priv->widget);
}


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


/* TODO identify selected text. if some text is selected,
 * compute x (left), y (top), width (columns), height (rows) */
static void
editor_toolbar_align (BjbEditorToolbar *self, GdkEvent  *event)
{
  gint                     x_alignment, y_alignment;
  BjbEditorToolbarPrivate *priv = self->priv;
  cairo_rectangle_int_t    rect;

  x_alignment = event->button.x;// + EDITOR_TOOLBAR_X_OFFSET;
  y_alignment = event->button.y + EDITOR_TOOLBAR_Y_OFFSET;

  if ( x_alignment < 0)
    x_alignment = 0;

  rect.x = x_alignment;
  rect.y = y_alignment;
  rect.width = 1;
  rect.height = 1;

  gtk_popover_set_pointing_to (GTK_POPOVER (priv->widget), &rect);
}

static void
show_edit_bar (BjbEditorToolbar *self, GdkEvent *event)
{
  if (event)
    editor_toolbar_align (self, event);

  bjb_editor_toolbar_fade_in (self);
}

static gboolean
on_button_released (GtkWidget *widget,
                    GdkEvent *event,
                    BjbEditorToolbar *self)
{
  switch (event->button.button)
  {
    /* If left click, see if selection */
    case 1:
      if (biji_note_obj_editor_has_selection (self->priv->note))
        show_edit_bar (self, event);

      else
        bjb_editor_toolbar_fade_out (self);

      return FALSE;

    default:
      return FALSE;
  }
}

static gboolean
on_key_released                     (GtkWidget *widget,
                                     GdkEvent  *event,
                                     gpointer   user_data)
{
  BjbEditorToolbar *self = BJB_EDITOR_TOOLBAR (user_data);

  if (biji_note_obj_editor_has_selection (self->priv->note))
    show_edit_bar (self, event);

  else
    bjb_editor_toolbar_fade_out (self);

  return FALSE;
}

static gboolean
on_button_pressed (GtkWidget *widget,
                   GdkEvent  *event,
                   BjbEditorToolbar *self)
{
  switch (event->button.button)
  {
    /* Show toolbar on right-click */
    case 3:
      show_edit_bar (self, event);
      return TRUE;

    /* Do not break stuff otherwise */
    default :
      return FALSE;
  }
}

static gboolean
on_cut_clicked (GtkWidget *button, BjbEditorToolbar *self)
{
  biji_note_obj_editor_cut (self->priv->note);
  bjb_editor_toolbar_fade_out (self);
  return TRUE ;
}

static gboolean
on_copy_clicked (GtkWidget *button, BjbEditorToolbar *self)
{
  biji_note_obj_editor_copy (self->priv->note);
  bjb_editor_toolbar_fade_out (self);
  return TRUE ;
}

static gboolean
on_paste_clicked (GtkWidget *button, BjbEditorToolbar *self)
{
  biji_note_obj_editor_paste (self->priv->note);
  bjb_editor_toolbar_fade_out (self);
  return TRUE ;
}

static void
bold_button_callback (GtkWidget *button, BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->priv->note, BIJI_BOLD);
  bjb_editor_toolbar_fade_out (self);
}

static void
italic_button_callback (GtkWidget *button, BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->priv->note, BIJI_ITALIC);
  bjb_editor_toolbar_fade_out (self);
}

static void
strike_button_callback (GtkWidget *button, BjbEditorToolbar *self)
{
  biji_note_obj_editor_apply_format (self->priv->note, BIJI_STRIKE);
  bjb_editor_toolbar_fade_out (self);
}

static void
link_callback (GtkWidget *button, BjbEditorToolbar *self)
{
  BjbSettings             *settings;
  gchar                   *link;
  GtkWidget               *window;
  BijiNoteObj             *result;
  GdkRGBA                 color;
  BijiManager            *manager;
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
  bjb_editor_toolbar_fade_out (self);
}

static void
bjb_editor_toolbar_constructed (GObject *obj)
{
  BjbEditorToolbar          *self;
  BjbEditorToolbarPrivate   *priv;
  GtkWidget                 *view;
  GtkWidget                 *window;
  GtkWidget                 *image;
  GdkPixbuf                 *pixbuf;
  gchar                     *icons_path, *full_path;
  GError                    *error = NULL;

  G_OBJECT_CLASS (bjb_editor_toolbar_parent_class)->constructed (obj);

  self = BJB_EDITOR_TOOLBAR (obj);
  priv = self->priv;
  window = bjb_note_view_get_base_window (priv->view);
  gtk_window_add_accel_group (GTK_WINDOW (window), priv->accel);


  /* Popover */
  priv->widget = gtk_popover_new (GTK_WIDGET (priv->view));
  gtk_style_context_add_class (gtk_widget_get_style_context (priv->widget),
                               GTK_STYLE_CLASS_OSD);
  gtk_popover_set_position (GTK_POPOVER (priv->widget),
                            GTK_POS_TOP);
  gtk_popover_set_modal (GTK_POPOVER (priv->widget), FALSE);


  /* Toolbar */
  priv->box = GTK_WIDGET (gtk_toolbar_new ());
  gtk_toolbar_set_style (GTK_TOOLBAR (priv->box), GTK_TOOLBAR_TEXT);
  gtk_toolbar_set_show_arrow (GTK_TOOLBAR (priv->box), FALSE);
  gtk_widget_show (priv->box);
  gtk_container_add (GTK_CONTAINER (priv->widget), priv->box);


  /* Cut */
  priv->toolbar_cut = GTK_WIDGET (gtk_tool_button_new (NULL, _("Cut")));
  gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (priv->toolbar_cut), TRUE);
  gtk_widget_show (priv->toolbar_cut);
  gtk_toolbar_insert (GTK_TOOLBAR (priv->box), GTK_TOOL_ITEM (priv->toolbar_cut), -1);


  /* Copy */
  priv->toolbar_copy = GTK_WIDGET (gtk_tool_button_new (NULL, _("Copy")));
  gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (priv->toolbar_copy), TRUE);
  gtk_widget_show (priv->toolbar_copy);
  gtk_toolbar_insert (GTK_TOOLBAR (priv->box), GTK_TOOL_ITEM (priv->toolbar_copy), -1);

  /* 'n paste */
  priv->toolbar_paste = GTK_WIDGET (gtk_tool_button_new (NULL, _("Paste")));
  gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (priv->toolbar_paste), TRUE);
  gtk_widget_show (priv->toolbar_paste);
  gtk_toolbar_insert (GTK_TOOLBAR (priv->box), GTK_TOOL_ITEM (priv->toolbar_paste), -1);

  if (biji_note_obj_can_format (priv->note))
  {

    /* Bold */
    image = gtk_image_new_from_icon_name ("format-text-bold-symbolic", GTK_ICON_SIZE_INVALID);
    gtk_image_set_pixel_size (GTK_IMAGE (image), 24);
    priv->toolbar_bold = GTK_WIDGET (gtk_tool_button_new (image, NULL));
    gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (priv->toolbar_bold), TRUE);
    gtk_widget_show_all (priv->toolbar_bold);
    gtk_toolbar_insert (GTK_TOOLBAR (priv->box), GTK_TOOL_ITEM (priv->toolbar_bold), -1);
    gtk_widget_set_tooltip_text (GTK_WIDGET (priv->toolbar_bold), _("Bold"));


    /* Italic */
    image = gtk_image_new_from_icon_name ("format-text-italic-symbolic", GTK_ICON_SIZE_INVALID);
    gtk_image_set_pixel_size (GTK_IMAGE (image), 24);
    priv->toolbar_italic = GTK_WIDGET (gtk_tool_button_new (image, NULL));
    gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (priv->toolbar_italic), TRUE);
    gtk_widget_show_all (priv->toolbar_italic);
    gtk_toolbar_insert (GTK_TOOLBAR (priv->box), GTK_TOOL_ITEM (priv->toolbar_italic), -1);
    gtk_widget_set_tooltip_text (GTK_WIDGET (priv->toolbar_italic), _("Italic"));


    /* Strike */
    image = gtk_image_new_from_icon_name ("format-text-strikethrough-symbolic", GTK_ICON_SIZE_INVALID);
    gtk_image_set_pixel_size (GTK_IMAGE (image), 24);
    priv->toolbar_strike = GTK_WIDGET (gtk_tool_button_new (image, NULL));
    gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (priv->toolbar_strike), TRUE);
    gtk_widget_show_all (priv->toolbar_strike);
    gtk_toolbar_insert (GTK_TOOLBAR (priv->box), GTK_TOOL_ITEM (priv->toolbar_strike), -1);
    gtk_widget_set_tooltip_text (GTK_WIDGET (priv->toolbar_strike), _("Strike"));
  }


  /* Link */
  icons_path = (gchar*) bijiben_get_bijiben_dir ();
  full_path = g_build_filename (icons_path,
                                "bijiben",
                                "icons",
                                "hicolor",
                                "scalable",
                                "actions",
                                "link.svg",
                                NULL);

  pixbuf = gdk_pixbuf_new_from_file (full_path, &error);
  g_free (full_path);

  if (error)
    g_warning ("error loading link icon : %s",error->message);

  image = gtk_image_new_from_pixbuf (pixbuf);
  gtk_image_set_pixel_size (GTK_IMAGE (image), 24);
  priv->toolbar_link = GTK_WIDGET (gtk_tool_button_new (image, NULL));
  gtk_tool_button_set_use_underline (GTK_TOOL_BUTTON (priv->toolbar_link), TRUE);
  gtk_widget_show_all (priv->toolbar_link);
  gtk_toolbar_insert (GTK_TOOLBAR (priv->box), GTK_TOOL_ITEM (priv->toolbar_link), -1);
  gtk_widget_set_tooltip_text (GTK_WIDGET (priv->toolbar_link),
                               _("Copy selection to a new note"));


  priv->glued = FALSE;

  /* text selected --> fade in , and not selected --> fade out */
  view = biji_note_obj_get_editor (priv->note);

  g_signal_connect(view,"button-press-event",
                   G_CALLBACK(on_button_pressed),self);

  g_signal_connect(view,"button-release-event",
                   G_CALLBACK(on_button_released),self);

  g_signal_connect(view,"key-release-event",
                   G_CALLBACK(on_key_released),self);

  /* buttons */

  g_signal_connect (priv->toolbar_cut,"clicked",
                    G_CALLBACK(on_cut_clicked), self);

  g_signal_connect (priv->toolbar_copy,"clicked",
                    G_CALLBACK(on_copy_clicked), self);

  g_signal_connect (priv->toolbar_paste,"clicked",
                    G_CALLBACK(on_paste_clicked), self);

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


BjbEditorToolbar *
bjb_editor_toolbar_new (BjbNoteView    *bjb_note_view,
                        BijiNoteObj    *biji_note_obj)
{
  return g_object_new (BJB_TYPE_EDITOR_TOOLBAR,
                       "bjbnoteview" , bjb_note_view,
                       "note"        , biji_note_obj,
                       NULL);
}
