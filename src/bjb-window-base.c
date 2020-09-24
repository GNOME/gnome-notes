
#include <glib/gi18n.h>
#include <glib/gprintf.h>
#include <stdlib.h>

#include <libbiji/libbiji.h>

#include "bjb-application.h"
#include "bjb-empty-results-box.h"
#include "bjb-window-base.h"
#include "bjb-list-view.h"
#include "bjb-list-view-row.h"
#include "bjb-note-view.h"
#include "bjb-organize-dialog.h"
#include "bjb-search-toolbar.h"
#include "bjb-share.h"

#define BIJIBEN_MAIN_WIN_TITLE N_("Notes")
#define SAVE_GEOMETRY_ID_TIMEOUT 100 /* ms */

enum {
  PROP_0,
  PROP_NOTE,
  NUM_PROPERTIES
};

static GParamSpec *properties[NUM_PROPERTIES] = { NULL, };


enum {
  BJB_WIN_BASE_VIEW_CHANGED,
  BJB_WIN_BASE_ACTIVATED,
  BJB_WIN_BASE_SIGNALS
};

static guint bjb_win_base_signals [BJB_WIN_BASE_SIGNALS] = { 0 };


struct _BjbWindowBase
{
  HdyApplicationWindow  parent_instance;

  BjbSettings          *settings;
  BjbController        *controller;
  gchar                *entry; // FIXME, remove this


  GtkStack             *stack;
  BjbWindowViewType     current_view;
  BjbListView          *note_list;
  BjbNoteView          *note_view;
  GtkWidget            *spinner;
  GtkWidget            *no_note;

  /* when a note is opened */
  BijiNoteObj          *note;
  gboolean              detached; // detached note

  /* window geometry */
  gint                  width;
  gint                  height;
  gint                  pos_x;
  gint                  pos_y;
  gboolean              is_maximized;

  HdyLeaflet           *main_leaflet;
  HdyHeaderGroup       *header_group;
  GtkRevealer          *back_revealer;
  GtkStack             *main_stack;
  GtkWidget            *back_button;
  GtkWidget            *headerbar;
  GtkWidget            *note_box;
  GtkWidget            *note_headerbar;
  GtkWidget            *sidebar_box;
  GtkWidget            *search_bar;
  GtkWidget            *title_entry;
  GtkWidget            *last_update_item;
};

/* Gobject */
G_DEFINE_TYPE (BjbWindowBase, bjb_window_base, HDY_TYPE_APPLICATION_WINDOW)

static void
switch_to_sidebar (BjbWindowBase *self)
{
  hdy_leaflet_set_visible_child (self->main_leaflet, self->sidebar_box);
}

static void
switch_to_note_view (BjbWindowBase *self)
{
  hdy_leaflet_set_visible_child (self->main_leaflet, self->note_box);
}

static void
update_fold_state (BjbWindowBase *self)
{
  gboolean folded;

  folded = hdy_leaflet_get_folded (self->main_leaflet);

  gtk_widget_set_visible (GTK_WIDGET(self->back_revealer), folded);
  gtk_revealer_set_reveal_child (self->back_revealer, folded);
}

static void
notify_header_visible_child_cb (BjbWindowBase *self)
{
  update_fold_state (self);
}

static void
notify_fold_cb (BjbWindowBase *self)
{
  update_fold_state (self);
}

static void
on_note_renamed (BijiItem      *note,
                 BjbWindowBase *self)
{
  const char *str;

  str = biji_item_get_title (note);
  if (str == NULL || strlen(str) == 0)
    str = _("Untitled");
  gtk_entry_set_text (GTK_ENTRY (self->title_entry), str);
  hdy_header_bar_set_custom_title (HDY_HEADER_BAR (self->note_headerbar),
                                   self->title_entry);
  gtk_widget_show (self->title_entry);
}

static void
on_note_list_row_activated (GtkListBox    *box,
                            GtkListBoxRow *row,
                            gpointer       user_data)
{
  const char *note_uuid;
  BijiItem *to_open;
  BijiManager *manager;
  BjbWindowBase *self = BJB_WINDOW_BASE (user_data);

  manager = bjb_window_base_get_manager (GTK_WIDGET (self));
  note_uuid = bjb_list_view_row_get_uuid (BJB_LIST_VIEW_ROW (row));
  to_open = biji_manager_get_item_at_path (manager, note_uuid);

  if (to_open && BIJI_IS_NOTE_OBJ (to_open))
    {
      switch_to_note_view (self);

      /* Only open the note if it's not already opened. */
      if (!biji_note_obj_is_opened (BIJI_NOTE_OBJ (to_open)))
        {
          bjb_window_base_load_note_item (self, to_open);
        }
    }
  else if (to_open && BIJI_IS_NOTEBOOK (to_open))
    {
      bjb_controller_set_notebook (self->controller, BIJI_NOTEBOOK (to_open));
    }
}

