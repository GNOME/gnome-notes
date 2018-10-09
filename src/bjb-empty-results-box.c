/*
 * Bijiben
 * Copyright © 2012, 2013 Red Hat, Inc.
 * Copyright 2017 Mohammed Sadiq <sadiq@sadiqpk.org>
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

#include "bjb-empty-results-box.h"


struct _BjbEmptyResultsBox
{
  GtkGrid                 parent_instance;

  GtkWidget              *primary_label;
  GtkWidget              *image;
  GtkLabel               *details_label;
  BjbEmptyResultsBoxType  type;
};

G_DEFINE_TYPE (BjbEmptyResultsBox, bjb_empty_results_box, GTK_TYPE_GRID);

void
bjb_empty_results_box_set_type (BjbEmptyResultsBox *self,
                                BjbEmptyResultsBoxType type)
{
  gchar *label;

  g_return_if_fail (BJB_IS_EMPTY_RESULTS_BOX (self));

  if (type == self->type)
    return;

  switch (type)
  {
    case BJB_EMPTY_RESULTS_NO_NOTE:
      gtk_label_set_label (
        self->details_label,
        _("Press the New button to create a note."));

      gtk_widget_show (GTK_WIDGET (self->details_label));
      gtk_widget_show (self->image);
      break;

    case BJB_EMPTY_RESULTS_NO_RESULTS:
      gtk_label_set_label (
        self->details_label, NULL);

      gtk_widget_hide (GTK_WIDGET (self->details_label));
      gtk_widget_hide (self->image);
      break;


    /*
     * Tracker is not installed, Bijiben cannot work,
     * do not try to workaround
     * TODO: PK
     */

    case BJB_EMPTY_RESULTS_TRACKER:
      label = _("Oops");
      gtk_label_set_label (
        GTK_LABEL (self->primary_label), label);

      gtk_label_set_label (
        self->details_label,
        _("Please install “Tracker” then restart the application."));

      gtk_widget_show (GTK_WIDGET (self->details_label));
      gtk_widget_show (self->image);
      break;

    case BJB_EMPTY_RESULTS_TYPE:
    default:
      break;
  }

  self->type = type;
}

static void
bjb_empty_results_box_init (BjbEmptyResultsBox *self)
{
  gtk_icon_theme_add_resource_path (gtk_icon_theme_get_default (),
                                    "/org/gnome/Notes/icons");

  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bjb_empty_results_box_class_init (BjbEmptyResultsBoxClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/empty-results-box.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbEmptyResultsBox, primary_label);
  gtk_widget_class_bind_template_child (widget_class, BjbEmptyResultsBox, details_label);
  gtk_widget_class_bind_template_child (widget_class, BjbEmptyResultsBox, image);
}


GtkWidget *
bjb_empty_results_box_new (void)
{
  return g_object_new (BJB_TYPE_EMPTY_RESULTS_BOX, NULL);
}
