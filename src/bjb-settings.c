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

#include "config.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "bjb-application.h"
#include "bjb-settings.h"
#include "bjb-settings-dialog.h"


struct _BjbSettings
{
  GSettings parent_instance;

  /* Note Appearance settings */
  gboolean use_system_font;
  gchar *font;
  gchar *color;

  /* Default Provider */
  gchar *primary;

  /* org.gnome.desktop */
  GSettings *system;
};


enum
{
  PROP_0,
  PROP_USE_SYSTEM_FONT,
  PROP_FONT,
  PROP_COLOR,
  PROP_PRIMARY,
  N_PROPERTIES
};


static GParamSpec *properties[N_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (BjbSettings, bjb_settings, G_TYPE_SETTINGS)

static void
bjb_settings_init (BjbSettings *self)
{
}

static void
bjb_settings_finalize (GObject *object)
{
  BjbSettings *self;

  self = BJB_SETTINGS (object);
  g_object_unref (self->system);

  g_free (self->font);
  g_free (self->color);
  g_free (self->primary);

  G_OBJECT_CLASS (bjb_settings_parent_class)->finalize (object);
}

static void
bjb_settings_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BjbSettings *self = BJB_SETTINGS (object);

  switch (prop_id)
  {
    case PROP_USE_SYSTEM_FONT:
      g_value_set_boolean (value, self->use_system_font);
      break;

    case PROP_FONT:
      g_value_set_string (value, self->font);
      break;

    case PROP_COLOR:
      g_value_set_string (value, self->color);
      break;

    case PROP_PRIMARY:
      g_value_set_string (value, self->primary);
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
  BjbSettings *self = BJB_SETTINGS (object);
  GSettings   *settings = G_SETTINGS (object);

  switch (prop_id)
  {
    case PROP_USE_SYSTEM_FONT:
      self->use_system_font = g_value_get_boolean (value);
      g_settings_set_boolean (settings, "use-system-font", self->use_system_font);
      break;

    case PROP_FONT:
      g_free (self->font);
      self->font = g_value_dup_string (value);
      g_settings_set_string (settings, "font", self->font);
      break;

    case PROP_COLOR:
      g_free (self->color);
      self->color = g_value_dup_string (value);
      g_settings_set_string (settings, "color", self->color);
      break;

    case PROP_PRIMARY:
      g_free (self->primary);
      self->primary = g_value_dup_string (value);
      g_settings_set_string (settings, "default-location", self->primary);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
bjb_settings_constructed (GObject *object)
{
  BjbSettings *self;
  GSettings   *settings;

  G_OBJECT_CLASS (bjb_settings_parent_class)->constructed (object);

  self = BJB_SETTINGS (object);
  settings = G_SETTINGS (object);
  self->system = g_settings_new ("org.gnome.desktop.interface");

  self->use_system_font = g_settings_get_boolean (settings, "use-system-font");
  self->font = g_settings_get_string (settings, "font");
  self->color = g_settings_get_string (settings, "color");
  self->primary = g_settings_get_string (settings, "default-location");
}


static void
bjb_settings_class_init (BjbSettingsClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->constructed = bjb_settings_constructed;
  object_class->finalize = bjb_settings_finalize;
  object_class->get_property = bjb_settings_get_property;
  object_class->set_property = bjb_settings_set_property;

  properties[PROP_USE_SYSTEM_FONT] = g_param_spec_boolean (
                                   "use-system-font",
                                   "Use system font",
                                   "Default System Font for Notes",
                                   TRUE,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_STRINGS);


  properties[PROP_FONT] = g_param_spec_string (
                                   "font",
                                   "Notes Font",
                                   "Font for Notes",
                                   NULL,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_STRINGS);


  properties[PROP_COLOR] = g_param_spec_string (
                                   "color",
                                   "New Notes Color",
                                   "Default Color for New Notes",
                                   NULL,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_STRINGS);


  properties[PROP_PRIMARY] = g_param_spec_string (
                                   "default-location",
                                   "Primary Location",
                                   "Default Provider for New Notes",
                                   NULL,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_STRINGS);


  g_object_class_install_properties (object_class, N_PROPERTIES, properties);
}




BjbSettings *
bjb_settings_new (void)
{
  return g_object_new (BJB_TYPE_SETTINGS, "schema-id", "org.gnome.Notes", NULL);
}


gboolean
bjb_settings_use_system_font            (BjbSettings *self)
{
  return self->use_system_font;
}


void
bjb_settings_set_use_system_font        (BjbSettings *self, gboolean value)
{
  self->use_system_font = value;
}


const gchar *
bjb_settings_get_default_font           (BjbSettings *self)
{
  return self->font;
}


const gchar *
bjb_settings_get_default_color          (BjbSettings *self)
{
  return self->color;
}


const gchar *
bjb_settings_get_default_location       (BjbSettings *self)
{
  return self->primary;
}


gchar *
bjb_settings_get_system_font            (BjbSettings *self)
{
  return g_settings_get_string (self->system,
                                "document-font-name");
}

BjbTextSizeType
bjb_settings_get_text_size              (BjbSettings *self)
{
  return g_settings_get_enum (G_SETTINGS (self), "text-size");
}

void
show_bijiben_settings_window (GtkWidget *parent_window)
{
  GtkDialog *dialog;

  dialog = bjb_settings_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent_window));

  /* result = */ gtk_dialog_run (dialog);
  gtk_widget_destroy (GTK_WIDGET (dialog));
}