static void
on_back_button_clicked (BjbWindowBase *self)
{
  switch_to_sidebar (self);
}

static void
on_new_note_clicked (BjbWindowBase *self)
{
  BijiNoteObj *result;
  BijiManager *manager;

  g_assert (BJB_IS_WINDOW_BASE (self));

  /* append note to notebook */
  manager = bjb_window_base_get_manager (GTK_WIDGET (self));
  result = biji_manager_note_new (manager,
                                  NULL,
                                  bjb_settings_get_default_location (self->settings));

  /* Go to that note */
  switch_to_note_view (self);
  bjb_window_base_load_note_item (self, BIJI_ITEM (result));
}

static void
on_title_changed (BjbWindowBase *self,
                  GtkEntry      *title)
{
  const char *str = gtk_entry_get_text (title);

  if (strlen (str) > 0)
    biji_note_obj_set_title (self->note, str);
}

static void
bjb_window_base_finalize (GObject *object)
{
  BjbWindowBase *self = BJB_WINDOW_BASE (object);

  g_clear_object (&self->controller);

  G_OBJECT_CLASS (bjb_window_base_parent_class)->finalize (object);
}




static void
bjb_window_base_get_property (GObject  *object,
                              guint     property_id,
                              GValue   *value,
                              GParamSpec *pspec)
{
  BjbWindowBase *self = BJB_WINDOW_BASE (object);

  switch (property_id)
  {
  case PROP_NOTE:
    g_value_set_object (value, self->note);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}


static void
bjb_window_base_set_property (GObject  *object,
                              guint     property_id,
                              const GValue *value,
                              GParamSpec *pspec)
{
  BjbWindowBase *self = BJB_WINDOW_BASE (object);

  switch (property_id)
  {
  case PROP_NOTE:
    self->note = g_value_get_object (value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static gboolean
on_key_pressed_cb (BjbWindowBase *self, GdkEvent *event)
{
  GApplication *app = g_application_get_default ();
  GdkModifierType modifiers = gtk_accelerator_get_default_mod_mask ();

  if ((event->key.state & modifiers) == GDK_MOD1_MASK &&
      event->key.keyval == GDK_KEY_Left &&
      (self->current_view == BJB_WINDOW_BASE_MAIN_VIEW ||
       self->current_view == BJB_WINDOW_BASE_ARCHIVE_VIEW))
  {
    BijiItemsGroup items;

    items = bjb_controller_get_group (self->controller);

    /* Back to main view from trash bin */
    if (items == BIJI_ARCHIVED_ITEMS)
      bjb_controller_set_group (self->controller, BIJI_LIVING_ITEMS);
    /* Back to main view */
    else
      bjb_controller_set_notebook (self->controller, NULL);

    return TRUE;
  }

  switch (event->key.keyval)
  {
    case GDK_KEY_F1:
      if ((event->key.state & modifiers) != GDK_CONTROL_MASK)
        {
          bjb_app_help (BJB_APPLICATION (app));
          return TRUE;
        }
      break;

    case GDK_KEY_F10:
      if ((event->key.state & modifiers) != GDK_CONTROL_MASK)
        {
          /* FIXME: Port this to BjbWindowBase. */
          /* bjb_main_toolbar_open_menu (self->main_toolbar); */
          return TRUE;
        }
      break;

    case GDK_KEY_F2:
    case GDK_KEY_F3:
    case GDK_KEY_F4:
    case GDK_KEY_F5:
    case GDK_KEY_F6:
    case GDK_KEY_F7:
    case GDK_KEY_F8:
    case GDK_KEY_F9:
    case GDK_KEY_F11:
      return TRUE;

    case GDK_KEY_q:
      if ((event->key.state & modifiers) == GDK_CONTROL_MASK)
        {
          g_application_quit (app);
          return TRUE;
        }
      break;

    default:
      ;
    }

  return FALSE;
}

static void
on_detach_window_cb (GSimpleAction *action,
                     GVariant      *parameter,
                     gpointer       user_data)
{
  BjbWindowBase *self = BJB_WINDOW_BASE (user_data);
  BijiNoteObj   *note = bjb_window_base_get_note (self);

  if (!note)
    return;

  if (biji_note_obj_is_trashed (note))
    bjb_window_base_switch_to (self, BJB_WINDOW_BASE_ARCHIVE_VIEW);
  else
    bjb_window_base_switch_to (self, BJB_WINDOW_BASE_MAIN_VIEW);

  bijiben_new_window_for_note (g_application_get_default (), note);
}

static void
on_paste_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  BijiNoteObj *note = bjb_window_base_get_note (BJB_WINDOW_BASE (user_data));

  if (!note)
    return;

  biji_webkit_editor_paste (BIJI_WEBKIT_EDITOR (biji_note_obj_get_editor (note)));
}

static void
on_undo_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  BijiNoteObj *note = bjb_window_base_get_note (BJB_WINDOW_BASE (user_data));

  if (!note)
    return;

  biji_webkit_editor_undo (BIJI_WEBKIT_EDITOR (biji_note_obj_get_editor (note)));
}

static void
on_redo_cb (GSimpleAction *action,
            GVariant      *parameter,
            gpointer       user_data)
{
  BijiNoteObj *note = bjb_window_base_get_note (BJB_WINDOW_BASE (user_data));

  if (!note)
    return;

  biji_webkit_editor_redo (BIJI_WEBKIT_EDITOR (biji_note_obj_get_editor (note)));
}

static void
on_view_notebooks_cb (GSimpleAction *action,
                      GVariant      *parameter,
                      gpointer       user_data)
{
  BjbWindowBase *self = BJB_WINDOW_BASE (user_data);
  BijiNoteObj *note = bjb_window_base_get_note (self);
  g_autoptr (GList) list = NULL;

  if (!note)
    return;

  list = g_list_append (list, note);
  bjb_organize_dialog_new (GTK_WINDOW (self), list);
}

static void
on_email_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  BijiNoteObj *note = bjb_window_base_get_note (BJB_WINDOW_BASE (user_data));

  if (!note)
    return;

  on_email_note_callback (note);
}

static void
on_trash_cb (GSimpleAction *action,
             GVariant      *parameter,
             gpointer       user_data)
{
  BijiNoteObj *note = bjb_window_base_get_note (BJB_WINDOW_BASE (user_data));

  if (!note)
    return;

  /* Delete the note from notebook
   * The deleted note will emit a signal. */
  biji_item_trash (BIJI_ITEM (note));
}

static void
on_close (GSimpleAction *action,
          GVariant      *parameter,
          gpointer       user_data)
{
  GtkApplicationWindow *window;

  window = GTK_APPLICATION_WINDOW (user_data);

  gtk_window_close (GTK_WINDOW (window));
}

static void
bjb_window_base_save_geometry (BjbWindowBase *self)
{
  GSettings *settings = G_SETTINGS (self->settings);

  g_settings_set_boolean (settings, "window-maximized", self->is_maximized);
  g_settings_set (settings, "window-size", "(ii)", self->width, self->height);
  g_settings_set (settings, "window-position", "(ii)", self->pos_x, self->pos_y);
}

static void
bjb_window_base_load_geometry (BjbWindowBase *self)
{
  GSettings *settings = G_SETTINGS (self->settings);

  self->is_maximized = g_settings_get_boolean (settings, "window-maximized");
  g_settings_get (settings, "window-size", "(ii)", &self->width, &self->height);
  g_settings_get (settings, "window-position", "(ii)", &self->pos_x, &self->pos_y);
}

/* Just disconnect to avoid crash, the finalize does the real
 * job */
static void
bjb_window_base_destroy (gpointer a, BjbWindowBase * self)
{
  bjb_controller_disconnect (self->controller);
  bjb_window_base_save_geometry (self);
}

static gboolean
bjb_window_base_configure_event (GtkWidget         *widget,
                                 GdkEventConfigure *event)
{
  BjbWindowBase *self;

  self = BJB_WINDOW_BASE (widget);

  self->is_maximized = gtk_window_is_maximized (GTK_WINDOW (self));
  if (!self->is_maximized)
    {
      gtk_window_get_size (GTK_WINDOW (self), &self->width, &self->height);
      gtk_window_get_position (GTK_WINDOW (self), &self->pos_x, &self->pos_y);
    }

  return GTK_WIDGET_CLASS (bjb_window_base_parent_class)->configure_event (widget,
                                                                           event);
}

static GActionEntry win_entries[] = {
  { "detach-window", on_detach_window_cb, NULL, NULL, NULL },
  { "paste", on_paste_cb, NULL, NULL, NULL },
  { "undo", on_undo_cb, NULL, NULL, NULL },
  { "redo", on_redo_cb, NULL, NULL, NULL },
  { "view-notebooks", on_view_notebooks_cb, NULL, NULL, NULL },
  { "email", on_email_cb, NULL, NULL, NULL },
  { "trash", on_trash_cb, NULL, NULL, NULL },
  { "close", on_close },
};

/* Gobj */
static void
bjb_window_base_constructed (GObject *obj)
{
  BjbSearchToolbar *search_bar;
  BjbWindowBase *self = BJB_WINDOW_BASE (obj);
  GtkListBox *list_box;

  G_OBJECT_CLASS (bjb_window_base_parent_class)->constructed (obj);

  g_action_map_add_action_entries (G_ACTION_MAP (self),
                                   win_entries,
                                   G_N_ELEMENTS (win_entries),
                                   self);

  self->settings = bjb_app_get_settings ((gpointer) g_application_get_default ());

  gtk_window_set_position (GTK_WINDOW (self),GTK_WIN_POS_CENTER);
  gtk_window_set_title (GTK_WINDOW (self), _(BIJIBEN_MAIN_WIN_TITLE));

  bjb_window_base_load_geometry (self);
  gtk_window_set_default_size (GTK_WINDOW (self), self->width, self->height);

  if (self->is_maximized)
    gtk_window_maximize (GTK_WINDOW (self));
  else if (self->pos_x >= 0)
    gtk_window_move (GTK_WINDOW (self), self->pos_x, self->pos_y);

  /*  We probably want to offer a no entry window at first (startup) */
  self->entry = NULL;

  self->controller = bjb_controller_new
    (bijiben_get_manager (BJB_APPLICATION(g_application_get_default())),
     GTK_WINDOW (obj),
     self->entry );

  self->note_list = bjb_list_view_new ();
  bjb_list_view_setup (self->note_list, self->controller);

  list_box = bjb_list_view_get_list_box (self->note_list);
  g_signal_connect (list_box, "row-activated",
                    G_CALLBACK (on_note_list_row_activated), self);

  /* Search entry toolbar */
  search_bar = BJB_SEARCH_TOOLBAR (self->search_bar);
  bjb_search_toolbar_setup (search_bar, GTK_WIDGET(obj), self->controller);

  self->spinner = gtk_spinner_new ();
  gtk_stack_add_named (self->main_stack, self->spinner, "spinner");
  gtk_stack_set_visible_child_name (self->main_stack, "spinner");
  gtk_widget_show (self->spinner);
  gtk_spinner_start (GTK_SPINNER (self->spinner));

  self->no_note = bjb_empty_results_box_new ();
  gtk_stack_add_named (self->main_stack, self->no_note, "empty");

  gtk_stack_add_named (self->main_stack, GTK_WIDGET (self->note_list), "main-view");
  gtk_widget_show (GTK_WIDGET (self->main_stack));


  /* Connection to window signals */

  g_signal_connect (GTK_WIDGET (self),
                    "destroy",
                    G_CALLBACK (bjb_window_base_destroy),
                    self);

  /* Keys */

  g_signal_connect (GTK_WIDGET (self),
                    "key-press-event",
                    G_CALLBACK(on_key_pressed_cb),
                    self);

  /* If a note is requested at creation, show it
   * This is a specific type of window not associated with any view */
  if (self->note != NULL)
  {
    self->detached = TRUE;
    bjb_window_base_load_note_item (self, BIJI_ITEM (self->note));
  }
}


static void
bjb_window_base_init (BjbWindowBase *self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

static void
bjb_window_base_class_init (BjbWindowBaseClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->constructed = bjb_window_base_constructed;
  gobject_class->finalize = bjb_window_base_finalize ;
  gobject_class->get_property = bjb_window_base_get_property;
  gobject_class->set_property = bjb_window_base_set_property;

  widget_class->configure_event = bjb_window_base_configure_event;

  bjb_win_base_signals[BJB_WIN_BASE_VIEW_CHANGED] = g_signal_new ("view-changed" ,
                                                    G_OBJECT_CLASS_TYPE (klass),
                                                    G_SIGNAL_RUN_LAST,
                                                    0,
                                                    NULL,
                                                    NULL,
                                                    g_cclosure_marshal_VOID__VOID,
                                                    G_TYPE_NONE,
                                                    0);

  bjb_win_base_signals[BJB_WIN_BASE_ACTIVATED] =    g_signal_new ("activated" ,
                                                    G_OBJECT_CLASS_TYPE (klass),
                                                    G_SIGNAL_RUN_LAST,
                                                    0,
                                                    NULL,
                                                    NULL,
                                                    g_cclosure_marshal_VOID__BOOLEAN,
                                                    G_TYPE_NONE,
                                                    1,
                                                    G_TYPE_BOOLEAN);

  properties[PROP_NOTE] = g_param_spec_object ("note",
                                               "NoteObj",
                                               "Currently opened note",
                                               BIJI_TYPE_NOTE_OBJ,
                                               G_PARAM_READWRITE |
                                               G_PARAM_CONSTRUCT |
                                               G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, NUM_PROPERTIES, properties);

  gtk_widget_class_set_template_from_resource (widget_class, "/org/gnome/Notes/ui/bjb-window-base.ui");
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, main_leaflet);
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, header_group);
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, back_revealer);
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, main_stack);
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, back_button);
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, headerbar);
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, note_box);
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, note_headerbar);
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, sidebar_box);
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, search_bar);
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, title_entry);
  gtk_widget_class_bind_template_child (widget_class, BjbWindowBase, last_update_item);
  gtk_widget_class_bind_template_callback (widget_class, notify_header_visible_child_cb);
  gtk_widget_class_bind_template_callback (widget_class, notify_fold_cb);
  gtk_widget_class_bind_template_callback (widget_class, on_back_button_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_new_note_clicked);
  gtk_widget_class_bind_template_callback (widget_class, on_title_changed);
}


