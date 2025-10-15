/*
 * bjb_settings_dialog.c
 *
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

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "bjb-settings.h"
#include "bjb-settings-dialog.h"

struct _BjbSettingsDialog
{
  GtkDialog         parent;

  GtkWidget        *parent_window;
  BjbSettings      *settings;

  GtkStack         *stack;

  /* Note Appearance page */
  GtkColorChooser *color_button;
  GtkFontButton   *font_button;
  GtkSwitch       *system_font_switch;
};

G_DEFINE_TYPE (BjbSettingsDialog, bjb_settings_dialog, GTK_TYPE_DIALOG)

/* Callbacks */

static void
bjb_settings_dialog_font_set_cb (BjbSettingsDialog   *self,
                                 GParamSpec          *pspec,
                                 GtkFontDialogButton *button)
{
  g_autofree char *font_name = NULL;
  PangoFontDescription *font;

  g_assert (BJB_IS_SETTINGS_DIALOG (self));
  g_assert (GTK_IS_FONT_DIALOG_BUTTON (button));

  font = gtk_font_dialog_button_get_font_desc (button);
  font_name = pango_font_description_to_string (font);

  g_object_set (self->settings, "font", font_name, NULL);

}

static void
bjb_settings_dialog_color_set_cb (BjbSettingsDialog    *self,
                                  GParamSpec           *pspec,
                                  GtkColorDialogButton *button)
{
  g_autofree char *color_str = NULL;
  const GdkRGBA *color;

  g_assert (BJB_IS_SETTINGS_DIALOG (self));
  g_assert (GTK_IS_COLOR_DIALOG_BUTTON (button));

  color = gtk_color_dialog_button_get_rgba (button);
  color_str = gdk_rgba_to_string (color);

  g_object_set (self->settings, "color", color_str, NULL);
}

static void
bjb_settings_dialog_constructed (GObject *object)
{
  BjbSettingsDialog *self;
  GdkRGBA            color;
  g_autoptr(PangoFontDescription) font = NULL;

  G_OBJECT_CLASS (bjb_settings_dialog_parent_class)->constructed (object);

  self = BJB_SETTINGS_DIALOG (object);
  self->settings = bjb_settings_get_default();

  g_object_bind_property (self->settings,
                          "use-system-font",
                          self->system_font_switch,
                          "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  font = pango_font_description_from_string (bjb_settings_get_custom_font (self->settings));
  gtk_font_dialog_button_set_font_desc (GTK_FONT_DIALOG_BUTTON (self->font_button), font);

  gdk_rgba_parse (&color, bjb_settings_get_default_color (self->settings));
  gtk_color_dialog_button_set_rgba (GTK_COLOR_DIALOG_BUTTON (self->color_button), &color);
}

static void
bjb_settings_dialog_init (BjbSettingsDialog *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bjb_settings_dialog_class_init (BjbSettingsDialogClass *klass)
{
  GtkWidgetClass *gtk_widget_class;
  GObjectClass   *g_object_class;

  g_object_class = G_OBJECT_CLASS (klass);
  gtk_widget_class = GTK_WIDGET_CLASS (klass);

  g_object_class->constructed = bjb_settings_dialog_constructed;

  gtk_widget_class_set_template_from_resource (gtk_widget_class, "/org/gnome/Notes/ui/settings-dialog.ui");

  gtk_widget_class_bind_template_child (gtk_widget_class, BjbSettingsDialog, color_button);
  gtk_widget_class_bind_template_child (gtk_widget_class, BjbSettingsDialog, font_button);
  gtk_widget_class_bind_template_child (gtk_widget_class, BjbSettingsDialog, stack);
  gtk_widget_class_bind_template_child (gtk_widget_class, BjbSettingsDialog, system_font_switch);

  gtk_widget_class_bind_template_callback (gtk_widget_class, bjb_settings_dialog_color_set_cb);
  gtk_widget_class_bind_template_callback (gtk_widget_class, bjb_settings_dialog_font_set_cb);
}


GtkDialog *
bjb_settings_dialog_new (void)
{
  return g_object_new (BJB_TYPE_SETTINGS_DIALOG,
                       "use-header-bar", TRUE,
                       NULL);
}
