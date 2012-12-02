/* bjb-color-button.h
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

#ifndef _BJB_COLOR_BUTTON_H_
#define _BJB_COLOR_BUTTON_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define BJB_TYPE_COLOR_BUTTON             (bjb_color_button_get_type ())
#define BJB_COLOR_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BJB_TYPE_COLOR_BUTTON, BjbColorButton))
#define BJB_COLOR_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BJB_TYPE_COLOR_BUTTON, BjbColorButtonClass))
#define BJB_IS_COLOR_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BJB_TYPE_COLOR_BUTTON))
#define BJB_IS_COLOR_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BJB_TYPE_COLOR_BUTTON))
#define BJB_COLOR_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BJB_TYPE_COLOR_BUTTON, BjbColorButtonClass))

typedef struct _BjbColorButtonClass BjbColorButtonClass;
typedef struct _BjbColorButton BjbColorButton;
typedef struct _BjbColorButtonPrivate BjbColorButtonPrivate;


struct _BjbColorButtonClass
{
  GtkColorButtonClass parent_class;
};

struct _BjbColorButton
{
  GtkColorButton parent_instance;
  BjbColorButtonPrivate *priv; 
};

GType bjb_color_button_get_type (void) G_GNUC_CONST;

GtkWidget * bjb_color_button_new (void);

G_END_DECLS

#endif /* _BJB_COLOR_BUTTON_H_ */
