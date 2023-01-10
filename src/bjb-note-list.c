#define G_LOG_DOMAIN "bjb-note-list"

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#define _GNU_SOURCE
#include <string.h>
#include <handy.h>

#include "contrib/gtk.h"
#include "bjb-list-view-row.h"
#include "bjb-note-list.h"

struct _BjbNoteList
{
  GtkBox              parent_instance;

  GtkSearchEntry     *search_entry;
  GtkStack           *main_stack;
  GtkBox             *notes_box;
  GtkListBox         *notes_list_box;
  HdyStatusPage      *status_page;

  GListModel         *notes_list;
  GtkFilterListModel *filtered_notes;
  GtkFilter          *filter;
  BijiNotebook       *current_notebook;
  char               *search_term;
  char               *selected_note;
  gboolean            show_trash;
};


enum {
  SELECTION_CHANGED,
  N_SIGNALS
};

static guint signals[N_SIGNALS];
G_DEFINE_TYPE (BjbNoteList, bjb_note_list, GTK_TYPE_BOX)

static gboolean
note_list_filter_notes (gpointer note,
                        gpointer user_data)
{
  BjbNoteList *self = user_data;
  const char *title, *content;

  if (self->current_notebook)
    {
      GList *notebooks;
      const char *notebook;
      gboolean match = FALSE;

      notebook = biji_notebook_get_title (self->current_notebook);
      notebooks = biji_note_obj_get_notebooks (note);

      for (GList *item = notebooks; item; item = item->next)
        {
          if (g_strcmp0 (notebook, item->data) != 0)
            continue;

          match = TRUE;
          break;
        }

      if (!match)
        return FALSE;
    }

  if (!self->search_term || !*self->search_term)
    return TRUE;

  title = biji_note_obj_get_title (note);

  if (title && strcasestr (title, self->search_term))
    return TRUE;

  content = biji_note_obj_get_raw_text (note);

  if (content && strcasestr (content, self->search_term))
    return TRUE;

  return FALSE;
}

static void
note_list_items_changed_cb (BjbNoteList *self)
{
  guint n_items;

  g_assert (BJB_IS_NOTE_LIST (self));

  n_items = g_list_model_get_n_items (G_LIST_MODEL (self->filtered_notes));

  if (n_items == 0)
    gtk_stack_set_visible_child (self->main_stack, GTK_WIDGET (self->status_page));
  else
    gtk_stack_set_visible_child (self->main_stack, GTK_WIDGET (self->notes_box));

  n_items = g_list_model_get_n_items (self->notes_list);
  gtk_widget_set_sensitive (GTK_WIDGET (self->search_entry), n_items != 0);

  if (n_items == 0)
    {
      hdy_status_page_set_icon_name (self->status_page, "document-new-symbolic");
      hdy_status_page_set_title (self->status_page, _("Add Notes"));
      hdy_status_page_set_description (self->status_page, _("Use the + button to add a note"));
    }
  else if (self->search_term)
    {
      hdy_status_page_set_icon_name (self->status_page, "system-search-symbolic");
      hdy_status_page_set_title (self->status_page, _("No Results"));
      hdy_status_page_set_description (self->status_page, _("Try a different search"));
    }
}

static void
note_list_search_changed_cb (BjbNoteList *self)
{
  const char *search_str;

  g_assert (BJB_IS_NOTE_LIST (self));

  search_str = gtk_entry_get_text (GTK_ENTRY (self->search_entry));
  g_free (self->search_term);
  self->search_term = g_strdup (search_str);

  gtk_filter_changed (self->filter, GTK_FILTER_CHANGE_DIFFERENT);
}

static void
note_list_row_activated_cb (BjbNoteList    *self,
                            BjbListViewRow *row)
{
  const char *selected_note;

  g_assert (BJB_IS_NOTE_LIST (self));
  g_assert (BJB_IS_LIST_VIEW_ROW (row));

  selected_note = bjb_list_view_row_get_uuid (row);
  g_free (self->selected_note);
  self->selected_note = g_strdup (selected_note);

  g_signal_emit (self, signals[SELECTION_CHANGED], 0);
}

static void
bjb_note_list_finalize (GObject *object)
{
  BjbNoteList *self = (BjbNoteList *)object;

  g_clear_object (&self->notes_list);
  g_clear_object (&self->filtered_notes);
  g_clear_pointer (&self->search_term, g_free);
  g_clear_pointer (&self->selected_note, g_free);
  g_clear_object (&self->current_notebook);

  G_OBJECT_CLASS (bjb_note_list_parent_class)->finalize (object);
}

static void
bjb_note_list_class_init (BjbNoteListClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  object_class->finalize = bjb_note_list_finalize;

  signals [SELECTION_CHANGED] =
    g_signal_new ("selection-changed",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
  gtk_widget_class_set_template_from_resource (widget_class,
                                               "/org/gnome/Notes/"
                                               "ui/bjb-note-list.ui");

  gtk_widget_class_bind_template_child (widget_class, BjbNoteList, search_entry);
  gtk_widget_class_bind_template_child (widget_class, BjbNoteList, main_stack);
  gtk_widget_class_bind_template_child (widget_class, BjbNoteList, notes_box);
  gtk_widget_class_bind_template_child (widget_class, BjbNoteList, notes_list_box);
  gtk_widget_class_bind_template_child (widget_class, BjbNoteList, status_page);

  gtk_widget_class_bind_template_callback (widget_class, note_list_search_changed_cb);
  gtk_widget_class_bind_template_callback (widget_class, note_list_row_activated_cb);
}

static void
bjb_note_list_init (BjbNoteList *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

void
bjb_note_list_set_model (BjbNoteList *self,
                         GListModel  *model)
{
  g_return_if_fail (BJB_IS_NOTE_LIST (self));
  g_return_if_fail (G_IS_LIST_MODEL (model));

  if (self->notes_list)
    return;

  self->notes_list = g_object_ref (model);

  self->filter = gtk_custom_filter_new (note_list_filter_notes, self, NULL);
  self->filtered_notes = gtk_filter_list_model_new (self->notes_list, self->filter);
  g_signal_connect_object (self->filtered_notes, "items-changed",
                           G_CALLBACK (note_list_items_changed_cb),
                           self, G_CONNECT_SWAPPED);
  note_list_items_changed_cb (self);

  gtk_list_box_bind_model (self->notes_list_box, G_LIST_MODEL (self->filtered_notes),
                           (GtkListBoxCreateWidgetFunc)bjb_list_view_row_new_with_note,
                           self, NULL);
}

const char *
bjb_note_list_get_selected_note (BjbNoteList *self)
{
  g_return_val_if_fail (BJB_IS_NOTE_LIST (self), NULL);

  return self->selected_note;
}

void
bjb_note_list_set_notebook (BjbNoteList  *self,
                            BijiNotebook *notebook)
{
  g_return_if_fail (BJB_IS_NOTE_LIST (self));
  g_return_if_fail (!notebook || BIJI_IS_NOTEBOOK (notebook));

  if (g_set_object (&self->current_notebook, notebook))
    gtk_filter_changed (self->filter, GTK_FILTER_CHANGE_DIFFERENT);
}
