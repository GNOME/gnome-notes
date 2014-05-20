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

#include "bjb-bijiben.h"
#include "bjb-color-button.h"
#include "bjb-settings.h"
#include "bjb-settings-dialog.h"


enum
{
  PROP_0,
  PROP_PARENT,
  N_PROPERTIES
};


struct BjbSettingsDialogPrivate_
{
  GtkWidget        *parent;
  BjbSettings      *settings;
  BijiManager     *manager;

  GtkStack         *stack;
  GtkStackSwitcher *switcher;

  /* Primary NoteBook page */

  GtkListBox       *box;
  GList            *children;


  /* Note Edition page */

  GtkWidget       *font_lbl;
  GtkWidget       *font_bt;
};




G_DEFINE_TYPE (BjbSettingsDialog, bjb_settings_dialog, GTK_TYPE_DIALOG)


static void
update_buttons (BjbSettingsDialog *self)
{
  BjbSettingsDialogPrivate *priv;
  gboolean use_system_font;

  priv = self->priv;
  use_system_font = bjb_settings_use_system_font (priv->settings);

  gtk_widget_set_sensitive (priv->font_lbl, !use_system_font);
  gtk_widget_set_sensitive (priv->font_bt, !use_system_font);
}


/* Callbacks */

static void
on_system_font_toggled (GtkSwitch         *button,
                        GParamSpec        *pspec,
                        BjbSettingsDialog *self)
{
  BjbSettings *settings;


  settings = self->priv->settings;
  bjb_settings_set_use_system_font (settings, gtk_switch_get_active (button));
  update_buttons (self);
}


static void
on_font_selected (GtkFontButton     *widget,
                  BjbSettings       *settings)
{
  g_settings_set_string (G_SETTINGS (settings),
                         "font",
                         gtk_font_button_get_font_name (widget));

}



static void
on_color_set (GtkColorButton *button,
              BjbSettings    *settings)
{
  GdkRGBA color;
  gchar *color_str;

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &color);
  color_str = gdk_rgba_to_string (&color);

  g_settings_set_string (G_SETTINGS (settings),
                         "color",
                         color_str);

  g_free (color_str);
}






/* Primary Provider page */

typedef struct
{
  GtkWidget      *overlay;
  GtkWidget      *widget;
  GtkWidget      *toggle;

  const gchar    *id;
  const char     *name;
  GtkWidget      *icon;

  gboolean       selected;

} ProviderChild;


static ProviderChild *
provider_child_new ()
{
  ProviderChild *retval;

  retval = g_slice_new (ProviderChild);
  retval->overlay = NULL;
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
child_toggle_new ()
{
  GtkWidget *w;

  w = gtk_image_new_from_icon_name ("object-select-symbolic", GTK_ICON_SIZE_INVALID);
  gtk_image_set_pixel_size (GTK_IMAGE (w), 48);
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
    gtk_overlay_add_overlay (GTK_OVERLAY (child->overlay), child->toggle);
  }


  else
  {
    if (child->toggle && GTK_IS_WIDGET (child->toggle))
      gtk_widget_destroy (child->toggle);
  }
}



static void
update_providers (BjbSettingsDialog *self)
{
  g_list_foreach (self->priv->children, toggle_child, self);
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

  g_object_set (bjb_app_get_settings (g_application_get_default ()),
                "default-location", child->id, NULL);


  /* Toggle everything : unselect all but this one */
  g_list_foreach (self->priv->children, unselect_child, NULL);
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
  BjbSettingsDialogPrivate    *priv;
  const BijiProviderInfo      *info;
  ProviderChild               *child;
  GtkWidget                   *box, *w;

  self = BJB_SETTINGS_DIALOG (user_data);
  priv = self->priv;
  info = (const BijiProviderInfo*) provider_info;


  child = provider_child_new ();
  child->id = info->unique_id;
  child->icon = info->icon;
  child->name = info->name;


  // what if (child->id != NULL && g_strcmp0 (child->id, "") != 0)


  /* Is the provider the primary ? */
  if (g_strcmp0 (child->id, bjb_settings_get_default_location (priv->settings)) ==0)
    child->selected = TRUE;

  /* Create the widget */

  child->widget = box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 12);
  child->overlay = gtk_overlay_new ();
  gtk_container_add (GTK_CONTAINER (priv->box), child->overlay);
  gtk_container_add (GTK_CONTAINER (child->overlay), box);


  g_object_set_qdata_full (G_OBJECT (child->overlay), application_quark (),
                           child, provider_child_free);

  w = child->icon;
  gtk_widget_set_margin_start (w, 12);
  gtk_container_add (GTK_CONTAINER (box), w);


  w = gtk_label_new (child->name);
  gtk_widget_set_margin_end (w, 180);
  gtk_container_add (GTK_CONTAINER (box), w);
  gtk_widget_set_valign (w, GTK_ALIGN_CENTER);
  gtk_box_pack_end (GTK_BOX (box), w, FALSE, FALSE, 0);

  self->priv->children = g_list_prepend (priv->children, child);
  gtk_widget_show_all (box);
}


