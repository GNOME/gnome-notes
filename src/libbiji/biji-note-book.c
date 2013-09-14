/* bjb-note-book.c
 * Copyright (C) Pierre-Yves LUYTEN 2012 <py@luyten.fr>
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

#include <gtk/gtk.h>
#include <uuid/uuid.h>

#include "libbiji.h"
#include "biji-local-note.h" // FIXME !!!! biji_provider_note_new ()
#include "biji-collection.h"
#include "biji-error.h"

#include "provider/biji-local-provider.h"
#include "provider/biji-own-cloud-provider.h"


struct _BijiNoteBookPrivate
{
  /* Notes & Collections.
   * Keep a direct pointer to local provider for convenience. */

  GHashTable *items;
  GHashTable *providers;
  BijiProvider *local_provider;

  /* Signals */
  gulong note_renamed ;

  GFile *location;
  GError *error;
  TrackerSparqlConnection *connection;
  ZeitgeistLog *log;
  GdkRGBA color;
};


/* Properties */
enum {
  PROP_0,
  PROP_LOCATION,
  PROP_COLOR,
  PROP_ERROR,
  BIJI_BOOK_PROPERTIES
};


/* Signals */
enum {
  BOOK_AMENDED,
  BIJI_BOOK_SIGNALS
};

static guint biji_book_signals[BIJI_BOOK_SIGNALS] = { 0 };
static GParamSpec *properties[BIJI_BOOK_PROPERTIES] = { NULL, };



#define BIJI_NOTE_BOOK_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), BIJI_TYPE_NOTE_BOOK, BijiNoteBookPrivate))

G_DEFINE_TYPE (BijiNoteBook, biji_note_book, G_TYPE_OBJECT);






static void
biji_note_book_init (BijiNoteBook *self)
{
  BijiNoteBookPrivate *priv;

  priv = self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_NOTE_BOOK,
                                                   BijiNoteBookPrivate);

  /* Item path is key for table */
  priv->items = g_hash_table_new_full (g_str_hash,
                                       g_str_equal,
                                       NULL,
                                       g_object_unref);

  /*
   * Providers are the different notes storage
   * the hash table use an id
   * 
   * - local files stored notes = "local"
   * - own cloud notes = account_get_id
   */

  priv->providers = g_hash_table_new (g_str_hash, g_str_equal);
}


ZeitgeistLog *
biji_note_book_get_zg_log (BijiNoteBook *book)
{
  return book->priv->log;
}


TrackerSparqlConnection *
biji_note_book_get_tracker_connection (BijiNoteBook *book)
{
  return book->priv->connection;
}



GList *
biji_note_book_get_providers         (BijiNoteBook *book)
{
  GList *providers, *l, *retval;

  retval = NULL;
  providers = g_hash_table_get_values (book->priv->providers);

  for (l = providers; l != NULL; l = l->next)
  {
    retval = g_list_prepend (
               retval, (gpointer) biji_provider_get_info (BIJI_PROVIDER (l->data)));
  }

  g_list_free (providers);
  return retval;
}


static void
biji_note_book_finalize (GObject *object)
{
  BijiNoteBook *book = BIJI_NOTE_BOOK (object) ;


  g_clear_object (&book->priv->location);
  g_hash_table_destroy (book->priv->items);

  G_OBJECT_CLASS (biji_note_book_parent_class)->finalize (object);
}




