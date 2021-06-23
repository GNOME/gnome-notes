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

#include "biji-manager.h"
#include "bjb-application.h"
#include "bjb-color-button.h"
#include "bjb-settings.h"
#include "bjb-settings-dialog.h"

struct _BjbSettingsDialog
{
  GtkDialog         parent;

  GtkWidget        *parent_window;
  BjbSettings      *settings;
  BijiManager      *manager;

  GtkStack         *stack;

  /* Primary NoteBook page */
  GtkListBox       *listbox;

  /* Note Appearance page */
  GtkColorChooser *color_button;
  GtkFontButton   *font_button;
  GtkSwitch       *system_font_switch;
};

G_DEFINE_TYPE (BjbSettingsDialog, bjb_settings_dialog, GTK_TYPE_DIALOG)

/* Callbacks */

static void
on_font_selected (GtkFontButton     *widget,
                  BjbSettingsDialog *self)
{
  g_autofree gchar *font_name = NULL;

  font_name = gtk_font_chooser_get_font (GTK_FONT_CHOOSER (widget));

  g_object_set (self->settings, "font", font_name, NULL);

}

static void
on_color_set (GtkColorButton    *button,
              BjbSettingsDialog *self)
{
  GdkRGBA color;
  g_autofree gchar *color_str = NULL;

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
  color_str = gdk_rgba_to_string (&color);

  g_object_set (self->settings, "color", color_str, NULL);
}

static GtkWidget *
child_toggle_new (void)
{
  GtkWidget *w;

  w = gtk_image_new_from_icon_name ("object-select-symbolic", GTK_ICON_SIZE_INVALID);
  gtk_image_set_pixel_size (GTK_IMAGE (w), 16);
  gtk_widget_set_halign (w, GTK_ALIGN_END);
  gtk_widget_show (w);
  return w;
}

static void
toggle_child (GtkWidget *row,
              gboolean   active)
{
  GtkWidget *widget, *toggle;

  g_assert (GTK_IS_LIST_BOX_ROW (row));

  widget = gtk_bin_get_child (GTK_BIN (row));
  toggle = g_object_get_data (G_OBJECT (widget), "toggle");
  g_assert (toggle);

  gtk_widget_set_opacity (toggle, active ? 1.0 : 0.0);
}

static void
on_row_activated_cb    (GtkListBox    *list_box,
                        GtkListBoxRow *row,
                        gpointer       user_data)
{
  BjbSettingsDialog *self;
  BijiProvider *provider;
  GtkWidget *widget, *toggle;
  g_autoptr(GList) children = NULL;
  const BijiProviderInfo *info;

  self = BJB_SETTINGS_DIALOG (user_data);

  widget = gtk_bin_get_child (GTK_BIN (row));
  provider = g_object_get_data (G_OBJECT (widget), "provider");
  g_assert (provider);

  info = biji_provider_get_info (provider);
  toggle = g_object_get_data (G_OBJECT (widget), "toggle");
  g_assert (toggle);

  /* If the row is already selected, simply return */
  if (G_APPROX_VALUE (1.0, gtk_widget_get_opacity (toggle), DBL_EPSILON))
    return;

  children = gtk_container_get_children (GTK_CONTAINER (list_box));

  /* Deselect all rows first */
  for (GList *child = children; child; child = child->next)
    toggle_child (child->data, FALSE);

  /* Now select the current one */
  toggle_child (GTK_WIDGET (row), TRUE);

  g_object_set (self->settings, "default-location", info->unique_id, NULL);
  biji_manager_set_provider (self->manager, info->unique_id);
}


