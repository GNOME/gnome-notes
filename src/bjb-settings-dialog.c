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
  GList            *children;

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

/* Primary Provider page */

typedef struct
{
  GtkWidget      *widget;
  GtkWidget      *toggle;

  const gchar    *id;
  const char     *name;
  const char     *user;
  const char     *domain;
  GtkWidget      *icon;

  gboolean       selected;

} ProviderChild;


static ProviderChild *
provider_child_new (void)
{
  ProviderChild *retval;

  retval = g_slice_new (ProviderChild);
  retval->widget = NULL;
  retval->toggle = NULL;
  retval->selected = FALSE;
  retval->id = NULL;
  retval->icon = NULL;

  return retval;
}


static void
provider_child_free (gpointer child)
{
  g_slice_free (ProviderChild, child);
}


static inline GQuark
application_quark (void)
{
  static GQuark quark;

  if (G_UNLIKELY (quark == 0))
    quark = g_quark_from_static_string ("bjb-application");

  return quark;
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
toggle_child (gpointer iter,
              gpointer user_data)
{
  ProviderChild *child;

  child = (ProviderChild*) iter;

  if (child->selected)
    {
      child->toggle = child_toggle_new ();
      gtk_container_add (GTK_CONTAINER (child->widget), child->toggle);
    }
  else
    {
      if (child->toggle && GTK_IS_WIDGET (child->toggle))
        gtk_widget_destroy (child->toggle);

      child->toggle = NULL;
    }
}



static void
update_providers (BjbSettingsDialog *self)
{
  g_list_foreach (self->children, toggle_child, self);
}


static void
unselect_child (gpointer data, gpointer user_data)
{
  ProviderChild *child;

  child = (ProviderChild*) data;
  child->selected = FALSE;
}


static void
on_row_activated_cb    (GtkListBox    *list_box,
                        GtkListBoxRow *row,
                        gpointer       user_data)
{
  BjbSettingsDialog *self;
  GtkWidget     *widget;
  ProviderChild *child;


  self = BJB_SETTINGS_DIALOG (user_data);

  /* Write GSettings if the provider was not the primary one */
  widget = gtk_bin_get_child (GTK_BIN (row));
  child = g_object_get_qdata (G_OBJECT (widget), application_quark ());

  if (child->selected == TRUE)
    return;

  g_object_set (self->settings, "default-location", child->id, NULL);
  biji_manager_set_provider (self->manager, child->id);

  /* Toggle everything : unselect all but this one */
  g_list_foreach (self->children, unselect_child, NULL);
  child->selected = TRUE;
  update_providers (self);
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


static void
add_child (gpointer provider_info, gpointer user_data)
{
  BjbSettingsDialog           *self;
  const BijiProviderInfo      *info;
  ProviderChild               *child;
  GtkWidget                   *box, *w, *hbox;
  g_autofree char             *identity = NULL;

  self = BJB_SETTINGS_DIALOG (user_data);
  info = (const BijiProviderInfo*) provider_info;

  child = provider_child_new ();
  child->id = info->unique_id;
  child->icon = info->icon;
  child->name = info->name;
  child->user = info->user;
  child->domain = info->domain;

  if (child->user && info->domain)
    identity = g_strconcat(child->user, "@", child->domain, NULL);

  /* Is the provider the primary ? */
  if (g_strcmp0 (child->id, bjb_settings_get_default_location (self->settings)) ==0)
    child->selected = TRUE;

  /* Create the widget */

  child->widget = box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 24);
  gtk_widget_set_margin_start (box, 12);
  gtk_widget_set_margin_end (box, 12);
  gtk_container_add (GTK_CONTAINER (self->listbox), child->widget);


  g_object_set_qdata_full (G_OBJECT (child->widget), application_quark (),
                           child, provider_child_free);


  w = child->icon;
  gtk_container_add (GTK_CONTAINER (box), w);

  hbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
  gtk_container_add (GTK_CONTAINER (box), hbox);

  w = gtk_label_new (child->name);
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

  self->children = g_list_prepend (self->children, child);
  gtk_widget_show_all (box);
}

static void
bjb_settings_dialog_constructed (GObject *object)
{
  BjbSettingsDialog *self;
  GApplication      *app;
  GList             *providers;
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
                             bjb_settings_get_default_font (self->settings));

  gdk_rgba_parse (&color, bjb_settings_get_default_color (self->settings));
  gtk_color_chooser_set_rgba (self->color_button, &color);


  /* Add providers */
  providers = biji_manager_get_providers (self->manager);
  g_list_foreach (providers, add_child, self);
  g_list_free (providers);

  /* Check GSettings : toggle the actual default provider */
  update_providers (self);
}

static void
bjb_settings_dialog_finalize (GObject *object)
{
  BjbSettingsDialog *self;

  g_return_if_fail (BJB_IS_SETTINGS_DIALOG (object));

  self = BJB_SETTINGS_DIALOG (object);

  g_list_free (self->children);

  G_OBJECT_CLASS (bjb_settings_dialog_parent_class)->finalize (object);
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

  g_object_class->finalize = bjb_settings_dialog_finalize;
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
