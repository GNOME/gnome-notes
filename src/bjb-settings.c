/* bjb-settings.c
 *
 * Copyright (C) Pierre-Yves LUYTEN 2011 <py@luyten.fr>
 * Copyright 2021 Mohammed Sadiq <sadiq@sadiqpk.org>
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
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
*/

#define G_LOG_DOMAIN "bjb-settings"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "bjb-settings.h"
#include "bjb-settings-dialog.h"


struct _BjbSettings
{
  GObject    parent_instance;

  /* Note Appearance settings */
  gboolean   use_system_font;
  char      *font;
  char      *color;

  /* Default Provider */
  char      *primary;

  /* org.gnome.desktop */
  GSettings *system;
  /* org.gnome.Notes */
  GSettings *app_settings;
};


enum {
  PROP_0,
  PROP_USE_SYSTEM_FONT,
  PROP_FONT,
  PROP_COLOR,
  PROP_PRIMARY,
  PROP_TEXT_SIZE,
  N_PROPS
};


static GParamSpec *properties[N_PROPS];

G_DEFINE_TYPE (BjbSettings, bjb_settings, G_TYPE_OBJECT)

static void
bjb_settings_finalize (GObject *object)
{
  BjbSettings *self = BJB_SETTINGS (object);

  g_object_unref (self->system);
  g_object_unref (self->app_settings);

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

    case PROP_TEXT_SIZE:
      g_value_take_string (value, g_settings_get_string (self->app_settings, "text-size"));
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
  GSettings *settings = self->app_settings;

  switch (prop_id)
    {
    case PROP_USE_SYSTEM_FONT:
      g_settings_set_boolean (settings, "use-system-font",
                              g_value_get_boolean (value));
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FONT]);
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

    case PROP_TEXT_SIZE:
      g_settings_set_string (settings, "text-size", g_value_get_string (value));
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FONT]);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
bjb_settings_class_init (BjbSettingsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bjb_settings_finalize;
  object_class->get_property = bjb_settings_get_property;
  object_class->set_property = bjb_settings_set_property;

  properties[PROP_USE_SYSTEM_FONT] =
    g_param_spec_boolean ("use-system-font",
                          "Use system font",
                          "Default System Font for Notes",
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_FONT] =
    g_param_spec_string ("font",
                         "Notes Font",
                         "Font for Notes",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_COLOR] =
    g_param_spec_string ("color",
                         "New Notes Color",
                         "Default Color for New Notes",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_PRIMARY] =
    g_param_spec_string ("default-location",
                         "Primary Location",
                         "Default Provider for New Notes",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  properties[PROP_TEXT_SIZE] =
    g_param_spec_string ("text-size",
                         "Text size",
                         "Text size for notes",
                         NULL,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, N_PROPS, properties);
}

static void
bjb_settings_init (BjbSettings *self)
{
  self->system = g_settings_new ("org.gnome.desktop.interface");
  self->app_settings = g_settings_new ("org.gnome.Notes");

  self->font = g_settings_get_string (self->app_settings, "font");
  self->color = g_settings_get_string (self->app_settings, "color");
  self->primary = g_settings_get_string (self->app_settings, "default-location");

  g_settings_bind (self->app_settings, "text-size",
                   self, "text-size",
                   G_SETTINGS_BIND_DEFAULT);
}

BjbSettings *
bjb_settings_new (void)
{
  return g_object_new (BJB_TYPE_SETTINGS, NULL);
}

BjbSettings *
bjb_settings_get_default (void)
{
  static BjbSettings *self;

  if (!self)
    g_set_weak_pointer (&self, g_object_new (BJB_TYPE_SETTINGS, NULL));

  return self;
}

/**
 * bjb_settings_get_font:
 * @self: A #BjbSettings
 *
 * Get the font that should be used for notes.
 * If :use-system-font is set, the name of system wide
 * document font is returned.
 *
 * Returns: (transfer full): The font string
 */
char *
bjb_settings_get_font (BjbSettings *self)
{
  g_return_val_if_fail (BJB_IS_SETTINGS (self), NULL);

  if (g_settings_get_boolean (self->app_settings, "use-system-font"))
    return g_settings_get_string (self->system, "document-font-name");

  return g_strdup (self->font);
}

/**
 * bjb_settings_get_custom_font:
 * @self: A #BjbSettings
 *
 * Get the custom font if set.  Use this function
 * if you want to get the custom font name
 * regardless of whether :use-system-font is set
 * or not.
 * Also see bjb_settings_get_font()
 *
 * Returns: (transfer none): The font string
 */
const char *
bjb_settings_get_custom_font (BjbSettings *self)
{
  g_return_val_if_fail (BJB_IS_SETTINGS (self), NULL);

  return self->font;
}

const char *
bjb_settings_get_default_color (BjbSettings *self)
{
  g_return_val_if_fail (BJB_IS_SETTINGS (self), NULL);

  return self->color;
}

const char *
bjb_settings_get_default_location (BjbSettings *self)
{
  g_return_val_if_fail (BJB_IS_SETTINGS (self), NULL);

  return self->primary;
}

BjbTextSizeType
bjb_settings_get_text_size (BjbSettings *self)
{
  g_return_val_if_fail (BJB_IS_SETTINGS (self), BJB_TEXT_SIZE_MEDIUM);

  return g_settings_get_enum (self->app_settings, "text-size");
}

GAction *
bjb_settings_get_text_size_gaction (BjbSettings *self)
{
  g_return_val_if_fail (BJB_IS_SETTINGS (self), NULL);

  return g_settings_create_action (self->app_settings, "text-size");

}

void
bjb_settings_set_last_opened_item (BjbSettings *self,
                                   const char  *note_path)
{
  g_return_if_fail (BJB_IS_SETTINGS (self));

  g_settings_set_string (self->app_settings, "last-opened-item", note_path);
}

char *
bjb_settings_get_last_opened_item (BjbSettings *self)
{
  g_return_val_if_fail (BJB_IS_SETTINGS (self), NULL);

  return g_settings_get_string (self->app_settings, "last-opened-item");
}

void
show_bijiben_settings_window (GtkWidget *parent_window)
{
  GtkDialog *dialog;

  dialog = bjb_settings_dialog_new ();
  gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent_window));

  gtk_window_present (GTK_WINDOW (dialog));
}