static void
biji_note_book_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BijiNoteBook *self = BIJI_NOTE_BOOK (object);
  GdkRGBA *color;

  switch (property_id)
    {
    case PROP_LOCATION:
      self->priv->location = g_value_dup_object (value);
      break;

    case PROP_ERROR:
      self->priv->error = g_value_get_pointer (value);
      break;

    case PROP_COLOR:
      color = g_value_get_pointer (value);
      self->priv->color.red = color->red;
      self->priv->color.blue = color->blue;
      self->priv->color.green = color->green;
      self->priv->color.alpha = color->alpha;
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_note_book_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BijiNoteBook *self = BIJI_NOTE_BOOK (object);

  switch (property_id)
    {
    case PROP_LOCATION:
      g_value_set_object (value, self->priv->location);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static gboolean
title_is_unique (BijiNoteBook *book, gchar *title)
{
  gboolean is_unique = TRUE;
  BijiItem *iter;
  GList *items, *l;

  items = g_hash_table_get_values (book->priv->items);

  for ( l=items ; l != NULL ; l = l->next)
  {
    iter = BIJI_ITEM (l->data);

    if (g_strcmp0 (biji_item_get_title (iter), title) == 0)
    {
     is_unique = FALSE;
     break;
    }
  }

  g_list_free (items);
  return is_unique;
}

/* If title not unique, add sufffix "n", starting with 2, until ok */
gchar *
biji_note_book_get_unique_title (BijiNoteBook *book, const gchar *title)
{
  if (!book)
    return g_strdup (title);

  gchar *new_title;

  if (!title)
    title = "";

  new_title = g_strdup (title);
  gint suffix = 2;

  while (!title_is_unique (book, new_title))
  {
    g_free (new_title);
    new_title = g_strdup_printf("%s (%i)", title, suffix);
    suffix++;
  }

  return new_title;
}


void
biji_note_book_notify_changed (BijiNoteBook            *book,
                               BijiNoteBookChangeFlag   flag,
                               BijiItem                *item)
{
  g_signal_emit (book,
                 biji_book_signals[BOOK_AMENDED],
                 0,
                 flag,
                 item);
}


/* TODO : use the same for note, put everything there
 * rather calling a func */
static void
on_item_deleted_cb (BijiItem *item, BijiNoteBook *book)
{
  biji_note_book_remove_item (book, item);
}


void
book_on_note_changed_cb (BijiNoteObj *note, BijiNoteBook *book)
{
  biji_note_book_notify_changed (book, BIJI_BOOK_NOTE_AMENDED, BIJI_ITEM (note));
}

static void
book_on_item_icon_changed_cb (BijiNoteObj *note, BijiNoteBook *book)
{
  biji_note_book_notify_changed (book, BIJI_BOOK_ITEM_ICON_CHANGED, BIJI_ITEM (note));
}

static void
_biji_note_book_add_one_item (BijiNoteBook *book, BijiItem *item)
{
  g_return_if_fail (BIJI_IS_ITEM (item));


  /* Add it to the list */
  g_hash_table_insert (book->priv->items,
                       (gpointer) biji_item_get_uuid (item), item);

  /* Notify */
  if (BIJI_IS_NOTE_OBJ (item))
  {
    g_signal_connect (item, "changed", G_CALLBACK (book_on_note_changed_cb), book);
    g_signal_connect (item, "renamed", G_CALLBACK (book_on_note_changed_cb), book);
    g_signal_connect (item, "color-changed", G_CALLBACK (book_on_item_icon_changed_cb), book);
  }

  else if (BIJI_IS_COLLECTION (item))
  {
    g_signal_connect (item, "deleted", G_CALLBACK (on_item_deleted_cb), book);
    g_signal_connect (item , "icon-changed", G_CALLBACK (book_on_item_icon_changed_cb), book);
  }
}


static void
on_provider_loaded_cb (BijiProvider *provider,
                       GList *items,
                       BijiNoteBook *book)
{
  BijiItem *item = NULL;
  BijiNoteBookChangeFlag flag = BIJI_BOOK_CHANGE_FLAG;
  GList *l;
  gint i = 0;


  for (l=items; l!=NULL; l=l->next)
  {
    if (BIJI_IS_ITEM (l->data))
    {
      _biji_note_book_add_one_item (book, l->data);
      i++;
    }
  }

  if (i==1)
    flag = BIJI_BOOK_ITEM_ADDED;

  else if (i>1)
    flag = BIJI_BOOK_MASS_CHANGE;


  if (flag > BIJI_BOOK_CHANGE_FLAG)
    biji_note_book_notify_changed (book, flag, item);
}


/* 
 * It should be the right place
 * to stock somehow providers list
 * in order to handle properly book__note_new ()
 * 
 */
static void
_add_provider (BijiNoteBook *self,
               BijiProvider *provider)
{
  g_return_if_fail (BIJI_IS_PROVIDER (provider));


  /* we can safely cast get_id from const to gpointer
   * since there is no key free func */

  const BijiProviderInfo *info;

  info = biji_provider_get_info (provider);
  g_hash_table_insert (self->priv->providers, (gpointer) info->unique_id, provider);
  g_signal_connect (provider, "loaded", 
                    G_CALLBACK (on_provider_loaded_cb), self);
}


void
biji_note_book_add_goa_object (BijiNoteBook *self,
                               GoaObject *object)
{
  BijiProvider *provider;
  GoaAccount *account;
  const gchar *type;

  provider = NULL;
  account =  goa_object_get_account (object);

  if (GOA_IS_ACCOUNT (account))
  {
    type = goa_account_get_provider_type (account);

    if (g_strcmp0 (type, "owncloud") ==0)
      provider = biji_own_cloud_provider_new (self, object);
  }

  _add_provider (self, provider);
}


static void
biji_note_book_constructed (GObject *object)
{
  BijiNoteBook *self;
  BijiNoteBookPrivate *priv;
  gchar *filename;
  GFile *cache;
  GError *error;


  G_OBJECT_CLASS (biji_note_book_parent_class)->constructed (object);
  self = BIJI_NOTE_BOOK (object);
  priv = self->priv;
  error = NULL;

  /* If tracker fails for some reason,
   * do not attempt anything */
  priv->connection = tracker_sparql_connection_get (NULL, &error);

  if (error)
  {
    g_warning ("%s", error->message);
    g_error_free (error);
    priv->error = g_error_new (BIJI_ERROR, BIJI_ERROR_TRACKER, "Tracker is not available");
    return;
  }

  priv->log = biji_zeitgeist_init ();

  /* Ensure cache directory for icons */
  filename = g_build_filename (g_get_user_cache_dir (),
                               g_get_application_name (),
                               NULL);
  cache = g_file_new_for_path (filename);
  g_free (filename);
  g_file_make_directory (cache, NULL, NULL);
  g_object_unref (cache);

  priv->local_provider = biji_local_provider_new (self, self->priv->location);
  _add_provider (self, priv->local_provider);
}


static void
biji_note_book_class_init (BijiNoteBookClass *klass)
{
  GObjectClass* object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = biji_note_book_finalize;
  object_class->constructed = biji_note_book_constructed;
  object_class->set_property = biji_note_book_set_property;
  object_class->get_property = biji_note_book_get_property;

  biji_book_signals[BOOK_AMENDED] =
    g_signal_new ("changed", G_OBJECT_CLASS_TYPE (klass), G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,                         /* offset & accumulator */
                  _biji_marshal_VOID__ENUM_POINTER,
                  G_TYPE_NONE, 2, G_TYPE_INT, G_TYPE_POINTER);

  properties[PROP_LOCATION] =
    g_param_spec_object ("location",
                         "The book location",
                         "The location where the notes are loaded and saved",
                         G_TYPE_FILE,
                         G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);


  properties[PROP_COLOR] =
    g_param_spec_pointer ("color",
                         "Default color",
                         "Note book default color for notes",
                         G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);


  properties[PROP_ERROR] =
    g_param_spec_pointer ("error",
                          "Unrecoverable error",
                          "Note book unrecoverable error",
                          G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, BIJI_BOOK_PROPERTIES, properties);
  g_type_class_add_private (klass, sizeof (BijiNoteBookPrivate));
}


void
biji_note_book_get_default_color (BijiNoteBook *book, GdkRGBA *color)
{
  g_return_if_fail (BIJI_IS_NOTE_BOOK (book));

  color->red = book->priv->color.red;
  color->blue = book->priv->color.blue;
  color->green = book->priv->color.green;
  color->alpha = book->priv->color.alpha;
}


gboolean 
biji_note_book_remove_item (BijiNoteBook *book, BijiItem *item)
{
  g_return_val_if_fail (BIJI_IS_NOTE_BOOK (book), FALSE);
  g_return_val_if_fail (BIJI_IS_ITEM      (item), FALSE);

  BijiItem *to_delete = NULL;
  const gchar *path;
  gboolean retval = FALSE;

  path = biji_item_get_uuid (item);
  to_delete = g_hash_table_lookup (book->priv->items, path);

  if (to_delete)
  {
    /* Signal before doing anything here. So the note is still
     * fully available for signal receiver. */
    biji_note_book_notify_changed (book, BIJI_BOOK_ITEM_TRASHED, to_delete);
    biji_item_trash (item);
    g_hash_table_remove (book->priv->items, path);

    retval = TRUE;
  }

  return retval;
}

gboolean
biji_note_book_add_item (BijiNoteBook *book, BijiItem *item, gboolean notify)
{
  g_return_val_if_fail (BIJI_IS_NOTE_BOOK (book), FALSE);
  g_return_val_if_fail (BIJI_IS_ITEM (item), FALSE);

  const gchar *uid;
  gboolean retval = TRUE;

  uid = biji_item_get_uuid (item);

  if (g_hash_table_lookup (book->priv->items, uid))
    retval = FALSE;

  else if (BIJI_IS_NOTE_OBJ (item))
    _biji_note_book_add_one_item (book, item);

  else if (BIJI_IS_COLLECTION (item))
  {
    g_hash_table_insert (book->priv->items,
                         (gpointer) biji_item_get_uuid (item),
                         item);

    g_signal_connect (item, "deleted",
                      G_CALLBACK (on_item_deleted_cb), book);
  }

  if (retval && notify)
    biji_note_book_notify_changed (book, BIJI_BOOK_ITEM_ADDED, item);

  return retval;
}

GList *
biji_note_book_get_items (BijiNoteBook *book)
{
  return g_hash_table_get_values (book->priv->items);
}

BijiItem *
biji_note_book_get_item_at_path (BijiNoteBook *book, const gchar *path)
{
  return g_hash_table_lookup (book->priv->items, (gconstpointer) path);
}


BijiNoteBook *
biji_note_book_new (GFile *location, GdkRGBA *color, GError **error)
{
  BijiNoteBook *retval;

  retval = g_object_new (BIJI_TYPE_NOTE_BOOK,
                           "location", location,
                           "color", color,
                           "error", *error,
                           NULL);

  *error = retval->priv->error;
  return retval;
}


BijiNoteObj *
biji_note_get_new_from_file (BijiNoteBook *book, const gchar* path)
{
  BijiInfoSet  set;
  BijiNoteObj *ret;

  set.url = (gchar*) path;
  set.mtime = 0;
  set.title = NULL;
  set.content = NULL;

  ret = biji_local_note_new_from_info (book->priv->local_provider, book, &set);
  biji_lazy_deserialize (ret);

  return ret ;
}

gchar *
biji_note_book_get_uuid (void)
{
  uuid_t unique;
  char out[40];

  uuid_generate (unique);
  uuid_unparse_lower (unique, out);
  return g_strdup_printf ("%s.note", out);
}

/* Common UUID skeleton for new notes. */
static BijiNoteObj *
get_note_skeleton (BijiNoteBook *book)
{
  BijiNoteObj *ret = NULL;
  gchar * folder, *name, *path;
  BijiInfoSet set;

  set.title = NULL;
  set.content = NULL;
  set.mtime = 0;
  folder = g_file_get_path (book->priv->location);

  while (!ret)
  {
    name = biji_note_book_get_uuid ();
    path = g_build_filename (folder, name, NULL);
    g_free (name);
    set.url = path;

    if (!g_hash_table_lookup (book->priv->items, path))
      ret = biji_local_note_new_from_info (book->priv->local_provider, book, &set);

    g_free (path);
  }

  biji_note_obj_set_all_dates_now (ret);
  return ret;
}


/* 
 * TODO : move this to local provider.
 */
static BijiNoteObj *
biji_note_book_local_note_new           (BijiNoteBook *book, gchar *str)
{
  BijiNoteObj *ret = get_note_skeleton (book);

  if (str)
  {
    gchar *unique, *html;

    unique = biji_note_book_get_unique_title (book, str);
    html = html_from_plain_text (str);

    biji_note_obj_set_title (ret, unique);
    biji_note_obj_set_raw_text (ret, str);
    biji_note_obj_set_html (ret, html);

    g_free (unique);
    g_free (html);
  }

  biji_note_obj_save_note (ret);
  biji_note_book_add_item (book, BIJI_ITEM (ret), TRUE);

  return ret;
}


/* 
 * Use "local" for a local note new
 * Use goa_account_get_id for goa
 */
BijiNoteObj *
biji_note_book_note_new            (BijiNoteBook *book,
                                    gchar        *str,
                                    gchar        *provider_id)
{
  BijiProvider *provider;

  // If we move local_note_new to create_note for local provider
  // we won't need this stupid switch.

  if (provider_id == NULL ||
      g_strcmp0 (provider_id, "local") == 0)
    return biji_note_book_local_note_new (book, str);


  provider = g_hash_table_lookup (book->priv->providers,
                                  provider_id);

  return BIJI_PROVIDER_GET_CLASS (provider)->create_note (provider, str);
}
                                    
