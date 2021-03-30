/* bjb-memo-note.c
 * Copyright (C) Pierre-Yves LUYTEN 2014 <py@luyten.fr>
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

#include "biji-memo-provider.h"
#include "biji-memo-note.h"

struct _BijiMemoNote
{
  BijiNoteObj    parent_instance;
  BijiProvider  *provider;
  ECalComponent *ecal;
  ECalClient    *client;
  const char    *description;
};

G_DEFINE_TYPE (BijiMemoNote, biji_memo_note, BIJI_TYPE_NOTE_OBJ)

/* Properties */
enum {
  PROP_0,
  PROP_ECAL,
  MEMO_NOTE_PROP
};

static GParamSpec *properties[MEMO_NOTE_PROP] = { NULL, };

/* Function from evo calendar gui comp-util.c (LGPL)
 * to be removed if we depen on evo,
 * the func is borrowed as is (=3.13.1) */
static gboolean
cal_comp_is_on_server (ECalComponent *comp,
                       ECalClient *client)
{
  const char *uid;
  g_autofree char *rid = NULL;
  ICalComponent *icalcomp = NULL;
  g_autoptr(GError) error = NULL;

  g_return_val_if_fail (comp != NULL, FALSE);
  g_return_val_if_fail (E_IS_CAL_COMPONENT (comp), FALSE);
  g_return_val_if_fail (client != NULL, FALSE);
  g_return_val_if_fail (E_IS_CAL_CLIENT (client), FALSE);

  /* See if the component is on the server.  If it is not, then it likely
   * means that the appointment is new, only in the day view, and we
   * haven't added it yet to the server.	In that case, we don't need to
   * confirm and we can just delete the event.  Otherwise, we ask
   * the user.
   */
  uid = e_cal_component_get_uid (comp);

  /* TODO We should not be checking for this here. But since
   *	e_cal_util_construct_instance does not create the instances
   *	of all day events, so we default to old behaviour. */
  if (e_cal_client_check_recurrences_no_master (client))
    {
      rid = e_cal_component_get_recurid_as_string (comp);
    }

  e_cal_client_get_object_sync (client, uid, rid, &icalcomp, NULL, &error);

  if (icalcomp != NULL)
    {
      g_clear_object (&icalcomp);

      return TRUE;
    }

  if (!g_error_matches (error, E_CAL_CLIENT_ERROR, E_CAL_CLIENT_ERROR_OBJECT_NOT_FOUND))
    g_warning (G_STRLOC ": %s", error->message);

  return FALSE;
}

/*
 * Parse current note content to update ECalComponent
 *
 * "clone" properties we do not change remain as "comp"
 * This is true for attachments, too.
 *
 * Comments refer to where evolution 3.13.1 does the same.
 */
static void
fill_in_components (ECalComponent *comp,
                    ECalComponent *clone,
                    BijiMemoNote  *self)
{
  ECalComponentText *text;
  GSList             l;
  glong              mtime;
  ICalTime          *t;


  /* ----------------- FIELDS FROM "memo_page_fill_components"------------------ */


  /* Set : title */
  text = e_cal_component_text_new (biji_item_get_title (BIJI_ITEM (self)), NULL);
  e_cal_component_set_summary (clone, text);
  e_cal_component_text_free (text);

  /* Set : content */
  text = e_cal_component_text_new (biji_note_obj_get_raw_text (BIJI_NOTE_OBJ (self)), NULL);
  l.data = text;
  l.next = NULL;
  e_cal_component_set_descriptions (clone, &l);
  e_cal_component_text_free (text);


  /* dtstart : we'd rather use "created", "modified" */

  /* Classification :? */


  /* Categories : to be implemented */

  /* Recipients : do not touch this */
  /* Organizer  : do not touch this */


  /* -------------  FIELDS FROM "save_comp" in comp-editor.c ------- */

  /* Attachment list */


  /* rdate, rrule, exdate, exrule */

  /* Sequence */


  /* X-EVOLUTION-OPTIONS-DELAY */


  /* -------------- OHER FILEDS (from specification) -------
   *    FIXME: some of theme belong somewhere above....
   *
   * Unique: created / description / dtstamp / recurid / status / url /
   * Several: attach / attendee / comment / contact / related / rstatus / x-prop
  */

  mtime = biji_item_get_mtime (BIJI_ITEM (self));
  t = icaltime_from_time_val (mtime);
  if (t)
    {
      e_cal_component_set_last_modified (clone, t);
      g_object_unref (t);
    }
}

/*
 * https://git.gnome.org/browse/evolution/tree/calendar/gui/dialogs/comp-editor.c#n471
 */
static void
memo_note_save (BijiNoteObj *note)
{
  BijiMemoNote *self = BIJI_MEMO_NOTE (note);
  ICalComponent *icalcomp;
  gboolean result;
  g_autoptr(GError) error = NULL;
  ECalComponent *clone;

  clone = e_cal_component_clone (self->ecal);
  fill_in_components (self->ecal, clone, self);

  /* Save */
  e_cal_component_commit_sequence (clone);
  g_object_unref (self->ecal);
  self->ecal = clone;

  icalcomp = e_cal_component_get_icalcomponent (self->ecal);


  if (!cal_comp_is_on_server (self->ecal, self->client))
    {
      g_autofree char *uid = NULL;
      result = e_cal_client_create_object_sync (self->client,
                                                icalcomp,
                                                E_CAL_OPERATION_FLAG_NONE,
                                                &uid,
                                                NULL,
                                                &error);
      if (result)
        i_cal_component_set_uid (icalcomp, uid);
    }
  else
    {
      result = e_cal_client_modify_object_sync (self->client,
                                                icalcomp,
                                                E_CAL_OBJ_MOD_THIS,
                                                E_CAL_OPERATION_FLAG_NONE,
                                                NULL,
                                                &error);
      e_cal_component_commit_sequence (clone);
    }
}

