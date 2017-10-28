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

#ifndef BJB_EMPTY_RESULTS_BOX_H
#define BJB_EMPTY_RESULTS_BOX_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
  BJB_EMPTY_RESULTS_TYPE,
  BJB_EMPTY_RESULTS_NO_NOTE,
  BJB_EMPTY_RESULTS_NO_RESULTS,
  BJB_EMPTY_RESULTS_TRACKER
} BjbEmptyResultsBoxType;

#define BJB_TYPE_EMPTY_RESULTS_BOX (bjb_empty_results_box_get_type ())

G_DECLARE_FINAL_TYPE (BjbEmptyResultsBox, bjb_empty_results_box, BJB, EMPTY_RESULTS_BOX, GtkGrid)

GtkWidget          *bjb_empty_results_box_new                (void);

void                bjb_empty_results_box_set_type           (BjbEmptyResultsBox *self,
                                                              BjbEmptyResultsBoxType type);

G_END_DECLS

#endif /* BJB_EMPTY_RESULTS_BOX_H */