GtkWidget *
create_page_primary (BjbSettingsDialog *self)
{
  BjbSettingsDialogPrivate *priv;
  GList *providers_info;

  priv = self->priv;
  priv->box = GTK_LIST_BOX (gtk_list_box_new ());

  /* Create the list */

  gtk_list_box_set_selection_mode (priv->box, GTK_SELECTION_NONE);
  gtk_list_box_set_header_func (priv->box, header_func, NULL, NULL);

  g_signal_connect (priv->box, "row-activated",
                    G_CALLBACK (on_row_activated_cb), self);


  /* Add providers */

  providers_info = biji_manager_get_providers (priv->manager);
  g_list_foreach (providers_info, add_child, self);
  g_list_free (providers_info);


  /* Check GSettings : toggle the actual default provider */
  update_providers (self);

  return GTK_WIDGET (priv->box);
}




/* Edition page (font, color) */


GtkWidget *
create_page_edition (BjbSettingsDialog *self)
{
  BjbSettingsDialogPrivate   *priv;
  BjbSettings                *settings;
  GtkWidget                  *grid, *label, *picker, *box;
  GdkRGBA                    color;

  priv = self->priv;
  settings = priv->settings;

  grid = gtk_grid_new ();
  gtk_grid_set_column_spacing (GTK_GRID (grid), 36);
  gtk_grid_set_row_spacing (GTK_GRID (grid), 6);
  gtk_widget_set_halign (grid, GTK_ALIGN_CENTER);



  /* Use System Font */
  label = gtk_label_new (_("Use System Font"));
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 1, 1, 1);
  picker = gtk_switch_new ();
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box), picker, FALSE, FALSE, 0);
  gtk_switch_set_active (GTK_SWITCH (picker),
                         bjb_settings_use_system_font (settings));
  g_signal_connect (picker, "notify::active",
                    G_CALLBACK (on_system_font_toggled), self);
  gtk_grid_attach (GTK_GRID (grid), box, 2, 1, 1, 1);


  /* Default font */
  label = priv->font_lbl = gtk_label_new (_("Note Font"));
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 2, 1, 1);

  picker = priv->font_bt = gtk_font_button_new_with_font (bjb_settings_get_default_font (settings));
  g_signal_connect (picker, "font-set",
                    G_CALLBACK (on_font_selected), settings);
  gtk_grid_attach (GTK_GRID (grid), picker, 2, 2, 1, 1);
  update_buttons (self);


  /* Default color */
  label = gtk_label_new (_("Default Color"));
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_grid_attach (GTK_GRID (grid), label, 1, 3, 1, 1);

  picker = bjb_color_button_new ();
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box), picker, FALSE, FALSE, 0);
  gdk_rgba_parse (&color, bjb_settings_get_default_color (settings));
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (picker), &color);
  g_signal_connect (picker, "color-set",
                    G_CALLBACK (on_color_set), settings); 
  gtk_grid_attach (GTK_GRID (grid), box, 2, 3, 1, 1);

  return grid;
}



/* Packaging */

