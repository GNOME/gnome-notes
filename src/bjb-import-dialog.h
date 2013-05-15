/*
 * bjb-import-dialog.h
 * Copyright (C) Pierre-Yves LUYTEN 2013 <py@luyten.fr>
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
 * with this program.  If not, see <http://www.gnu.org/licenses/>.*/




#ifndef BJB_IMPORT_DIALOG_H_
#define BJB_IMPORT_DIALOG_H_ 1

#include <gtk/gtk.h>

G_BEGIN_DECLS


#define BJB_TYPE_IMPORT_DIALOG             (bjb_import_dialog_get_type ())
#define BJB_IMPORT_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BJB_TYPE_IMPORT_DIALOG, BjbImportDialog))
#define BJB_IMPORT_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BJB_TYPE_IMPORT_DIALOG, BjbImportDialogClass))
#define BJB_IS_IMPORT_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BJB_TYPE_IMPORT_DIALOG))
#define BJB_IS_IMPORT_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BJB_TYPE_IMPORT_DIALOG))
#define BJB_IMPORT_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BJB_TYPE_IMPORT_DIALOG, BjbImportDialogClass))

typedef struct BjbImportDialog_         BjbImportDialog;
typedef struct BjbImportDialogClass_    BjbImportDialogClass;
typedef struct BjbImportDialogPrivate_  BjbImportDialogPrivate;


struct BjbImportDialog_
{
  GtkDialog parent;
  BjbImportDialogPrivate *priv;
};


struct BjbImportDialogClass_
{
  GtkDialogClass parent_class;
};


GType         bjb_import_dialog_get_type     (void);


GtkDialog *   bjb_import_dialog_new          (GtkApplication *bijiben);


GList *       bjb_import_dialog_get_paths    (BjbImportDialog *dialog);


G_END_DECLS

#endif /* BJB_IMPORT_DIALOG_H_ */
