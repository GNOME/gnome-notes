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

// Bijiben probably wants something like 6 light colors
#define BJB_NUM_COLORS 4

static gchar *palette_str[BJB_NUM_COLORS] = {
  "rgb(239, 242, 209)", // eff2d1 from the mockup
  "rgb(210, 219, 230)", // d2dbe6 from the mockup
  "rgb(229, 230, 210)", //
  "rgb(235, 239, 244)", //
};

struct _BjbColorButtonPrivate
{
  GdkRGBA palette [BJB_NUM_COLORS];
  GtkWidget *dialog;
  gchar *title;
  GdkRGBA rgba;
};

#define BJB_COLOR_BUTTON_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BJB_TYPE_COLOR_BUTTON, BjbColorButtonPrivate))


G_DEFINE_TYPE (BjbColorButton, bjb_color_button, GTK_TYPE_COLOR_BUTTON);

static gboolean
dialog_destroy (GtkWidget *widget,
                gpointer   data)
{
  BjbColorButton *button = BJB_COLOR_BUTTON (data);

  button->priv->dialog = NULL;

  return FALSE;
}

static void
dialog_response (GtkDialog *dialog,
                 gint       response,
                 gpointer   data)
{
  BjbColorButton *button = BJB_COLOR_BUTTON (data);
  BjbColorButtonPrivate *priv = button->priv;

  if (response == GTK_RESPONSE_CANCEL)
    gtk_widget_hide (GTK_WIDGET (dialog));

  else if (response == GTK_RESPONSE_OK)
  {
      gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (dialog),
                                  &priv->rgba);

      gtk_widget_hide (GTK_WIDGET (dialog));
      gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (button), &priv->rgba);
      g_signal_emit_by_name (button, "color-set");
  }
}

static void
bjb_color_button_clicked (GtkButton *b)
{
  BjbColorButton *button = BJB_COLOR_BUTTON (b);
  BjbColorButtonPrivate *priv = button->priv;
  GtkWidget *dialog;
  gint i;

  /* if dialog already exists, make sure it's shown and raised */
  if (!button->priv->dialog)
    {
      /* Create the dialog and connects its buttons */
      GtkWidget *parent;

      parent = gtk_widget_get_toplevel (GTK_WIDGET (button));

      button->priv->dialog = dialog = gtk_color_chooser_dialog_new (button->priv->title, NULL);

      if (gtk_widget_is_toplevel (parent) && GTK_IS_WINDOW (parent))
        {
          if (GTK_WINDOW (parent) != gtk_window_get_transient_for (GTK_WINDOW (dialog)))
            gtk_window_set_transient_for (GTK_WINDOW (dialog), GTK_WINDOW (parent));

          gtk_window_set_modal (GTK_WINDOW (dialog),
                                gtk_window_get_modal (GTK_WINDOW (parent)));
        }

      for (i=0 ; i< BJB_NUM_COLORS ; i++)
      {
        GdkRGBA color;
  
        if (gdk_rgba_parse (&color, palette_str[i]))
          priv->palette [i] = color;
      }

      gtk_color_chooser_add_palette (GTK_COLOR_CHOOSER (dialog),
                                     GTK_ORIENTATION_HORIZONTAL,
                                     BJB_NUM_COLORS,
                                     BJB_NUM_COLORS,
                                     priv->palette);


      g_signal_connect (dialog, "response",
                        G_CALLBACK (dialog_response), button);
      g_signal_connect (dialog, "destroy",
                        G_CALLBACK (dialog_destroy), button);
    }

  gtk_color_chooser_get_rgba (GTK_COLOR_CHOOSER (button), &priv->rgba);
  gtk_color_chooser_set_rgba (GTK_COLOR_CHOOSER (button->priv->dialog),
                              &button->priv->rgba);

  gtk_window_present (GTK_WINDOW (button->priv->dialog));
}



static void
bjb_color_button_init (BjbColorButton *self)
{
  BjbColorButtonPrivate *priv = BJB_COLOR_BUTTON_GET_PRIVATE(self);
  self->priv = priv;

  priv->title = _("Note Color");
}

static void
bjb_color_button_constructed (GObject *obj)
{
  G_OBJECT_CLASS (bjb_color_button_parent_class)->constructed (obj);
}

static void
bjb_color_button_finalize (GObject *object)
{
  G_OBJECT_CLASS (bjb_color_button_parent_class)->finalize (object);
}

static void
bjb_color_button_class_init (BjbColorButtonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass); 
  GtkButtonClass *button_class = GTK_BUTTON_CLASS (klass);
    
  g_type_class_add_private (klass, sizeof (BjbColorButtonPrivate));

  object_class->constructed = bjb_color_button_constructed;
  object_class->finalize = bjb_color_button_finalize;

  /* Override std::gtk_color_button */
  button_class->clicked = bjb_color_button_clicked;
}

GtkWidget *
bjb_color_button_new (void)
{
  return g_object_new (BJB_TYPE_COLOR_BUTTON, NULL);
}
