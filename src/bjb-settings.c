/* bjb-settings.c
 * Copyright (C) Pierre-Yves LUYTEN 2011 <py@luyten.fr>
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.*/

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "bjb-bijiben.h"
#include "bjb-color-button.h"
#include "bjb-settings.h"

struct _BjbSettingsPrivate
{
  GSettings *settings ;
};

// Properties binded to gsettings.
enum
{
  PROP_0,

  // Note Editor.
  PROP_FONT,
  PROP_COLOR,

  N_PROPERTIES
};

#define BJB_SETTINGS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BJB_TYPE_SETTINGS, BjbSettingsPrivate))

G_DEFINE_TYPE (BjbSettings, bjb_settings, G_TYPE_OBJECT);

static void
bjb_settings_init (BjbSettings *object)
{    
  object->priv = 
  G_TYPE_INSTANCE_GET_PRIVATE(object,BJB_TYPE_SETTINGS,BjbSettingsPrivate);
}

static void
bjb_settings_finalize (GObject *object)
{
  G_OBJECT_CLASS (bjb_settings_parent_class)->finalize (object);
}

static void
bjb_settings_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BjbSettings *settings = BJB_SETTINGS (object);

  switch (prop_id)
  {            
    case PROP_FONT:
      g_value_set_string (value,settings->font);
      break;

    case PROP_COLOR:
      g_value_set_string (value,settings->color);
      break;
                                
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bjb_settings_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BjbSettings *settings = BJB_SETTINGS (object);

  switch (prop_id)
  {
    case PROP_FONT:
      settings->font = g_value_dup_string(value) ; 
      break;

    case PROP_COLOR:
      settings->color = g_value_dup_string(value);
      break;
            
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bjb_settings_class_init (BjbSettingsClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BjbSettingsPrivate));

  object_class->finalize = bjb_settings_finalize;
  object_class->get_property = bjb_settings_get_property;
  object_class->set_property = bjb_settings_set_property;
    
  g_object_class_install_property (object_class,PROP_FONT,
                                   g_param_spec_string("font",
                                                       "Notes Font",
                                                       "Font for Notes",
                                                       NULL,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (object_class,PROP_COLOR,
                                   g_param_spec_string("color",
                                                       "New Notes Color",
                                                       "Default Color for New Notes",
                                                       NULL,
                                                       G_PARAM_READWRITE | 
                                                       G_PARAM_STATIC_STRINGS));
}

// only init from bijiben

BjbSettings *
initialize_settings (void)
{
  BjbSettings *result = g_object_new (BJB_TYPE_SETTINGS,NULL) ;
  result->priv->settings= g_settings_new ("org.gnome.bijiben");

  // Note editor settings
  g_settings_bind  (result->priv->settings, "font",
                    result,"font",
                    G_SETTINGS_BIND_DEFAULT);

  g_settings_bind  (result->priv->settings, "color",
                    result,"color",
                    G_SETTINGS_BIND_DEFAULT);

  return result ;
}

static void
on_font_selected (GtkFontButton *widget,
                  BjbSettings *settings)
{
  g_settings_set_string (settings->priv->settings,
                         "font",
                         gtk_font_button_get_font_name (widget));
}

static void
on_color_set (GtkColorButton *button,
              BjbSettings *settings)
{
  GdkRGBA color;
  gchar *color_str;

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
  color_str = gdk_rgba_to_string (&color);

  g_settings_set_string (settings->priv->settings,
                         "color",
                         color_str);

  g_free (color_str);
}

void
show_bijiben_settings_window (GtkWidget *parent_window)
{
  GtkWidget *dialog,*area, *vbox, *hbox, *grid, *label, *picker;
  GdkRGBA color;
  gint width, height;

  GApplication *app = g_application_get_default();
  BjbSettings *settings = bjb_app_get_settings(app);

  /* create dialog */
  dialog = gtk_dialog_new_with_buttons (_("Preferences"),
                                        GTK_WINDOW(parent_window),
                                        GTK_DIALOG_MODAL| 
                                        GTK_DIALOG_DESTROY_WITH_PARENT,
                                        _("_Close"),
                                        GTK_RESPONSE_OK,
                                        NULL);

  gtk_window_get_size (GTK_WINDOW (parent_window), &width, &height);
  gtk_window_set_default_size (GTK_WINDOW (dialog),
                               width /2,
                               height /2);

  area = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
  gtk_container_set_border_width (GTK_CONTAINER (area), 0);
  gtk_widget_set_hexpand (area, TRUE);
  gtk_widget_set_vexpand (area, TRUE);

  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_widget_set_valign (vbox, GTK_ALIGN_CENTER);

  hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_widget_set_halign (hbox, GTK_ALIGN_CENTER);
  gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, FALSE, 0);

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 12);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_box_pack_start (GTK_BOX (hbox), grid, TRUE, FALSE, 0);

  /* Note Font */
  label = gtk_label_new (_("Note Font"));
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);
  picker = gtk_font_button_new_with_font (settings->font);
  g_signal_connect(picker,"font-set",
                   G_CALLBACK(on_font_selected),settings);
  gtk_grid_attach (GTK_GRID (grid), picker, 2, 1, 1, 1);

  /* Default Color */
  label = gtk_label_new (_("Default Color"));
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);
  picker = bjb_color_button_new ();
  gtk_grid_attach (GTK_GRID (grid), picker, 2, 2, 1, 1);
  gdk_rgba_parse (&color, settings->color );
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (picker), &color);
  g_signal_connect (picker, "color-set",
                    G_CALLBACK(on_color_set), settings);

  /* pack, show, run, kill */
  gtk_box_pack_start (GTK_BOX (area), vbox, TRUE, TRUE, 0);
  gtk_widget_show_all (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
