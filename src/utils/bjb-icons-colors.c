#include "bjb-bijiben.h"
#include "bjb-icons-colors.h"

GtkWidget *
get_icon (gchar *icon)
{
  GIcon *gi;
  GtkWidget *retval;

  gi = g_themed_icon_new_with_default_fallbacks (icon);
  retval = gtk_image_new_from_gicon (gi, GTK_ICON_SIZE_BUTTON);
  g_object_unref (gi);

  return retval;
}
