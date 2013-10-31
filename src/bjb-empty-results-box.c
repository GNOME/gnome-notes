/*
 * Bijiben
 * Copyright © 2012, 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

/* Based on code from:
 *   + Documents
 *   + Photos
 */


#include "config.h"

#include <glib/gi18n.h>

#include "bjb-bijiben.h"
#include "bjb-empty-results-box.h"

G_DEFINE_TYPE (BjbEmptyResultsBox, bjb_empty_results_box, GTK_TYPE_GRID);

struct _BjbEmptyResultsBoxPrivate
{
  GtkWidget              *primary_label;
  GtkLabel               *details_label;
  BjbEmptyResultsBoxType  type;
};


static void
bjb_empty_results_box_constructed (GObject *object)
{
  BjbEmptyResultsBox *self;
  BjbEmptyResultsBoxPrivate *priv;
  GtkStyleContext *context;
  GdkPixbuf *pixbuf;
  GtkWidget *image, *labels_grid;
  gchar *label, *icons_path, *note_icon_path, *markup;
  GError *error;

  G_OBJECT_CLASS (bjb_empty_results_box_parent_class)->constructed (object);
  self = BJB_EMPTY_RESULTS_BOX (object);
  priv = self->priv;

  gtk_widget_set_halign (GTK_WIDGET (self), GTK_ALIGN_CENTER);
  gtk_widget_set_hexpand (GTK_WIDGET (self), TRUE);
  gtk_widget_set_valign (GTK_WIDGET (self), GTK_ALIGN_CENTER);
  gtk_widget_set_vexpand (GTK_WIDGET (self), TRUE);

  gtk_orientable_set_orientation (GTK_ORIENTABLE (self), GTK_ORIENTATION_HORIZONTAL);
  gtk_grid_set_column_spacing (GTK_GRID (self), 12);

  context = gtk_widget_get_style_context (GTK_WIDGET (self));
  gtk_style_context_add_class (context, "dim-label");

  icons_path = (gchar*) bijiben_get_bijiben_dir ();
  note_icon_path = g_build_filename (icons_path,
                                "bijiben",
                                "icons",
                                "hicolor",
                                "scalable",
                                "actions",
                                "note.svg",
                                NULL);

  error = NULL;
  pixbuf = gdk_pixbuf_new_from_file (note_icon_path, &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
  }

  image  = gtk_image_new_from_pixbuf (pixbuf);
  g_free (note_icon_path);

  gtk_image_set_pixel_size (GTK_IMAGE (image), 64);
  gtk_container_add (GTK_CONTAINER (self), image);

  labels_grid = gtk_grid_new ();
  gtk_orientable_set_orientation (GTK_ORIENTABLE (labels_grid), GTK_ORIENTATION_VERTICAL);
  gtk_grid_set_row_spacing (GTK_GRID (labels_grid), 12);
  gtk_container_add (GTK_CONTAINER (self), labels_grid);

  label = _("No Notes Found");
  markup = g_markup_printf_escaped ("<big><b>%s</b></big>", label);

  priv->primary_label = gtk_label_new (NULL);
  gtk_label_set_markup (GTK_LABEL (priv->primary_label), markup);
  g_free (markup);

  gtk_widget_set_halign (priv->primary_label, GTK_ALIGN_START);
  gtk_widget_set_vexpand (priv->primary_label, TRUE);
  gtk_label_set_use_markup (GTK_LABEL (priv->primary_label), TRUE);
  gtk_container_add (GTK_CONTAINER (labels_grid), priv->primary_label);


  self->priv->type = BJB_EMPTY_RESULTS_TYPE;
  label = "";
  self->priv->details_label = GTK_LABEL (gtk_label_new (label));
  gtk_label_set_use_markup (GTK_LABEL (self->priv->details_label), TRUE);
  gtk_widget_set_halign (priv->primary_label, GTK_ALIGN_START);
  // xalign: 0,
  // max_width_chars: 24,
  // wrap: true

  gtk_container_add (GTK_CONTAINER (labels_grid), GTK_WIDGET (self->priv->details_label));

  gtk_widget_set_valign (priv->primary_label, GTK_ALIGN_CENTER);
  gtk_widget_show_all (GTK_WIDGET (self));
}

void
bjb_empty_results_box_set_type (BjbEmptyResultsBox *self,
                                BjbEmptyResultsBoxType type)
{
  gchar *label;

  g_return_if_fail (BJB_IS_EMPTY_RESULTS_BOX (self));

  if (type == self->priv->type)
    return;

  switch (type)
  {
    case BJB_EMPTY_RESULTS_NO_NOTE:
      gtk_label_set_label (
        self->priv->details_label,
        _("Your notes notebook is empty.\nClick the New button to create your first note."));
      break;

    case BJB_EMPTY_RESULTS_NO_RESULTS:
      gtk_label_set_label (
        self->priv->details_label,
        _("No result found for this research."));
      break;


    /* 
     * Tracker is not installed, Bijiben cannot work,
     * do not try to workaround
     * TODO: PK
     */

    case BJB_EMPTY_RESULTS_TRACKER:
      label = _("Oops");
      gtk_label_set_label (
        GTK_LABEL (self->priv->primary_label), label);

      gtk_label_set_label (
        self->priv->details_label,
        _("Please install 'Tracker' then restart the application."));
      break;

    default:
      break;
  }

  self->priv->type = type;
}

static void
bjb_empty_results_box_init (BjbEmptyResultsBox *self)
{
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self,
                                      BJB_TYPE_EMPTY_RESULTS_BOX,
                                      BjbEmptyResultsBoxPrivate);
}


static void
bjb_empty_results_box_class_init (BjbEmptyResultsBoxClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BjbEmptyResultsBoxPrivate));

  object_class->constructed = bjb_empty_results_box_constructed;
}


GtkWidget *
bjb_empty_results_box_new (void)
{
  return g_object_new (BJB_TYPE_EMPTY_RESULTS_BOX, NULL);
}

