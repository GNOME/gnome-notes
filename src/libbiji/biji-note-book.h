#ifndef _BIJI_NOTE_BOOK_H_
#define _BIJI_NOTE_BOOK_H_

#include <glib-object.h>

#include "biji-note-obj.h"

G_BEGIN_DECLS

/* The flag tells if view should reload the whole model or not */
typedef enum
{
  BIJI_BOOK_CHANGE_FLAG,
  BIJI_BOOK_MASS_CHANGE,   // Startup, mass import.. rather rebuild the whole.
  BIJI_BOOK_ITEM_ADDED,    // Single item added
  BIJI_BOOK_ITEM_TRASHED,  // Single item trashed
  BIJI_BOOK_NOTE_AMENDED,  // Single note amended (title, content)
  BIJI_BOOK_NOTE_COLORED,  // Single note color
} BijiNoteBookChangeFlag;

#define BIJI_TYPE_NOTE_BOOK             (biji_note_book_get_type ())
#define BIJI_NOTE_BOOK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), BIJI_TYPE_NOTE_BOOK, BijiNoteBook))
#define BIJI_NOTE_BOOK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), BIJI_TYPE_NOTE_BOOK, BijiNoteBookClass))
#define BIJI_IS_NOTE_BOOK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BIJI_TYPE_NOTE_BOOK))
#define BIJI_IS_NOTE_BOOK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), BIJI_TYPE_NOTE_BOOK))
#define BIJI_NOTE_BOOK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), BIJI_TYPE_NOTE_BOOK, BijiNoteBookClass))

typedef struct _BijiNoteBookClass BijiNoteBookClass;
typedef struct _BijiNoteBook BijiNoteBook;

typedef struct _BijiNoteBookPrivate BijiNoteBookPrivate;

struct _BijiNoteBookClass
{
  GObjectClass parent_class;
};

struct _BijiNoteBook
{
  GObject parent_instance;
  BijiNoteBookPrivate *priv ;
};

GType biji_note_book_get_type (void) G_GNUC_CONST; 

BijiNoteBook * biji_note_book_new (GFile *location);

gchar * biji_note_book_get_unique_title (BijiNoteBook *book, gchar *title);

void biji_note_book_append_new_note (BijiNoteBook *book, BijiNoteObj *note, gboolean notify);

gboolean biji_note_book_notify_changed (BijiNoteBook           *book,
                                        BijiNoteBookChangeFlag  flag,
                                        BijiItem               *item);

gboolean biji_note_book_remove_item (BijiNoteBook *book, BijiItem *item);

BijiItem * biji_note_book_get_item_at_path (BijiNoteBook *book, gchar *path);

/* Get all items, either notes or collections
 * Free the GList, not its content */
GList * biji_note_book_get_items (BijiNoteBook *book);

BijiNoteObj* biji_note_get_new_from_file (const gchar* tomboy_format_note_path);

BijiNoteObj * biji_note_book_note_new (BijiNoteBook *book, gchar *str);

G_END_DECLS

#endif /* _BIJI_NOTE_BOOK_H_ */