static void
biji_memo_note_init (BijiMemoNote *biji_memo_note)
{
}

/* Let the provider finalize the ECalComponent. */
static void
biji_memo_note_finalize (GObject *object)
{
  G_OBJECT_CLASS (biji_memo_note_parent_class)->finalize (object);
}

static void
biji_memo_note_set_property (GObject      *object,
                             guint         property_id,
                             const GValue *value,
                             GParamSpec   *pspec)
{
  BijiMemoNote *self = BIJI_MEMO_NOTE (object);

  switch (property_id)
    {
    case PROP_ECAL:
      self->ecal = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
biji_memo_note_get_property (GObject    *object,
                             guint       property_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  BijiMemoNote *self = BIJI_MEMO_NOTE (object);

  switch (property_id)
    {
    case PROP_ECAL:
      g_value_set_object (value, self->ecal);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
memo_set_html (BijiNoteObj *note,
               const char  *html)
{
  /* NULL */
}

static gboolean
memo_item_delete (BijiItem *item)
{
  g_return_val_if_fail (BIJI_IS_NOTE_OBJ (item), FALSE);

  g_warning ("Memo note delete is not yet implemented");
  return FALSE;
}

static void
on_memo_deleted (GObject      *ecal,
                 GAsyncResult *res,
                 gpointer      user_data)
{
  g_autoptr(GError) error = NULL;

  e_cal_client_remove_object_finish (E_CAL_CLIENT (ecal), res, &error);

  if (error)
    g_warning ("Could not delete memo:%s", error->message);
}

static gboolean
memo_delete (BijiNoteObj *note)
{
  BijiMemoNote *self = BIJI_MEMO_NOTE (note);
  const char *uid = e_cal_component_get_uid (self->ecal);

  e_cal_client_remove_object (self->client,
                              uid,
                              NULL,               /* rid : all occurences */
                              E_CAL_OBJ_MOD_ALL,  /*       all occurences */
                              E_CAL_OPERATION_FLAG_NONE,
                              NULL,               /* Cancellable */
                              on_memo_deleted,
                              self);

  return TRUE;
}

static char *
memo_get_html (BijiNoteObj *note)
{
  // we cast but the func should expect a const char, really
  return html_from_plain_text ((char*) biji_note_obj_get_raw_text (note));
}

static char *
memo_get_basename (BijiNoteObj *note)
{
  BijiMemoNote *self = BIJI_MEMO_NOTE (note);
  const char *out = e_cal_component_get_uid (self->ecal);

  return g_strdup (out);
}

static const char *
memo_get_place (BijiItem *item)
{
  BijiMemoNote *self = BIJI_MEMO_NOTE (item);
  const BijiProviderInfo *info = biji_provider_get_info (BIJI_PROVIDER (self->provider));

  return info->name;
}

static gboolean
note_no (BijiNoteObj *note)
{
  return FALSE;
}

static void
biji_memo_note_class_init (BijiMemoNoteClass *klass)
{
  GObjectClass     *object_class = G_OBJECT_CLASS (klass);
  BijiItemClass    *item_class;
  BijiNoteObjClass *note_class;

  item_class = BIJI_ITEM_CLASS (klass);
  note_class = BIJI_NOTE_OBJ_CLASS (klass);

  object_class->finalize = biji_memo_note_finalize;
  object_class->get_property = biji_memo_note_get_property;
  object_class->set_property = biji_memo_note_set_property;

  item_class->get_place = memo_get_place;
  item_class->delete = memo_item_delete;

  note_class->get_basename = memo_get_basename;
  note_class->get_html = memo_get_html;
  note_class->set_html = memo_set_html;
  note_class->save_note = memo_note_save;
  note_class->can_format = note_no;
  note_class->archive = memo_delete;
  note_class->is_trashed = note_no;

  properties[PROP_ECAL] =
    g_param_spec_object("ecal",
                        "ECalComponent",
                        "ECalComponent which this note refers to",
                        E_TYPE_CAL_COMPONENT,
                        G_PARAM_READWRITE | G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (object_class, MEMO_NOTE_PROP, properties);
}

/*
 * The provider looks for a content ("description")
 * in order to push to tracker.
 *
 */
BijiNoteObj *
biji_memo_note_new_from_info (BijiMemoProvider *provider,
                              BijiManager      *manager,
                              BijiInfoSet      *info,
                              ECalComponent    *component,
                              const char       *description,
                              ECalClient       *client)
{
  BijiMemoNote *ret = g_object_new (BIJI_TYPE_MEMO_NOTE,
                                    "manager", manager,
                                    "path",    info->url,
                                    "title",   info->title,
                                    "mtime",   info->mtime,
                                    "content", info->content,
                                    "ecal",    component,
                                    NULL);

  ret->provider = BIJI_PROVIDER (provider);
  ret->description = description;
  ret->client = client;
  return BIJI_NOTE_OBJ (ret);
}

