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

#ifndef BJB_EMPTY_RESULTS_BOX_H
#define BJB_EMPTY_RESULTS_BOX_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
  BJB_EMPTY_RESULTS_TYPE,
  BJB_EMPTY_RESULTS_NO_NOTE,
  BJB_EMPTY_RESULTS_NO_RESULTS
} BjbEmptyResultsBoxType;

#define BJB_TYPE_EMPTY_RESULTS_BOX (bjb_empty_results_box_get_type ())

#define BJB_EMPTY_RESULTS_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
   BJB_TYPE_EMPTY_RESULTS_BOX, BjbEmptyResultsBox))

#define BJB_EMPTY_RESULTS_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), \
   BJB_TYPE_EMPTY_RESULTS_BOX, BjbEmptyResultsBoxClass))

#define BJB_IS_EMPTY_RESULTS_BOX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
   BJB_TYPE_EMPTY_RESULTS_BOX))

#define BJB_IS_EMPTY_RESULTS_BOX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), \
   BJB_TYPE_EMPTY_RESULTS_BOX))

#define BJB_EMPTY_RESULTS_BOX_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS ((obj), \
   BJB_TYPE_EMPTY_RESULTS_BOX, BjbEmptyResultsBoxClass))

typedef struct _BjbEmptyResultsBox        BjbEmptyResultsBox;
typedef struct _BjbEmptyResultsBoxClass   BjbEmptyResultsBoxClass;
typedef struct _BjbEmptyResultsBoxPrivate BjbEmptyResultsBoxPrivate;

struct _BjbEmptyResultsBox
{
  GtkGrid parent_instance;
  BjbEmptyResultsBoxPrivate *priv;
};

struct _BjbEmptyResultsBoxClass
{
  GtkGridClass parent_class;
};

GType               bjb_empty_results_box_get_type           (void) G_GNUC_CONST;

GtkWidget          *bjb_empty_results_box_new                (void);

void                bjb_empty_results_box_set_type           (BjbEmptyResultsBox *self,
                                                              BjbEmptyResultsBoxType type);

G_END_DECLS

#endif /* BJB_EMPTY_RESULTS_BOX_H */
