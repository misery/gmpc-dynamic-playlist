/* gmpc-dynamic-playlist (GMPC plugin)
 * Copyright (C) 2009 Andre Klitzing <andre@incubo.de>
 * Homepage: http://www.incubo.de

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "prune.h"
#include "plugin.h"
#include <gmpc/playlist3-messages.h>
#include <libmpd/libmpd.h>
#include <glib/gi18n-lib.h>

static gint m_keep = -1;

void init_prune()
{
	m_keep = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "keep", -1);
}

void set_prune_value(gint l_value)
{
	m_keep = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "keep", m_keep);
}

gint get_prune_value()
{
	return m_keep;
}

void prune_playlist(gint l_curPos)
{
	prune_playlist_value(l_curPos, m_keep);
}

void prune_playlist_value(gint l_curPos, gint l_keep)
{
	if(l_keep < 0 || l_curPos < 1)
		return;

	gint del;
	for(del = 0; del < l_curPos - l_keep; ++del)
		mpd_playlist_queue_delete_pos(connection, 0);

	mpd_playlist_queue_commit(connection);
}

void prune_playlist_easy(G_GNUC_UNUSED gpointer l_data, const gchar* l_param)
{
	g_assert(l_param != NULL);

	if(!dyn_get_enabled())
	{
		playlist3_show_error_message(_("Dynamic playlist is disabled"), ERROR_INFO);
		return;
	}

	mpd_Song* curSong = mpd_playlist_get_current_song(connection);
	if(curSong == NULL)
	{
		playlist3_show_error_message(_("Cannot prune playlist! You need to play a song for pruning."), ERROR_INFO);
		return;
	}

	if(l_param[0] == '\0')
		prune_playlist(curSong->pos);
	else
		prune_playlist_value(curSong->pos, atoi(l_param));
}

/* vim:set ts=4 sw=4: */
