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

struct _BijiMemoNotePrivate
{
  BijiProvider  *provider;
  ECalComponent *ecal;
  ECalClient    *client;
  gchar         *description;
  BijiNoteID    *id;
};


G_DEFINE_TYPE (BijiMemoNote, biji_memo_note, BIJI_TYPE_NOTE_OBJ);


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
gboolean
cal_comp_is_on_server (ECalComponent *comp,
                       ECalClient *client)
{
  const gchar *uid;
  gchar *rid = NULL;
  icalcomponent *icalcomp = NULL;
  GError *error = NULL;

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
  e_cal_component_get_uid (comp, &uid);

  /* TODO We should not be checking for this here. But since
   *	e_cal_util_construct_instance does not create the instances
   *	of all day events, so we default to old behaviour. */
  if (e_cal_client_check_recurrences_no_master (client))
  {
    rid = e_cal_component_get_recurid_as_string (comp);
  }

  e_cal_client_get_object_sync (
   	client, uid, rid, &icalcomp, NULL, &error);

  if (icalcomp != NULL)
  {
    icalcomponent_free (icalcomp);
    g_free (rid);

    return TRUE;
  }

  if (!g_error_matches (error, E_CAL_CLIENT_ERROR, E_CAL_CLIENT_ERROR_OBJECT_NOT_FOUND))
    g_warning (G_STRLOC ": %s", error->message);

  g_clear_error (&error);
  g_free (rid);

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
  ECalComponentText  text;
  GSList             l;
  glong              mtime;
  icaltimetype       t;


  /* ----------------- FIELDS FROM "memo_page_fill_components"------------------ */


  /* Set : title */
  text.value = biji_item_get_title (BIJI_ITEM (self));
  text.altrep = NULL;
  e_cal_component_set_summary (clone, &text);

  /* Set : content */
  text.value = biji_note_obj_get_raw_text (BIJI_NOTE_OBJ (self));
  text.altrep = NULL;
  l.data = &text;
  l.next = NULL;
  e_cal_component_set_description_list (clone, &l);


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
  if (icaltime_from_time_val (mtime, &t))
    e_cal_component_set_last_modified (clone, &t);
}



/*
 * https://git.gnome.org/browse/evolution/tree/calendar/gui/dialogs/comp-editor.c#n471
 */
static void
memo_note_save (BijiNoteObj *note)
{
  BijiMemoNote *self = BIJI_MEMO_NOTE (note);
  BijiMemoNotePrivate *priv = self->priv;
  const gchar *orig_uid;
  icalcomponent *icalcomp;
  gboolean result;
  GError *error;
  ECalComponent *clone;
  //gchar *orig_uid_copy;

  clone = e_cal_component_clone (priv->ecal);
  fill_in_components (priv->ecal, clone, self);

  /* Save */
  e_cal_component_commit_sequence (clone);
  g_object_unref (priv->ecal);
  priv->ecal = clone;

  e_cal_component_get_uid (priv->ecal, &orig_uid);

  /* Make a copy of it, because call of e_cal_create_object()
   * rewrites the internal uid.
   orig_uid_copy = g_strdup (orig_uid); */
  icalcomp = e_cal_component_get_icalcomponent (priv->ecal);


  if (!cal_comp_is_on_server (priv->ecal, priv->client))
  {
    gchar *uid = NULL;
    result = e_cal_client_create_object_sync (
    priv->client, icalcomp, &uid, NULL, &error);
      if (result)
      {
        icalcomponent_set_uid (icalcomp, uid);
	g_free (uid);
        //g_signal_emit_by_name (editor, "object_created");
      }
   }

  else
  {
    result = e_cal_client_modify_object_sync (
      priv->client, icalcomp, E_CAL_OBJ_MOD_THIS, NULL, &error);
    e_cal_component_commit_sequence (clone);
  }
}


/* Save title when saving note.
 * No need to do anything here */