static void
header_func (GtkListBoxRow *row,
             GtkListBoxRow *before_row,
             gpointer user_data)
{
  if (before_row != NULL && gtk_list_box_row_get_header (row) != NULL)
    gtk_list_box_row_set_header (row, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

  else
    gtk_list_box_row_set_header (row, NULL);
}

static GtkWidget *
provider_row_new (BjbSettingsDialog *self,
                  BijiProvider      *provider)
{
  const BijiProviderInfo *info;
  GtkWidget *row, *w, *hbox;
  g_autofree char *identity = NULL;

  g_assert (BJB_IS_SETTINGS_DIALOG (self));
  g_assert (BIJI_IS_PROVIDER (provider));

  info = biji_provider_get_info (provider);

  if (info->user && info->domain)
    identity = g_strconcat (info->user, "@", info->domain, NULL);

  row = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 24);
  gtk_widget_set_margin_start (row, 12);
  gtk_widget_set_margin_end (row, 12);
  gtk_container_add (GTK_CONTAINER (row), info->icon);
  g_object_set_data (G_OBJECT (row), "provider", provider);

  hbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (row), hbox);

  w = gtk_label_new (info->name);
  gtk_widget_set_halign (w, GTK_ALIGN_START);
  gtk_widget_set_valign (w, GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand (w, TRUE);
  gtk_widget_set_vexpand (w, TRUE);
  gtk_container_add (GTK_CONTAINER (hbox), w);

  if (identity)
    {
      w = gtk_label_new (identity);
      gtk_widget_set_opacity (w, 0.5);
      gtk_widget_set_halign (w, GTK_ALIGN_START);
      gtk_widget_set_valign (w, GTK_ALIGN_CENTER);
      gtk_widget_set_hexpand (w, TRUE);
      gtk_container_add (GTK_CONTAINER (hbox), w);
    }

  w = child_toggle_new ();
  gtk_container_add (GTK_CONTAINER (row), w);
  g_object_set_data (G_OBJECT (row), "toggle", w);

  if (g_strcmp0 (info->unique_id, bjb_settings_get_default_location (self->settings)) == 0)
    gtk_widget_set_opacity (w, 1.0);
  else
    gtk_widget_set_opacity (w, 0.0);

  gtk_widget_show_all (row);

  return row;
}

static void
on_provider_items_changed_cb (BjbSettingsDialog *self)
{
  GListModel *providers;
  guint n_items;

  g_assert (BJB_IS_SETTINGS_DIALOG (self));

  providers = biji_manager_get_providers (self->manager);
  n_items = g_list_model_get_n_items (providers);

  /* First remove every row */
  gtk_container_foreach (GTK_CONTAINER (self->listbox),
                         (GtkCallback)gtk_widget_destroy, NULL);

  /* Now add a row for each provider */
  for (guint i = 0; i < n_items; i++)
    {
      g_autoptr(BijiProvider) provider = NULL;
      GtkWidget *child;

      provider = g_list_model_get_item (providers, i);
      child = provider_row_new (self, provider);
      gtk_container_add (GTK_CONTAINER (self->listbox), child);
    }
}

static void
bjb_settings_dialog_constructed (GObject *object)
{
  BjbSettingsDialog *self;
  GApplication      *app;
  GListModel *providers;
  GdkRGBA            color;

  G_OBJECT_CLASS (bjb_settings_dialog_parent_class)->constructed (object);

  self = BJB_SETTINGS_DIALOG (object);
  app = g_application_get_default ();
  self->manager = bijiben_get_manager (BJB_APPLICATION (app));
  self->settings = bjb_app_get_settings (app);

  gtk_list_box_set_selection_mode (self->listbox, GTK_SELECTION_NONE);
  gtk_list_box_set_header_func (self->listbox, header_func, NULL, NULL);

  g_object_bind_property (self->settings,
                          "use-system-font",
                          self->system_font_switch,
                          "active",
                          G_BINDING_BIDIRECTIONAL | G_BINDING_SYNC_CREATE);

  gtk_font_chooser_set_font (GTK_FONT_CHOOSER (self->font_button),
                             bjb_settings_get_custom_font (self->settings));

  gdk_rgba_parse (&color, bjb_settings_get_default_color (self->settings));
  gtk_color_chooser_set_rgba (self->color_button, &color);


  /* Add providers */
  providers = biji_manager_get_providers (self->manager);
  g_signal_connect_object (providers, "items-changed",
                           G_CALLBACK (on_provider_items_changed_cb),
                           self, G_CONNECT_SWAPPED);
  on_provider_items_changed_cb (self);
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
  gtk_widget_class_bind_template_child (gtk_widget_class, BjbSettingsDialog, listbox);
  gtk_widget_class_bind_template_child (gtk_widget_class, BjbSettingsDialog, stack);
  gtk_widget_class_bind_template_child (gtk_widget_class, BjbSettingsDialog, system_font_switch);

  gtk_widget_class_bind_template_callback (gtk_widget_class, on_color_set);
  gtk_widget_class_bind_template_callback (gtk_widget_class, on_font_selected);
  gtk_widget_class_bind_template_callback (gtk_widget_class, on_row_activated_cb);
}


GtkDialog *
bjb_settings_dialog_new (void)
{
  return g_object_new (BJB_TYPE_SETTINGS_DIALOG,
                       "use-header-bar", TRUE,
                       NULL);
}
