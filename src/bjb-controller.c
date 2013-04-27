/*
 * bjb-controller.c
 * Copyright (C) Pierre-Yves Luyten 2012, 2013 <py@luyten.fr>
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
 * bjb-controler is window-wide.
 * mainly useful for BjbMainView,
 * it controls the window behaviour.
 */

#include <libgd/gd.h>

#include "bjb-controller.h"
#include "bjb-main-view.h"
#include "bjb-window-base.h"

/* Gobject */

struct _BjbControllerPrivate
{
  BijiNoteBook  *book ;
  gchar         *needle ;
  GtkTreeModel  *model ;
  GtkTreeModel  *completion;

  BjbWindowBase *window;

  /*  Private  */
  GList          *items_to_show;
  gboolean       connected;
  gulong         book_change;
};

enum {
  PROP_0,
  PROP_BOOK,
  PROP_WINDOW,
  PROP_NEEDLE,
  PROP_MODEL,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };

/* Currently used by toolbars */
enum {
  SEARCH_CHANGED,
  DISPLAY_NOTES_CHANGED, // either search or book change
  BJB_CONTROLLER_SIGNALS
};

static guint bjb_controller_signals [BJB_CONTROLLER_SIGNALS] = { 0 };

#define BJB_CONTROLLER_GET_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BJB_TYPE_CONTROLLER, BjbControllerPrivate))

G_DEFINE_TYPE (BjbController, bjb_controller, G_TYPE_OBJECT);

/* GObject */

static void
bjb_controller_init (BjbController *self)
{
  BjbControllerPrivate *priv  ;
  GtkListStore     *store ;
  GtkListStore     *completion ;

  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, 
                                            BJB_TYPE_CONTROLLER, 
                                            BjbControllerPrivate);
  priv = self->priv ;

  /* Create the columns */
  store = gtk_list_store_new (GD_MAIN_COLUMN_LAST,
                              G_TYPE_STRING,   // urn
                              G_TYPE_STRING,   // uri
                              G_TYPE_STRING,   // name
                              G_TYPE_STRING,   // author
                              GDK_TYPE_PIXBUF,   // icon then note
                              G_TYPE_LONG, // mtime
                              G_TYPE_BOOLEAN);   // state

  priv->model = GTK_TREE_MODEL(store) ;
  priv->items_to_show = NULL;
  priv->needle = NULL;
  priv->connected = FALSE;

  completion  = gtk_list_store_new (1, G_TYPE_STRING);
  priv->completion = GTK_TREE_MODEL (completion);
}

static void
free_items_store (BjbController *self)
{
  gtk_list_store_clear (GTK_LIST_STORE (self->priv->model));
}

static void
bjb_controller_finalize (GObject *object)
{
  BjbController *self = BJB_CONTROLLER(object);
  BjbControllerPrivate *priv = self->priv ;

  free_items_store (self);
  g_object_unref (priv->model);

  g_object_unref (priv->completion);
  g_free (priv->needle);
  g_list_free (priv->items_to_show);

  G_OBJECT_CLASS (bjb_controller_parent_class)->finalize (object);
}