static void
bjb_settings_dialog_constructed (GObject *object)
{
  BjbSettingsDialog          *self;
  GtkDialog                  *dialog; //also self
  BjbSettingsDialogPrivate   *priv;
  GtkWidget                  *area, *grid, *hbox, *page;
  GtkWindow                  *window;
  GApplication               *app;


  G_OBJECT_CLASS (bjb_settings_dialog_parent_class)->constructed (object);

  self = BJB_SETTINGS_DIALOG (object);
  dialog = GTK_DIALOG (self);
  window = GTK_WINDOW (self);
  priv = self->priv;


  app = g_application_get_default ();
  priv->manager = bijiben_get_manager (BIJIBEN_APPLICATION (app));
  priv->settings = bjb_app_get_settings (app);


  gtk_window_set_default_size (window, 500, 300);
  gtk_window_set_modal (window, TRUE);
  gtk_dialog_add_button (dialog, _("_Close"), GTK_RESPONSE_CLOSE);


  /* Dialog Area */
  area = gtk_dialog_get_content_area (dialog);
  gtk_container_set_border_width (GTK_CONTAINER (area), 38);

  priv->stack = GTK_STACK (gtk_stack_new ());
  gtk_widget_set_hexpand (GTK_WIDGET (priv->stack), TRUE);
  gtk_widget_set_vexpand (GTK_WIDGET (priv->stack), TRUE);

  priv->switcher = GTK_STACK_SWITCHER (gtk_stack_switcher_new ());
  hbox = gtk_grid_new ();
  gtk_widget_set_vexpand (hbox, TRUE);
  gtk_grid_attach (GTK_GRID (hbox), GTK_WIDGET (priv->switcher), 1, 1, 1, 1);
  gtk_widget_set_halign (hbox, GTK_ALIGN_CENTER);
  gtk_widget_set_valign (hbox, GTK_ALIGN_CENTER);
  gtk_stack_switcher_set_stack (priv->switcher, priv->stack);

  grid = gtk_grid_new ();
  gtk_container_add (GTK_CONTAINER (area),grid);
  gtk_grid_attach (GTK_GRID (grid), hbox, 1, 1, 1, 1);
  gtk_widget_set_valign (GTK_WIDGET (priv->switcher), GTK_ALIGN_START);
  gtk_grid_attach (GTK_GRID (grid), GTK_WIDGET (priv->stack), 1, 2, 1, 1);
  gtk_widget_set_valign (GTK_WIDGET (priv->stack), GTK_ALIGN_CENTER);


  /* Dialog Pages */
  page = create_page_edition (self);
  gtk_widget_set_vexpand (page, TRUE);
  gtk_stack_add_titled (priv->stack, page, "edition", _("Note Edition"));

  page = create_page_primary (self);
  gtk_stack_add_titled (priv->stack, page, "provider", _("Primary Book"));

  gtk_widget_show_all (area);
} 



static void
bjb_settings_dialog_finalize (GObject *object)
{
  BjbSettingsDialog *self;

  g_return_if_fail (BJB_IS_SETTINGS_DIALOG (object));

  self = BJB_SETTINGS_DIALOG (object);
  //g_list_free_full (self->priv->children, provider_child_free); uh?
  g_list_free (self->priv->children);

  G_OBJECT_CLASS (bjb_settings_dialog_parent_class)->finalize (object);
}


static void
bjb_settings_dialog_get_property (GObject    *object,
                                  guint       prop_id,
                                  GValue     *value,
                                  GParamSpec *pspec)
{
  BjbSettingsDialog *self = BJB_SETTINGS_DIALOG (object);

  switch (prop_id)
  {            
    case PROP_PARENT:
      g_value_set_object (value, self->priv->parent);
      break;
                                
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bjb_settings_dialog_set_property (GObject      *object,
                                  guint         prop_id,
                                  const GValue *value,
                                  GParamSpec   *pspec)
{
  BjbSettingsDialog *self = BJB_SETTINGS_DIALOG (object);

  switch (prop_id)
  {
    case PROP_PARENT:
      self->priv->parent = g_value_get_object (value);
      break;
            
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
bjb_settings_dialog_init (BjbSettingsDialog *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BJB_TYPE_SETTINGS_DIALOG, BjbSettingsDialogPrivate);
}



static void
bjb_settings_dialog_class_init (BjbSettingsDialogClass *klass)
{
  GObjectClass *g_object_class;

  g_object_class = G_OBJECT_CLASS (klass);

  g_object_class->set_property = bjb_settings_dialog_set_property;
  g_object_class->get_property = bjb_settings_dialog_get_property;
  g_object_class->finalize = bjb_settings_dialog_finalize;
  g_object_class->constructed = bjb_settings_dialog_constructed;

  g_type_class_add_private ((gpointer)klass, sizeof (BjbSettingsDialogPrivate));

  g_object_class_install_property (g_object_class,PROP_PARENT,
                                   g_param_spec_object ("parent",
                                                        "Parent Window",
                                                        "Parent Window Transient For",
                                                        GTK_TYPE_WINDOW,
                                                        G_PARAM_READWRITE | 
                                                        G_PARAM_STATIC_STRINGS));
}


GtkDialog *
bjb_settings_dialog_new (GtkWidget *parent_window)
{
  return g_object_new (BJB_TYPE_SETTINGS_DIALOG, "parent", parent_window, NULL);
}
