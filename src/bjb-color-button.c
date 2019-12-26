/* bjb-color-button.c
 * Copyright (C) Pierre-Yves Luyten 2012 <py@luyten.fr>
 *
 * bijiben is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * anjuta is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/* This is mostly gtk-color-button.c
 * Just overrides the dialog to choose specific palette
 * But a dedicated dialog might be better */

#include <glib/gi18n.h>

#include "bjb-color-button.h"

// GIMP Palette
#define BJB_NUM_COLORS 8

static const char *palette_str[BJB_NUM_COLORS] = {
  "rgb(153, 193, 241)", // Blue 1
  "rgb(143, 240, 164)", // Green 1
  "rgb(249, 240, 107)", // Yellow 1
  "rgb(255, 190, 111)", // Orange 1
  "rgb(246,  97,  81)", // Red 1
  "rgb(220, 138, 221)", // Purple 1
  "rgb(205, 171, 143)", // Brown 1
  "rgb(255, 255, 255)", // Light 1
};

struct _BjbColorButton
{
  GtkColorButton parent_instance;

  GdkRGBA palette [BJB_NUM_COLORS];
  GtkWidget *dialog;
  gchar *title;
  GdkRGBA rgba;
};

G_DEFINE_TYPE (BjbColorButton, bjb_color_button, GTK_TYPE_COLOR_BUTTON);

static gboolean
dialog_destroy (GtkWidget *widget,
                gpointer   data)
{
  BjbColorButton *self = BJB_COLOR_BUTTON (data);

  self->dialog = NULL;

  return FALSE;
}

static void
dialog_response (GtkDialog *dialog,
                 gint       response,
                 gpointer   data)
{
  BjbColorButton *self = BJB_COLOR_BUTTON (data);

  if (response == GTK_RESPONSE_CANCEL)
    gtk_widget_hide (GTK_WIDGET (dialog));

  else if (response == GTK_RESPONSE_OK)
  {
      gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (dialog),
                                  &self->rgba);

      gtk_widget_hide (GTK_WIDGET (dialog));
      gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (self), &self->rgba);
      g_signal_emit_by_name (self, "color-set");
  }
}

static void
bjb_color_button_clicked (GtkButton *b)
{
  BjbColorButton *self = BJB_COLOR_BUTTON (b);
  GtkWidget *dialog;
  gint i;

  /* if dialog already exists, make sure it's shown and raised */
  if (!self->dialog)
    {
      /* Create the dialog and connects its buttons */
      GtkWidget *parent;

      parent = gtk_widget_get_toplevel (GTK_WIDGET (self));

      self->dialog = dialog = gtk_color_chooser_dialog_new (self->title, NULL);

      if (gtk_widget_is_toplevel (parent) && GTK_IS_WINDOW (parent))
        {
          if (GTK_WINDOW (parent) != gtk_window_get_transient_for (GTK_WINDOW (dialog)))
            gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));

          gtk_window_set_modal (GTK_WINDOW (dialog), TRUE);
        }

      for (i=0 ; i< BJB_NUM_COLORS ; i++)
      {
        GdkRGBA color;

        if (gdk_rgba_parse (&color, palette_str[i]))
          self->palette [i] = color;
      }

      gtk_color_chooser_add_palette (GTK_COLOR_CHOOSER (dialog),
                                     GTK_ORIENTATION_HORIZONTAL,
                                     BJB_NUM_COLORS,
                                     BJB_NUM_COLORS,
                                     self->palette);

      g_signal_connect (dialog, "response",
                        G_CALLBACK (dialog_response), self);
      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (dialog_destroy), self);
    }

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (self), &self->rgba);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (self->dialog),
                              &self->rgba);

  gtk_window_present (GTK_WINDOW (self->dialog));
}

static void
bjb_color_button_init (BjbColorButton *self)
{
  self->title = _("Note Color");
}

static void
bjb_color_button_class_init (BjbColorButtonClass *klass)
{
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);

  /* Override std::gtk_color_button */
  button_class->clicked = bjb_color_button_clicked;
}

GtkWidget *
bjb_color_button_new (void)
{
  return g_object_new (BJB_TYPE_COLOR_BUTTON, NULL);
}
