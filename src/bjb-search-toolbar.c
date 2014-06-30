/* bjb-search-toolbar.c
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

/*
 * The SearchToolbar displays an entry when text is inserted.
 * 
 * BjbController is updated accordingly and makes note to be
 * displayed or not.
 */

#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgd/gd-entry-focus-hack.h>
#include <libgd/gd-tagged-entry.h>

#include "bjb-controller.h"
#include "bjb-main-toolbar.h"
#include "bjb-main-view.h"
#include "bjb-search-toolbar.h"
#include "bjb-window-base.h"

enum
{
  PROP_0,
  PROP_WINDOW,
  PROP_CONTROLLER,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

struct _BjbSearchToolbarPrivate
{
  GdTaggedEntry     *entry;
  gchar             *needle;
  GtkEntryBuffer    *entry_buf;
  BjbController     *controller;

  /* Signals */
  gulong            key_pressed;
  gulong            deleted;
  gulong            inserted;


  GtkWidget         *window;
};

G_DEFINE_TYPE (BjbSearchToolbar, bjb_search_toolbar, GTK_TYPE_SEARCH_BAR);


static void
bjb_search_toolbar_toggle_search_button (BjbSearchToolbar *self,
                                         gboolean state)
{
  bjb_window_base_toggle_search_button (BJB_WINDOW_BASE (self->priv->window),
                                        state);
}



void
bjb_search_toolbar_fade_in (BjbSearchToolbar *self)
{
  if (gtk_search_bar_get_search_mode (GTK_SEARCH_BAR (self)) == TRUE)
    return;

  gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (self), TRUE);
  bjb_search_toolbar_toggle_search_button (self, TRUE);
}



void
bjb_search_toolbar_fade_out (BjbSearchToolbar *self)
{
  if (gtk_search_bar_get_search_mode (GTK_SEARCH_BAR (self)) == FALSE)
    return;

  /* clear the search before hiding */
  gtk_entry_set_text (GTK_ENTRY (self->priv->entry), "");
  bjb_controller_set_needle (self->priv->controller, "");


  gtk_search_bar_set_search_mode (GTK_SEARCH_BAR (self), FALSE);
  bjb_search_toolbar_toggle_search_button (self, FALSE);
}



static gboolean
on_key_pressed (GtkWidget *widget,GdkEvent  *event,gpointer user_data)
{
  BjbSearchToolbar *self;
  GdkModifierType modifiers;

  self = BJB_SEARCH_TOOLBAR (user_data);
  modifiers = gtk_accelerator_get_default_mod_mask ();


  if ((event->key.state & modifiers) == GDK_CONTROL_MASK)
    return FALSE;

  /* Reveal the entry is text is input */
  if (gtk_search_bar_get_search_mode (GTK_SEARCH_BAR (self)) == FALSE)
  {
    switch (event->key.keyval)
    {
      case GDK_KEY_Control_L :
      case GDK_KEY_Control_R :
      case GDK_KEY_Shift_L :
      case GDK_KEY_Shift_R :
      case GDK_KEY_Alt_L :
      case GDK_KEY_Alt_R :
      case GDK_KEY_Tab :
      case GDK_KEY_space :
      case GDK_KEY_BackSpace :
      case GDK_KEY_Left :
      case GDK_KEY_Right :
      case GDK_KEY_Up :
      case GDK_KEY_Down :
      case GDK_KEY_Return :
        return FALSE;

      /* err, we still return false to get the key for search... */
      default:
        if (event->key.keyval != GDK_KEY_Escape)
          bjb_search_toolbar_fade_in (self);
        return FALSE;
    }
  }

  /* If there is already an entry and escape pressed, hide entry
   * Maybe should we use gtk_widget_has_focus (widget) */
  else if (event->key.keyval == GDK_KEY_Escape)
  {
    bjb_search_toolbar_fade_out (self);
    return TRUE;
  }

  return FALSE;
}


static void
bjb_search_toolbar_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BjbSearchToolbar *self = BJB_SEARCH_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_WINDOW:
      g_value_set_object (value, self->priv->window);
      break;
    case PROP_CONTROLLER:
      g_value_set_object(value, self->priv->controller);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}


static void
bjb_search_toolbar_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BjbSearchToolbar *self = BJB_SEARCH_TOOLBAR (object);

  switch (property_id)
  {
    case PROP_WINDOW:
      self->priv->window = g_value_get_object (value);
      break;
    case PROP_CONTROLLER:
      self->priv->controller = g_value_get_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
  }
}




