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

#include "blacklist.h"
#include <gmpc/plugin.h>

static GStaticMutex m_mutex = G_STATIC_MUTEX_INIT;
gboolean m_blacklist_enabled = FALSE;
gboolean m_reload_blacklist = FALSE;
GSList* m_blacklist_genre = NULL;
GSList* m_blacklist_artist = NULL;

void set_active_blacklist(gboolean l_value)
{
	m_blacklist_enabled = l_value;
	if(m_blacklist_enabled)
		reload_blacklists();
}

gboolean get_active_blacklist()
{
	return m_blacklist_enabled;
}

gboolean is_blacklisted(const mpd_Song* l_song)
{
	g_assert(l_song != NULL);

	if(!m_blacklist_enabled)
		return FALSE;

	return is_blacklisted_genre(l_song->genre) || is_blacklisted_artist(l_song->artist);
}

gboolean is_blacklisted_slist(GSList* l_list, const gchar* l_value)
{
	g_assert(l_value != NULL);

	if(!m_blacklist_enabled)
		return FALSE;

	check_for_reload();
	if(l_list != NULL)
	{
		const GQuark value = g_quark_try_string(l_value);
		if(value != 0)
		{
			const GSList* iter;
			for(iter = l_list; iter != NULL; iter = g_slist_next(iter))
			{
				if( GPOINTER_TO_UINT(iter->data) == value )
				{
					g_debug("Blacklisted: %s", l_value);
					return TRUE;
				}
			}
		}
	}

	return FALSE;
}

gboolean is_blacklisted_genre(const gchar* l_genre)
{
	return is_blacklisted_slist(m_blacklist_genre, l_genre);
}

gboolean is_blacklisted_artist(const gchar* l_artist)
{
	return is_blacklisted_slist(m_blacklist_artist, l_artist);
}

void check_for_reload()
{
	g_static_mutex_lock(&m_mutex);
	if(m_reload_blacklist)
	{
		load_blacklists();
		m_reload_blacklist = FALSE;
	}
	g_static_mutex_unlock(&m_mutex);
}

void reload_blacklists()
{
	/* Needs a lock because it is possible that "findSimilar"
	 * refreshes blacklist at the moment.
	 */
	g_static_mutex_lock(&m_mutex);
	m_reload_blacklist = TRUE;
	g_static_mutex_unlock(&m_mutex);
}

void load_blacklists()
{
	free_blacklists();
	load_blacklist_genre();
	load_blacklist_artist();
}

void load_blacklist_genre()
{
	g_assert(m_blacklist_genre == NULL);

	MpdData* data = mpd_database_get_playlist_content(connection, BLACKLIST_GENRE);
	for(; data != NULL; data = mpd_data_get_next(data))
	{
		g_assert(data->type == MPD_DATA_TYPE_SONG);

		GQuark value = g_quark_from_string(data->song->genre);
		g_debug("Add genre to blacklist: %s", data->song->genre);
		m_blacklist_genre = g_slist_prepend(m_blacklist_genre, GUINT_TO_POINTER(value));
	}
}

void load_blacklist_artist()
{
	g_assert(m_blacklist_artist == NULL);

	MpdData* data = mpd_database_get_playlist_content(connection, BLACKLIST_ARTIST);
	for(; data != NULL; data = mpd_data_get_next(data))
	{
		g_assert(data->type == MPD_DATA_TYPE_SONG);

		GQuark value = g_quark_from_string(data->song->artist);
		g_debug("Add artist to blacklist: %s", data->song->artist);
		m_blacklist_artist = g_slist_prepend(m_blacklist_artist, GUINT_TO_POINTER(value));
	}
}

void free_blacklists()
{
	g_slist_free(m_blacklist_genre);
	m_blacklist_genre = NULL;

	g_slist_free(m_blacklist_artist);
	m_blacklist_artist = NULL;
}

/* vim:set ts=4 sw=4: */