GtkWindow *
bjb_window_base_new                    (BijiNoteObj *note)
{
  return g_object_new (BJB_TYPE_WINDOW_BASE,
                       "application", g_application_get_default(),
                       "note", note,
                       NULL);
}


BjbController *
bjb_window_base_get_controller (BjbWindowBase *self)
{
  return self->controller;
}

BijiNoteObj *
bjb_window_base_get_note (BjbWindowBase *self)
{
  g_return_val_if_fail (BJB_IS_WINDOW_BASE (self), NULL);

  return self->note;
}



static void
destroy_note_if_needed (BjbWindowBase *self)
{
  self->note = NULL;

  if (self->note_view && GTK_IS_WIDGET (self->note_view))
    gtk_widget_destroy (GTK_WIDGET (self->note_view));

  gtk_widget_hide (self->title_entry);

  self->note_view = NULL;
}

void
bjb_window_base_switch_to (BjbWindowBase *self, BjbWindowViewType type)
{
  destroy_note_if_needed (self);

  switch (type)
  {

    /* Precise the window does not display any specific note
     * Refresh the model
     * Ensure the main view receives the proper signals
     *
     * main view & archive view are the same widget
     */

    case BJB_WINDOW_BASE_MAIN_VIEW:
      switch_to_sidebar (self);
      gtk_widget_show (GTK_WIDGET (self->search_bar));
      gtk_stack_set_visible_child_name (self->main_stack, "main-view");
      break;

   case BJB_WINDOW_BASE_ARCHIVE_VIEW:
      gtk_widget_show (GTK_WIDGET (self->search_bar));
      gtk_stack_set_visible_child_name (self->main_stack, "main-view");
      break;

    case BJB_WINDOW_BASE_SPINNER_VIEW:
      gtk_widget_hide (GTK_WIDGET (self->search_bar));
      gtk_stack_set_visible_child_name (self->main_stack, "spinner");
      break;


    case BJB_WINDOW_BASE_NO_NOTE:
      bjb_empty_results_box_set_type (BJB_EMPTY_RESULTS_BOX (self->no_note),
                                      BJB_EMPTY_RESULTS_NO_NOTE);
      gtk_widget_show (self->no_note);
      gtk_widget_hide (GTK_WIDGET (self->search_bar));
      gtk_stack_set_visible_child_name (self->main_stack, "empty");
      break;


    case BJB_WINDOW_BASE_NO_RESULT:
      bjb_empty_results_box_set_type (BJB_EMPTY_RESULTS_BOX (self->no_note),
                                      BJB_EMPTY_RESULTS_NO_RESULTS);
      gtk_widget_show (self->no_note);
      gtk_stack_set_visible_child_name (self->main_stack, "empty");
      break;


    case BJB_WINDOW_BASE_ERROR_TRACKER:
      bjb_empty_results_box_set_type (BJB_EMPTY_RESULTS_BOX (self->no_note),
                                      BJB_EMPTY_RESULTS_TRACKER);
      gtk_widget_show_all (self->no_note);
      gtk_widget_hide (GTK_WIDGET (self->search_bar));
      gtk_stack_set_visible_child_name (self->stack, "empty");
      break;


    case BJB_WINDOW_BASE_NOTE_VIEW:
      break;


    case BJB_WINDOW_BASE_NO_VIEW:
    default:
      return;
  }

  self->current_view = type;

  g_signal_emit (G_OBJECT (self), bjb_win_base_signals[BJB_WIN_BASE_VIEW_CHANGED],0);
}

