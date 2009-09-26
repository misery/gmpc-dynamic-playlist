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

#include "played.h"
#include <string.h>
#include <gmpc/plugin.h>

static gint m_song = 0;
static gint m_artist = 0;
static dbQueue m_list = G_QUEUE_INIT;

static void flush_played_list(gint l_max)
{
	g_assert(l_max >= 0);

	const gint max = g_queue_get_length(&m_list) - l_max;
	gint i;
	for(i = 0; i < max; ++i)
		free_dbSong( (dbSong*) g_queue_pop_tail(&m_list) );
}

void add_played_song(dbSong* l_song)
{
	g_assert(l_song != NULL);
	g_assert(l_song->artist != NULL);
	g_assert(l_song->title != NULL);
	g_assert(l_song->artist[0] != '\0');
	g_assert(l_song->title[0] != '\0');

	g_queue_push_head(&m_list, l_song);
	flush_played_list( MAX(m_song, m_artist) );
}

static gboolean is_played_song_current(const gchar* l_artist, const gchar* l_title)
{
	mpd_Song* curSong = mpd_playlist_get_current_song(connection);
	if(curSong != NULL && strcasecmp(curSong->artist, l_artist) == 0)
	{
		if(m_artist > 0 || (l_title != NULL && strcasecmp(curSong->title, l_title) == 0))
			return TRUE;
	}

	return FALSE;
}

gboolean is_played_song(const gchar* l_artist, const gchar* l_title)
{
	g_assert(l_artist != NULL);
	g_assert(l_artist[0] != '\0');
	if(l_title != NULL)
		g_assert(l_title[0] != '\0');

	if(is_played_song_current(l_artist, l_title))
		return TRUE;

	if(!g_queue_is_empty(&m_list))
	{
		dbList* iter = g_queue_peek_head_link(&m_list);
		gint i;
		for(i = 0; iter != NULL; iter = g_list_next(iter), ++i)
		{
			dbSong* song = (dbSong*) iter->data;
			if(i < m_artist && strcasecmp(song->artist, l_artist) == 0)
			{
				g_debug("Artist blocked: %s", l_artist);
				return TRUE;
			}
			if(i < m_song && l_title != NULL && strcasecmp(song->artist, l_artist) == 0 && strcasecmp(song->title, l_title) == 0)
			{
				g_debug("Song blocked: %s::%s", l_artist, l_title);
				return TRUE;
			}
		}
	}

	return FALSE;
}

gboolean is_played_artist(const gchar* l_artist)
{
	return is_played_song(l_artist, NULL);
}

void set_played_limit_song(gint l_song)
{
	g_assert(l_song >= 0);
	m_song = l_song;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "block", m_song);
}

void set_played_limit_artist(gint l_artist)
{
	g_assert(l_artist >= 0);
	m_artist = l_artist;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "block_artist", m_artist);
}

gint get_played_limit_song()
{
	return m_song;
}

gint get_played_limit_artist()
{
	return m_artist;
}

void init_played_list()
{
	m_song = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "block", 100);
	m_artist = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "block_artist", 0);
}

void free_played_list()
{
	if(!g_queue_is_empty(&m_list))
		clear_dbQueue(&m_list);
}

/* vim:set ts=4 sw=4: */
