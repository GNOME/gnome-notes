/*
 * bjb_settings_dialog.h
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


#ifndef BJB_SETTINGS_DIALOG_H_
#define BJB_SETTINGS_DIALOG_H_ 1

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define BJB_TYPE_SETTINGS_DIALOG             (bjb_settings_dialog_get_type ())
#define BJB_SETTINGS_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BJB_TYPE_SETTINGS_DIALOG, BjbSettingsDialog))
#define BJB_SETTINGS_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BJB_TYPE_SETTINGS_DIALOG, BjbSettingsDialogClass))
#define BJB_IS_SETTINGS_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BJB_TYPE_SETTINGS_DIALOG))
#define BJB_IS_SETTINGS_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BJB_TYPE_SETTINGS_DIALOG))
#define BJB_SETTINGS_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BJB_TYPE_SETTINGS_DIALOG, BjbSettingsDialogClass))

typedef struct BjbSettingsDialog_         BjbSettingsDialog;
typedef struct BjbSettingsDialogClass_    BjbSettingsDialogClass;
typedef struct BjbSettingsDialogPrivate_  BjbSettingsDialogPrivate;

struct BjbSettingsDialog_
{
  GtkDialog parent;
  BjbSettingsDialogPrivate *priv;
};


struct BjbSettingsDialogClass_
{
  GtkDialogClass parent_class;
};


GType                  bjb_settings_dialog_get_type            (void);


GtkDialog             *bjb_settings_dialog_new                 (GtkWidget *window);


G_END_DECLS

#endif /* BJB_SETTINGS_DIALOG_H_ */