static void
on_last_updated_cb (BijiItem      *note,
                    BjbWindowBase *self)
{
  g_autofree char *label = NULL;
  g_autofree char *time_str = NULL;

  time_str = biji_note_obj_get_last_change_date_string (self->note);
  /* Translators: %s is the note last recency description.
   * Last updated is placed as in left to right language
   * right to left languages might move %s
   *         '%s Last Updated'
   */
  label = g_strdup_printf (_("Last updated: %s"), time_str);
  gtk_label_set_text (GTK_LABEL (self->last_update_item), label);
}

static void
populate_headerbar_for_note_view (BjbWindowBase *self)
{
  on_note_renamed (BIJI_ITEM (self->note), self);
  g_signal_connect (self->note, "renamed", G_CALLBACK (on_note_renamed), self);

  on_last_updated_cb (BIJI_ITEM (self->note), self);
  g_signal_connect (self->note, "changed", G_CALLBACK (on_last_updated_cb), self);
}

void
bjb_window_base_load_note_item (BjbWindowBase *self, BijiItem *item)
{
  GtkWidget *w = GTK_WIDGET (self);

  destroy_note_if_needed (self);

  if (BIJI_IS_NOTE_OBJ (item))
  {
    BijiNoteObj *note = BIJI_NOTE_OBJ (item);

    self->note = note;
    self->note_view = bjb_note_view_new (w, note);
    gtk_box_pack_end (GTK_BOX (self->note_box), GTK_WIDGET (self->note_view), TRUE, TRUE, 0);
    gtk_widget_show (GTK_WIDGET (self->note_view));
    bjb_note_view_grab_focus (self->note_view);

    populate_headerbar_for_note_view (self);
  }
}