static void
bjb_controller_get_property (GObject  *object,
               guint     property_id,
               GValue   *value,
               GParamSpec *pspec)
{
  BjbController *self = BJB_CONTROLLER (object);

  switch (property_id)
  {
  case PROP_BOOK:
    g_value_set_object (value, self->priv->book);
    break;
  case PROP_WINDOW:
    g_value_set_object (value, self->priv->window);
    break;
  case PROP_NEEDLE:
    g_value_set_string(value, self->priv->needle);
    break;
  case PROP_MODEL:
    g_value_set_object(value, self->priv->model);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bjb_controller_set_property (GObject  *object,
               guint     property_id,
               const GValue *value,
               GParamSpec *pspec)
{
  BjbController *self = BJB_CONTROLLER (object);

  switch (property_id)
  {
  case PROP_BOOK:
    bjb_controller_set_book(self,g_value_get_object(value));
    break;
  case PROP_WINDOW:
    self->priv->window = g_value_get_object (value);
    break;
  case PROP_NEEDLE:
    self->priv->needle = g_strdup (g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
bjb_controller_add_item (BjbController *self,
                         BijiItem      *item,
                         gboolean       prepend)
{
  GtkTreeIter    iter;
  GtkListStore  *store;
  GdkPixbuf     *pix = NULL;
  gchar         *uuid;

  g_return_if_fail (BIJI_IS_ITEM (item));
  store = GTK_LIST_STORE (self->priv->model);

  if (prepend)
    gtk_list_store_insert (store, &iter, 0);

  else
    gtk_list_store_append (store, &iter);

  /* Only append notes which are not templates. Currently useless */
  if (BIJI_IS_NOTE_OBJ (item)
      && biji_note_obj_is_template (BIJI_NOTE_OBJ (item)))
    return;

  /* First , if there is a gd main view , and if gd main view
   * is a list, then load the smaller emblem */
  if (bjb_window_base_get_view_type (self->priv->window) == BJB_WINDOW_BASE_MAIN_VIEW
      && bjb_window_base_get_main_view (self->priv->window)
      && bjb_main_view_get_view_type
                (bjb_window_base_get_main_view (self->priv->window)) == GD_MAIN_VIEW_LIST)
    pix = biji_item_get_emblem (item);

  /* Else, load the icon */
  if (!pix)
    pix = biji_item_get_icon (item);

  /* Appart from pixbuf, both icon & list view types
   * currently use the same model */
  uuid = biji_item_get_uuid (item);

  gtk_list_store_set (store, &iter,
       GD_MAIN_COLUMN_ID, uuid,
       GD_MAIN_COLUMN_URI, uuid,
       GD_MAIN_COLUMN_PRIMARY_TEXT, biji_item_get_title (item),
       GD_MAIN_COLUMN_SECONDARY_TEXT, NULL,
       GD_MAIN_COLUMN_ICON, pix,
       GD_MAIN_COLUMN_MTIME, biji_item_get_last_change (item),
       GD_MAIN_COLUMN_SELECTED, FALSE,
       -1);

  g_free (uuid);

}

/* If the user searches for notes, is the note searched? */
static void
bjb_controller_add_note_if_needed (BjbController *self,
                                   BijiNoteObj   *note,
                                   gboolean       prepend)
{
  gboolean need_to_add_note =FALSE;
  gchar *title, *content;
  GList *collections, *l;
  BjbControllerPrivate *priv = self->priv;
  BijiItem *item = BIJI_ITEM (note);

  /* No note... */
  if (!note || !BIJI_IS_NOTE_OBJ (note))
    return;

  /* No search - we add the note */
  if (!priv->needle || g_strcmp0 (priv->needle, "")==0)
  {
    need_to_add_note = TRUE;
  }

  /* a search.. we test...*/
  else
  {

    title = biji_item_get_title (item);
    content = biji_note_get_raw_text (note);

    /* matching title or content ... */
    if (g_strrstr (title  , priv->needle) != NULL ||
        g_strrstr (content, priv->needle) != NULL  )
      need_to_add_note = TRUE;

    /* last chance, matching collections... */
    else
    {
      collections = biji_note_obj_get_collections (note);
    
      for (l = collections; l != NULL; l=l->next)
      {
        if (g_strrstr (l->data, title))
        {
          need_to_add_note = TRUE;
          break;
        }
      }

      g_list_free (collections);
    }
  }

  if (need_to_add_note)
    bjb_controller_add_item (self, item, prepend);
}

void
bjb_controller_update_view (BjbController *self)
{
  GList *items, *l;

  /* Do not update if nothing to show */
  if (bjb_window_base_get_view_type (self->priv->window) != BJB_WINDOW_BASE_MAIN_VIEW)
    return;

  items = self->priv->items_to_show ;
  free_items_store (self);

  for (l = items; l != NULL; l = l->next)
  {
    bjb_controller_add_item (self, l->data, FALSE);
  }
}

static gint
most_recent_item_first (gconstpointer a, gconstpointer b)
{
  BijiItem *one = BIJI_ITEM (a);
  BijiItem *other = BIJI_ITEM (b);
  
  glong result = biji_item_get_last_change (other);
  return result - biji_item_get_last_change (one);
}

static void
sort_items (BjbController *self)
{
  self->priv->items_to_show = g_list_sort (self->priv->items_to_show,
                                           most_recent_item_first);
}

static void
sort_and_update (BjbController *self)
{
  sort_items (self);
  bjb_controller_update_view (self);

  g_signal_emit (G_OBJECT (self), bjb_controller_signals[DISPLAY_NOTES_CHANGED],0);
}

static void
update_controller_callback (GObject *source_object,
                            GAsyncResult *res,
                            gpointer user_data)
{
  GList *result;
  BjbController *self = BJB_CONTROLLER (user_data);

  result = biji_get_notes_with_strings_or_collection_finish (source_object, res, self->priv->book);
  self->priv->items_to_show = result;

  sort_and_update (self);
}          

void
bjb_controller_apply_needle (BjbController *self)
{
  BjbControllerPrivate *priv = self->priv;
  gchar *needle;

  if (priv->items_to_show)
    g_clear_pointer (&priv->items_to_show, g_list_free);
  
  needle = priv->needle;

  /* Show all notes */
  if (needle == NULL || g_strcmp0 (needle,"") == 0)
  {
    priv->items_to_show = biji_note_book_get_items (self->priv->book);

    /* If there are no note, report this */
    if (!priv->items_to_show)
      bjb_window_base_switch_to (self->priv->window,
                                 BJB_WINDOW_BASE_NO_NOTE);

    /* Otherwise do show existing notes */
    else
      sort_and_update (self);

    return;
  }

  /* There is a research, apply lookup
   * TODO : also look for collections */
  biji_get_notes_with_string_or_collection_async (needle, update_controller_callback, self);
}

static void
on_needle_changed (BjbController *self)
{
  bjb_controller_apply_needle (self);
  g_signal_emit (self, bjb_controller_signals[SEARCH_CHANGED], 0);
}

static void
add_item_to_completion (BijiItem *item, BjbController *self)
{
  GtkListStore *store;
  GtkTreeIter iter;

  store = GTK_LIST_STORE (self->priv->completion);

  // Search Tag.
  gtk_list_store_append (store, &iter);
  gtk_list_store_set (store, 
                      &iter, 
                      0, 
                      biji_item_get_title (item),
                      -1);
}

static void
refresh_completion(BjbController *self)
{
  GList *items;

  gtk_list_store_clear (GTK_LIST_STORE (self->priv->completion));
  items = biji_note_book_get_items (self->priv->book);

  if (items)
  {
    g_list_foreach (items, (GFunc) add_item_to_completion, self);
    g_list_free (items);
  }
}

static gboolean
bjb_controller_get_iter_at_note (BjbController *self, BijiNoteObj *note, GtkTreeIter **iter)
{
  BjbControllerPrivate *priv = self->priv;
  gchar *needle = biji_item_get_uuid (BIJI_ITEM (note));
  gboolean retval = FALSE;
  gboolean still = gtk_tree_model_get_iter_first (priv->model, *iter);

  while (still)
  {
    gchar *note_path;

    gtk_tree_model_get (priv->model, *iter, GD_MAIN_COLUMN_URI, &note_path,-1);

    if (g_strcmp0 (note_path, needle)==0)
      retval = TRUE;

    g_free (note_path);

    if (retval)
      break;

    else
      still = gtk_tree_model_iter_next (priv->model, *iter);
  }

  g_free (needle);
  return retval;
}

/* Depending on the change at data level,
 * the view has to be totaly refreshed or just amended */
static void
on_book_changed (BijiNoteBook           *book,
                 BijiNoteBookChangeFlag  flag,
                 gpointer               *note_obj,
                 BjbController          *self)
{
  BjbControllerPrivate *priv = self->priv;
  BijiNoteObj *note = BIJI_NOTE_OBJ (note_obj);
  BijiItem    *item = BIJI_ITEM (note);
  GtkTreeIter iter;
  GtkTreeIter *p_iter = &iter;

  switch (flag)
  {
    /* If this is a *new* note, per def prepend
     * But do not add a new note to a search window */
    case BIJI_BOOK_NOTE_ADDED:
        bjb_controller_add_note_if_needed (self, note, TRUE);
        priv->items_to_show = g_list_prepend (priv->items_to_show, note);
        g_signal_emit (G_OBJECT (self), bjb_controller_signals[DISPLAY_NOTES_CHANGED],0);
      break;

    /* If the note is *amended*, then per definition we prepend.
     * but if we add other ordering this does not work */
    case BIJI_BOOK_NOTE_AMENDED:
      if (bjb_controller_get_iter_at_note (self, note, &p_iter))
      {
        gtk_list_store_remove (GTK_LIST_STORE (priv->model), p_iter);
        bjb_controller_add_note_if_needed (self, note, TRUE);
      }
      break;

    /* If color changed we just amend the icon */
    case BIJI_BOOK_NOTE_COLORED:
      if (bjb_main_view_get_view_type (
             bjb_window_base_get_main_view (self->priv->window)) == GD_MAIN_VIEW_ICON
          && bjb_controller_get_iter_at_note (self, note, &p_iter))
        gtk_list_store_set (GTK_LIST_STORE (priv->model), p_iter,
                            GD_MAIN_COLUMN_ICON, biji_item_get_icon (item), -1);
      else if (bjb_main_view_get_view_type (
             bjb_window_base_get_main_view (self->priv->window)) == GD_MAIN_VIEW_LIST
          && bjb_controller_get_iter_at_note (self, note, &p_iter))
        gtk_list_store_set (GTK_LIST_STORE (priv->model), p_iter,
                            GD_MAIN_COLUMN_ICON, biji_item_get_emblem (item), -1);
      break;

    case BIJI_BOOK_NOTE_TRASHED:
      if (bjb_controller_get_iter_at_note (self, note, &p_iter))
        gtk_list_store_remove (GTK_LIST_STORE (priv->model), p_iter);

      priv->items_to_show = g_list_remove (priv->items_to_show, note);
      g_signal_emit (G_OBJECT (self), bjb_controller_signals[DISPLAY_NOTES_CHANGED],0);
      break;

    default:
      bjb_controller_apply_needle (self);
  }

  /* FIXME we refresh the whole completion model each time */
  refresh_completion(self);
}

void
bjb_controller_connect (BjbController *self)
{
  BjbControllerPrivate *priv = self->priv;
  
  if (!priv->connected)
  {
    priv->book_change = g_signal_connect (self->priv->book, "changed",
                                     G_CALLBACK(on_book_changed), self);
    priv->connected = TRUE;
  }

  bjb_controller_update_view (self);
}

void
bjb_controller_disconnect (BjbController *self)
{
  BjbControllerPrivate *priv = self->priv;

  g_signal_handler_disconnect (priv->book, priv->book_change);
  priv->book_change = 0;
}

static void
bjb_controller_constructed (GObject *obj)
{
  G_OBJECT_CLASS(bjb_controller_parent_class)->constructed(obj);
}

static void
bjb_controller_class_init (BjbControllerClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  g_type_class_add_private (klass, sizeof (BjbControllerPrivate));

  object_class->get_property = bjb_controller_get_property;
  object_class->set_property = bjb_controller_set_property;
  object_class->finalize = bjb_controller_finalize;
  object_class->constructed = bjb_controller_constructed;

  bjb_controller_signals[SEARCH_CHANGED] = g_signal_new ( "search-changed" ,
                                                  G_OBJECT_CLASS_TYPE (klass),
                                                  G_SIGNAL_RUN_LAST,
                                                  0, 
                                                  NULL, 
                                                  NULL,
                                                  g_cclosure_marshal_VOID__VOID,
                                                  G_TYPE_NONE,
                                                  0);

  bjb_controller_signals[DISPLAY_NOTES_CHANGED] = g_signal_new ( "display-notes-changed" ,
                                                  G_OBJECT_CLASS_TYPE (klass),
                                                  G_SIGNAL_RUN_LAST,
                                                  0, 
                                                  NULL, 
                                                  NULL,
                                                  g_cclosure_marshal_VOID__BOOLEAN,
                                                  G_TYPE_NONE,
                                                  0);

  properties[PROP_BOOK] = g_param_spec_object ("book",
                                               "Book",
                                               "The BijiNoteBook",
                                               BIJI_TYPE_NOTE_BOOK,
                                               G_PARAM_READWRITE |
                                               G_PARAM_CONSTRUCT |
                                               G_PARAM_STATIC_STRINGS);

  properties[PROP_WINDOW] = g_param_spec_object ("window",
                                                 "GtkWindow",
                                                 "BjbWindowBase",
                                                 BJB_TYPE_WINDOW_BASE,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT |
                                                 G_PARAM_STATIC_STRINGS);

  properties[PROP_NEEDLE] = g_param_spec_string ("needle",
                                                 "Needle",
                                                 "String to search notes",
                                                 NULL,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_CONSTRUCT |
                                                 G_PARAM_STATIC_STRINGS);


  properties[PROP_MODEL] = g_param_spec_object ("model",
                                                "Model",
                                                "The GtkTreeModel",
                                                GTK_TYPE_TREE_MODEL,
                                                G_PARAM_READABLE  |
                                                G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, properties);

}

BjbController *
bjb_controller_new (BijiNoteBook  *book,
                    GtkWindow     *window,
                    gchar         *needle)
{
  return g_object_new ( BJB_TYPE_CONTROLLER,
              "book", book,
              "window", window,
              "needle", needle,
              NULL); 
}

void
bjb_controller_set_book (BjbController *self, BijiNoteBook  *book )
{
  self->priv->book = book ;
  
  /* Only update completion.
   * Notes model is updated when needle changes */
  refresh_completion(self);
}

void
bjb_controller_set_needle (BjbController *self, const gchar *needle )
{
  if (self->priv->needle)
    g_free (self->priv->needle);

  self->priv->needle = g_strdup (needle);
  on_needle_changed (self);
}

gchar *
bjb_controller_get_needle (BjbController *self)
{
  if (!self->priv->needle)
    return NULL;

  return self->priv->needle;
}

GtkTreeModel *
bjb_controller_get_model  (BjbController *self)
{
  return self->priv->model ;
}

GtkTreeModel *
bjb_controller_get_completion(BjbController *self)
{
  return self->priv->completion ;
}

gboolean
bjb_controller_shows_item (BjbController *self)
{
  if (self->priv->items_to_show)
    return TRUE;

  return FALSE;
}
