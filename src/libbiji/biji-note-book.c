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
#include "biji-collection.h"


struct _BijiNoteBookPrivate
{
  /* Notes & Collections */
  GHashTable *items;

  /* Signals */
  gulong note_renamed ;

  GFile *location;
  GCancellable *load_cancellable;
};

/* Properties */
enum {
  PROP_0,
  PROP_LOCATION,
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
  self->priv = G_TYPE_INSTANCE_GET_PRIVATE (self, BIJI_TYPE_NOTE_BOOK,
                                            BijiNoteBookPrivate);

  /* Item path is key for table */
  self->priv->items = g_hash_table_new_full (g_str_hash,
                                             g_str_equal,
                                             NULL,
                                             g_object_unref);
}

static void
biji_note_book_finalize (GObject *object)
{
  BijiNoteBook *book = BIJI_NOTE_BOOK (object) ;

  if (book->priv->load_cancellable)
    g_cancellable_cancel (book->priv->load_cancellable);

  g_clear_object (&book->priv->load_cancellable);
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


  switch (property_id)
    {
    case PROP_LOCATION:
      self->priv->location = g_value_dup_object (value);
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

gboolean
biji_note_book_notify_changed (BijiNoteBook            *book,
                               BijiNoteBookChangeFlag   flag,
                               BijiItem                *item)
{
  g_signal_emit (G_OBJECT (book), biji_book_signals[BOOK_AMENDED], 0, flag, item);
  return FALSE;
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
_biji_note_book_add_one_note (BijiNoteBook *book, BijiNoteObj *note)
{
  g_return_if_fail (BIJI_IS_NOTE_OBJ (note));

  _biji_note_obj_set_book (note, (gpointer) book);

  /* Add it to the list */
  g_hash_table_insert (book->priv->items,
                       (gpointer) biji_item_get_uuid (BIJI_ITEM (note)),
                       note);

  /* Notify */
  g_signal_connect (note, "changed", G_CALLBACK (book_on_note_changed_cb), book);
  g_signal_connect (note, "renamed", G_CALLBACK (book_on_note_changed_cb), book);
  g_signal_connect (note, "color-changed", G_CALLBACK (book_on_item_icon_changed_cb), book);
}

#define ATTRIBUTES_FOR_NOTEBOOK "standard::content-type,standard::name"

static void
load_location_error (GFile *location,
                     GError *error)
{
  gchar *path = g_file_get_path (location);
  g_printerr ("Unable to load location %s: %s", path, error->message);

  g_free (path);
  g_error_free (error);
}

static void
release_enum_cb (GObject *source, GAsyncResult *res, gpointer user_data)
{
  g_file_enumerator_close_finish (G_FILE_ENUMERATOR (source),
                                  res,
                                  NULL);
  g_object_unref (source);
}

static void
create_collection_if_needed (gpointer key,
                             gpointer value,
                             gpointer user_data)
{
  BijiNoteBook *book = BIJI_NOTE_BOOK (user_data);
  BijiCollection *collection;

  collection = g_hash_table_lookup (book->priv->items, key);

  if (!collection)
  {
    collection = biji_collection_new (G_OBJECT (book), key, value);
    g_hash_table_insert (book->priv->items,
                         g_strdup (key),
                         collection);

    g_signal_connect (collection, "deleted",
                      G_CALLBACK (on_item_deleted_cb), book);
    g_signal_connect (collection , "icon-changed",
                      G_CALLBACK (book_on_item_icon_changed_cb), book);
  }
}

static void
load_book_finish (GObject *source_object,
                  GAsyncResult *res,
                  gpointer user_data)
{
  BijiNoteBook *self = BIJI_NOTE_BOOK (user_data);
  GHashTable *collections;

  collections = biji_get_all_collections_finish (source_object, res);
  g_hash_table_foreach (collections, create_collection_if_needed, user_data);
  g_hash_table_destroy (collections);

  biji_note_book_notify_changed (self, BIJI_BOOK_MASS_CHANGE, NULL);
}

static void
enumerate_next_files_ready_cb (GObject *source,
                               GAsyncResult *res,
                               gpointer user_data)
{
  GFileEnumerator *enumerator = G_FILE_ENUMERATOR (source);
  BijiNoteBook *self;
  GList *files, *l;
  GError *error = NULL;
  gchar *base_path;

  files = g_file_enumerator_next_files_finish (enumerator, res, &error);
  g_file_enumerator_close_async (enumerator, G_PRIORITY_DEFAULT, NULL,
                                 release_enum_cb, NULL);

  if (error != NULL)
    {
      load_location_error (g_file_enumerator_get_container (enumerator), error);
      return;
    }

  self = user_data;
  base_path = g_file_get_path (self->priv->location);

  // now load the notes
  for (l = files; l != NULL; l = l->next)
    {
      GFileInfo *info;
      const gchar *name;
      gchar *path;
      BijiNoteObj *note;

      info = l->data;
      name = g_file_info_get_name (info);

      if (!g_str_has_suffix (name, ".note"))
        continue;

      path = g_build_filename (base_path, name, NULL);
      note = biji_note_get_new_from_file (path);

      _biji_note_book_add_one_note (self, note);

      g_free (path);
    }

  g_free (base_path);
  g_list_free_full (files, g_object_unref);

  /* Now we have all notes,
   * load the collections and we're good to notify loading done */
  biji_get_all_collections_async (load_book_finish, self);
}

static void
enumerate_children_ready_cb (GObject *source,
                             GAsyncResult *res,
                             gpointer user_data)
{
  GFile *location = G_FILE (source);
  GFileEnumerator *enumerator;
  GError *error = NULL;
  BijiNoteBook *self;

  enumerator = g_file_enumerate_children_finish (location,
                                                 res, &error);

  if (error != NULL)
    {
      load_location_error (location, error);
      return;
    }

  self = user_data;

  // enumerate all files
  g_file_enumerator_next_files_async (enumerator, G_MAXINT,
                                      G_PRIORITY_DEFAULT,
                                      self->priv->load_cancellable,
                                      enumerate_next_files_ready_cb,
                                      self);
}

static void
note_book_load_from_location (BijiNoteBook *self)
{
  self->priv->load_cancellable = g_cancellable_new ();
  g_file_enumerate_children_async (self->priv->location,
                                   ATTRIBUTES_FOR_NOTEBOOK, 0,
                                   G_PRIORITY_DEFAULT,
                                   self->priv->load_cancellable,
                                   enumerate_children_ready_cb,
                                   self);
}

static void
biji_note_book_constructed (GObject *object)
{
  BijiNoteBook *self = BIJI_NOTE_BOOK (object);
  gchar *filename;
  GFile *cache;

  G_OBJECT_CLASS (biji_note_book_parent_class)->constructed (object);

  /* Ensure cache directory for icons */
  filename = g_build_filename (g_get_user_cache_dir (),
                               g_get_application_name (),
                               NULL);
  cache = g_file_new_for_path (filename);
  g_free (filename);
  g_file_make_directory (cache, NULL, NULL);
  g_object_unref (cache);

  note_book_load_from_location (self);
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
    g_param_spec_object("location",
                        "The book location",
                        "The location where the notes are loaded and saved",
                        G_TYPE_FILE,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, BIJI_BOOK_PROPERTIES, properties);
  g_type_class_add_private (klass, sizeof (BijiNoteBookPrivate));
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

    /* Ref note first, hash_table won't finalize it & we can delete it*/
    g_object_ref (to_delete);
    g_hash_table_remove (book->priv->items, path);
    biji_item_trash (item);

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
    _biji_note_book_add_one_note (book, BIJI_NOTE_OBJ (item));

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
biji_note_book_new (GFile *location)
{
  return g_object_new(BIJI_TYPE_NOTE_BOOK,
                      "location", location,
                      NULL);
}

BijiNoteObj *
biji_note_get_new_from_file (const gchar* path)
{
  BijiNoteObj* ret = biji_note_obj_new_from_path (path);

  /* The deserializer will handle note type */
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

  folder = g_file_get_path (book->priv->location);

  while (!ret)
  {
    name = biji_note_book_get_uuid ();
    path = g_build_filename (folder, name, NULL);
    g_free (name);

    if (!g_hash_table_lookup (book->priv->items, path))
      ret = biji_note_obj_new_from_path (path);

    g_free (path);
  }

  biji_note_obj_set_all_dates_now (ret);
  biji_note_obj_set_title_survives (ret, FALSE);
  return ret;
}

static char*
wrap_note_content (char *content)
{
  return g_strdup_printf("<html xmlns=\"http://www.w3.org/1999/xhtml\"><body>%s</body></html>", content);
}

BijiNoteObj *
biji_note_book_note_new           (BijiNoteBook *book, gchar *str)
{
  BijiNoteObj *ret = get_note_skeleton (book);

  if (str)
  {
    gchar *unique, *html;

    unique = biji_note_book_get_unique_title (book, str);
    html = wrap_note_content (str);

    biji_note_obj_set_title (ret, unique);
    biji_note_obj_set_raw_text (ret, str);
    biji_note_obj_set_html_content (ret, html);

    g_free (unique);
    g_free (html);
  }

  biji_note_obj_save_note (ret);
  biji_note_book_add_item (book, BIJI_ITEM (ret), TRUE);

  return ret;
}