static void
action_search_entry (GtkEntry *entry, BjbController *controller)
{
  bjb_controller_set_needle (controller, gtk_entry_get_text (entry));
}



static void
action_entry_insert_callback (GtkEntryBuffer *buffer,
                              guint position,
                              gchar *chars,
                              guint n_chars,
                              BjbSearchToolbar *self)
{
  action_search_entry (GTK_ENTRY (self->priv->entry),
                       self->priv->controller);
}




static void
action_entry_delete_callback (GtkEntryBuffer *buffer,
                              guint position,
                              guint n_chars,
                              BjbSearchToolbar *self)
{ 
  action_search_entry (GTK_ENTRY (self->priv->entry),
                       self->priv->controller);
}


void
bjb_search_toolbar_disconnect (BjbSearchToolbar *self)
{
  BjbSearchToolbarPrivate *priv = self->priv ;


  g_signal_handler_disconnect (priv->window,priv->key_pressed);
  g_signal_handler_disconnect (priv->entry_buf, priv->inserted);
  g_signal_handler_disconnect (priv->entry_buf, priv->deleted);


  priv->key_pressed = 0;
  priv->inserted = 0;
  priv->deleted = 0;

}


static void
bjb_search_toolbar_finalize (GObject *obj)
{
  G_OBJECT_CLASS (bjb_search_toolbar_parent_class)->finalize (obj);
}

void
bjb_search_toolbar_connect (BjbSearchToolbar *self)
{
  BjbSearchToolbarPrivate *priv = self->priv ;

  /* Connect to set the text */
  if (priv->key_pressed == 0)
    priv->key_pressed = g_signal_connect(priv->window,"key-press-event",
                                         G_CALLBACK(on_key_pressed),self);


  if (priv->inserted ==0)
    priv->inserted = g_signal_connect (priv->entry_buf, "inserted-text",
                        G_CALLBACK (action_entry_insert_callback), self);

  if (priv->deleted ==0)
    priv->deleted = g_signal_connect (priv->entry_buf, "deleted-text",
                        G_CALLBACK (action_entry_delete_callback), self);
}

static void
bjb_search_toolbar_constructed (GObject *obj)
{
  BjbSearchToolbar        *self = BJB_SEARCH_TOOLBAR(obj);
  BjbSearchToolbarPrivate *priv = self->priv ;

  G_OBJECT_CLASS (bjb_search_toolbar_parent_class)->constructed (obj);

  /* Get the needle from controller */
  priv->needle = bjb_controller_get_needle (priv->controller);
  priv->entry_buf = gtk_entry_get_buffer (GTK_ENTRY (priv->entry));

  if (priv->needle && g_strcmp0 (priv->needle, "") != 0)
  { 
    gtk_entry_set_text (GTK_ENTRY (priv->entry), priv->needle);
    bjb_search_toolbar_fade_in (self);
    gtk_editable_set_position (GTK_EDITABLE (self->priv->entry), -1);
  }
}


static void
bjb_search_toolbar_init (BjbSearchToolbar *self)
{
  BjbSearchToolbarPrivate    *priv;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BJB_TYPE_SEARCH_TOOLBAR, BjbSearchToolbarPrivate);
  priv = self->priv;

  priv->entry = gd_tagged_entry_new ();
  g_object_set (priv->entry, "width_request", 500, NULL);
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (priv->entry));
  gtk_widget_show (GTK_WIDGET (priv->entry));
}


static void
bjb_search_toolbar_class_init (BjbSearchToolbarClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);

  object_class->get_property = bjb_search_toolbar_get_property ;
  object_class->set_property = bjb_search_toolbar_set_property ;
  object_class->constructed = bjb_search_toolbar_constructed ;
  object_class->finalize = bjb_search_toolbar_finalize ;

  properties[PROP_WINDOW] = g_param_spec_object ("window",
                                                 "Window",
                                                 "Window",
                                                 GTK_TYPE_WIDGET,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT |
                                                 G_PARAM_STATIC_STRINGS);

  properties[PROP_CONTROLLER] = g_param_spec_object ("controller",
                                                     "Controller",
                                                     "Controller",
                                                     BJB_TYPE_CONTROLLER,
                                                     G_PARAM_READWRITE |
                                                     G_PARAM_CONSTRUCT |
                                                     G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);
  g_type_class_add_private (class, sizeof (BjbSearchToolbarPrivate));
}


BjbSearchToolbar *
bjb_search_toolbar_new (GtkWidget     *window,
                        BjbController *controller)
{
  return g_object_new (BJB_TYPE_SEARCH_TOOLBAR,
                       "window",      window,
                       "controller",  controller,
                       NULL);
}

