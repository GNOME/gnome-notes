/* bjb-settings.h
 * Copyright © 2012, 2013 Pierre-Yves LUYTEN <py@luyten.fr>
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

#ifndef _BJB_SETTINGS_H_
#define _BJB_SETTINGS_H_

#include <glib-object.h>



G_BEGIN_DECLS

#define BJB_TYPE_SETTINGS (bjb_settings_get_type ())

G_DECLARE_FINAL_TYPE (BjbSettings, bjb_settings, BJB, SETTINGS, GSettings)


BjbSettings      *bjb_settings_new                        (void);


gboolean          bjb_settings_use_system_font            (BjbSettings *self);


void              bjb_settings_set_use_system_font        (BjbSettings *self,
                                                           gboolean value);


const gchar      *bjb_settings_get_default_font           (BjbSettings *self);


const gchar      *bjb_settings_get_default_color          (BjbSettings *self);


const gchar      *bjb_settings_get_default_location       (BjbSettings *self);


gchar            *bjb_settings_get_system_font            (BjbSettings *self);


void              show_bijiben_settings_window            (GtkWidget *parent_window);

G_END_DECLS

#endif /* _BJB_SETTINGS_H_ */