BjbWindowViewType
bjb_window_base_get_view_type (BjbWindowBase *self)
{
  return self->current_view;
}

BijiManager *
bjb_window_base_get_manager(GtkWidget * win)
{
  return bijiben_get_manager (BJB_APPLICATION (g_application_get_default()));
}

void
bjb_window_base_set_entry(GtkWidget *win, gchar *search_entry)
{
  BjbWindowBase *self;

  g_return_if_fail (BJB_IS_WINDOW_BASE (win));

  self = BJB_WINDOW_BASE (win);
  self->entry = search_entry;
}


gchar *
bjb_window_base_get_entry(GtkWidget *win)
{
  BjbWindowBase *self = BJB_WINDOW_BASE(win);
  return self->entry;
}

GtkWidget *
bjb_window_base_get_search_bar (BjbWindowBase *self)
{
  return GTK_WIDGET (self->search_bar);
}

gboolean
bjb_window_base_get_show_search_bar (BjbWindowBase *self)
{

  /* There is no search bar at startup,
   * when main toolbar is first built... */
  if (!self->search_bar)
    return FALSE;

  return gtk_search_bar_get_search_mode (
            GTK_SEARCH_BAR (self->search_bar));
}

void
bjb_window_base_set_active (BjbWindowBase *self, gboolean active)
{
  gboolean available;

  available = (self->current_view != BJB_WINDOW_BASE_NOTE_VIEW);

  if (active == TRUE)
  {
    g_signal_emit (self,
                   bjb_win_base_signals[BJB_WIN_BASE_ACTIVATED],
                   0,
                   available);
  }
}


gboolean
bjb_window_base_is_detached (BjbWindowBase *self)
{
  return self->detached;
}