static void
on_title_changed_cb (BijiMemoNote *self)
{
}



static void
biji_memo_note_constructed (GObject *obj)
{
  BijiMemoNote *self = BIJI_MEMO_NOTE (obj);

  G_OBJECT_CLASS (biji_memo_note_parent_class)->constructed (obj);

  g_signal_connect_swapped (self->priv->id, "notify::title",
                            G_CALLBACK (on_title_changed_cb), self);
}


static void
biji_memo_note_init (BijiMemoNote *biji_memo_note)
{
  biji_memo_note->priv = G_TYPE_INSTANCE_GET_PRIVATE (biji_memo_note, BIJI_TYPE_MEMO_NOTE, BijiMemoNotePrivate);

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
      self->priv->ecal = g_value_dup_object (value);
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
      g_value_set_object (value, self->priv->ecal);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}



static void
memo_set_html (BijiNoteObj *note, gchar *html)
{
  /* NULL */
}




static void
on_memo_deleted (GObject *ecal,
                 GAsyncResult *res,
                 gpointer user_data)
{
  GError *error = NULL;

  e_cal_client_remove_object_finish (E_CAL_CLIENT (ecal),
                                     res, &error);

  if (error)
  {
    g_warning ("Could not delete memo:%s", error->message);
  }
}


static gboolean
memo_delete (BijiNoteObj *note)
{
  BijiMemoNote *self;
  const gchar *uid;

  self = BIJI_MEMO_NOTE (note);
  e_cal_component_get_uid (self->priv->ecal, &uid);
  e_cal_client_remove_object (self->priv->client,
                              uid,
                              NULL,               /* rid : all occurences */
                              E_CAL_OBJ_MOD_ALL,  /*       all occurences */
                              NULL,               /* Cancellable */
                              on_memo_deleted,
                              self);


  return TRUE;
}


static gchar *
memo_get_html (BijiNoteObj *note)
{
  // we cast but the func should expect a const gchar, really
  return html_from_plain_text ((gchar*) biji_note_obj_get_raw_text (note));
}



static gchar *
memo_get_basename (BijiNoteObj *note)
{
  const gchar *out;

  e_cal_component_get_uid (
    BIJI_MEMO_NOTE (note)->priv->ecal, &out);

  return g_strdup (out);
}




static const gchar *
memo_get_place (BijiItem *item)
{
  BijiMemoNote *self;
  const BijiProviderInfo *info;

  self = BIJI_MEMO_NOTE (item);
  info = biji_provider_get_info (BIJI_PROVIDER (self->priv->provider));

  return info->name;
}



static gboolean
item_no         (BijiItem * item)
{
  return FALSE;
}


static gboolean
note_no         (BijiNoteObj *item)
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

  g_type_class_add_private (klass, sizeof (BijiMemoNotePrivate));

  object_class->finalize = biji_memo_note_finalize;
  object_class->constructed = biji_memo_note_constructed;
  object_class->get_property = biji_memo_note_get_property;
  object_class->set_property = biji_memo_note_set_property;

  item_class->is_collectable = item_no;
  item_class->has_color = item_no;
  item_class->get_place = memo_get_place;

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
biji_memo_note_new_from_info          (BijiMemoProvider *provider,
                                       BijiManager      *manager,
                                       BijiInfoSet      *info,
				       ECalComponent    *component,
                                       gchar            *description,
				       ECalClient       *client)
{
  BijiNoteID *id;
  BijiMemoNote *ret;

  id = biji_note_id_new_from_info (info);

  ret = g_object_new (BIJI_TYPE_MEMO_NOTE,
                      "manager", manager,
                      "id", id,
	              "ecal", component,
                      NULL);

  ret->priv->id = id;
  ret->priv->provider = BIJI_PROVIDER (provider);
  ret->priv->description = description;
  ret->priv->client = client;
  return BIJI_NOTE_OBJ (ret);
}
