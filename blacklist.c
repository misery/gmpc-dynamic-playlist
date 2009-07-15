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
#include <glib/gi18n-lib.h>

#define BLACKLIST_COUNT 4
typedef enum _blacklist_name
{
	GENRE,
	ARTIST,
	ALBUM,
	SONG
} blacklist_name;

static const gchar* const blacklist[] =
{
	[GENRE]  = N_("!Blacklist Genre"),
	[ARTIST] = N_("!Blacklist Artist"),
	[ALBUM]  = N_("!Blacklist Album"),
	[SONG]   = N_("!Blacklist Song")
};

typedef struct
{
	GQuark artist;
	GQuark name; /* Album or Title */
} tuple;

gboolean m_blacklist_enabled = FALSE;
gboolean m_reload_blacklist = FALSE;
GSList* m_blacklist_genre = NULL;
GSList* m_blacklist_artist = NULL;
GSList* m_blacklist_album = NULL;
GSList* m_blacklist_song = NULL;

void set_active_blacklist(gboolean l_value)
{
	if(!m_blacklist_enabled && l_value)
		reload_blacklists();

	m_blacklist_enabled = l_value;
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

	return is_blacklisted_genre(l_song->genre)
			|| is_blacklisted_artist(l_song->artist)
			|| is_blacklisted_album(l_song->albumartist == NULL ? l_song->artist : l_song->albumartist, l_song->album)
			|| is_blacklisted_song(l_song->artist, l_song->title);
}

gboolean is_blacklisted_single(const GSList* l_list, const gchar* l_value)
{
	if(l_value == NULL || !m_blacklist_enabled)
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

gboolean is_blacklisted_tuple(const GSList* l_list, const gchar* l_artist, const gchar* l_name)
{
	if(l_artist == NULL || l_name == NULL || !m_blacklist_enabled)
		return FALSE;

	check_for_reload();
	if(l_list != NULL)
	{
		const GQuark artist = g_quark_try_string(l_artist);
		if(artist != 0)
		{
			const GQuark name = g_quark_try_string(l_name);
			if(name != 0)
			{
				const GSList* iter;
				for(iter = l_list; iter != NULL; iter = g_slist_next(iter))
				{
					const tuple* const elem = iter->data;
					if(elem->artist == artist && elem->name == name)
					{
						g_debug("Blacklisted: %s::%s", l_artist, l_name);
						return TRUE;
					}
				}
			}
		}
	}

	return FALSE;
}

gboolean is_blacklisted_genre(const gchar* l_genre)
{
	return is_blacklisted_single(m_blacklist_genre, l_genre);
}

gboolean is_blacklisted_artist(const gchar* l_artist)
{
	return is_blacklisted_single(m_blacklist_artist, l_artist);
}

gboolean is_blacklisted_album(const gchar* l_artist, const gchar* l_album)
{
	return is_blacklisted_tuple(m_blacklist_album, l_artist, l_album);
}

gboolean is_blacklisted_song(const gchar* l_artist, const gchar* l_title)
{
	return is_blacklisted_tuple(m_blacklist_song, l_artist, l_title);
}

void create_blacklists()
{
	MpdData* lists = mpd_database_playlist_list(connection);
	gint8 i;
	for(i = 0; i < BLACKLIST_COUNT; ++i)
	{
		if(lists == NULL || !create_blacklists_search(&lists, _(blacklist[i])))
			mpd_database_playlist_clear(connection, _(blacklist[i]));
	}

	if(lists != NULL)
		mpd_data_free(lists);
}

gboolean create_blacklists_search(MpdData** l_out_lists, const gchar* l_blacklist)
{
	g_assert(l_out_lists != NULL && *l_out_lists != NULL && l_blacklist != NULL);

	MpdData* iter = *l_out_lists;
	for(; iter != NULL; iter = mpd_data_get_next_real(iter, FALSE))
	{
		if(strcmp(iter->playlist->path, l_blacklist) == 0)
		{
			iter = mpd_data_delete_item(iter);
			if(iter == NULL || ((MpdData_real*) iter)->prev == NULL)
				*l_out_lists = iter;
			return TRUE;
		}
	}

	return FALSE;
}

void check_for_reload()
{
	if(m_reload_blacklist)
	{
		load_blacklists();
		create_blacklists();
		m_reload_blacklist = FALSE;
	}
}

void reload_blacklists()
{
	m_reload_blacklist = TRUE;
}

void load_blacklists()
{
	free_blacklists();
	load_blacklist_genre();
	load_blacklist_artist();
	load_blacklist_album();
	load_blacklist_song();
}

void load_blacklist_genre()
{
	g_assert(m_blacklist_genre == NULL);

	MpdData* data = mpd_database_get_playlist_content(connection, _(blacklist[GENRE]));
	for(; data != NULL; data = mpd_data_get_next(data))
	{
		g_assert(data->type == MPD_DATA_TYPE_SONG);

		const GQuark value = g_quark_from_string(data->song->genre);
		if(value != 0)
		{
			g_debug("Add genre to blacklist: %s", data->song->genre);
			m_blacklist_genre = g_slist_prepend(m_blacklist_genre, GUINT_TO_POINTER(value));
		}
	}
}

void load_blacklist_artist()
{
	g_assert(m_blacklist_artist == NULL);

	MpdData* data = mpd_database_get_playlist_content(connection, _(blacklist[ARTIST]));
	for(; data != NULL; data = mpd_data_get_next(data))
	{
		g_assert(data->type == MPD_DATA_TYPE_SONG);

		const GQuark value = g_quark_from_string(data->song->artist);
		if(value != 0)
		{
			g_debug("Add artist to blacklist: %s", data->song->artist);
			m_blacklist_artist = g_slist_prepend(m_blacklist_artist, GUINT_TO_POINTER(value));
		}
	}
}

void load_blacklist_album()
{
	g_assert(m_blacklist_album == NULL);

	MpdData* data = mpd_database_get_playlist_content(connection, _(blacklist[ALBUM]));
	for(; data != NULL; data = mpd_data_get_next(data))
	{
		g_assert(data->type == MPD_DATA_TYPE_SONG);

		const gchar* const artistChar = data->song->albumartist == NULL ? data->song->artist : data->song->albumartist;
		const GQuark artist = g_quark_from_string(artistChar);
		if(artist != 0)
		{
			const GQuark album = g_quark_from_string(data->song->album);
			if(album != 0)
			{
				tuple* tmp = g_slice_new(tuple);
				tmp->artist = artist;
				tmp->name = album;
				g_debug("Add album to blacklist: %s::%s", artistChar, data->song->album);
				m_blacklist_album = g_slist_prepend(m_blacklist_album, tmp);
			}
		}
	}
}

void load_blacklist_song()
{
	g_assert(m_blacklist_song == NULL);

	MpdData* data = mpd_database_get_playlist_content(connection, _(blacklist[SONG]));
	for(; data != NULL; data = mpd_data_get_next(data))
	{
		g_assert(data->type == MPD_DATA_TYPE_SONG);

		const GQuark artist = g_quark_from_string(data->song->artist);
		if(artist != 0)
		{
			const GQuark title = g_quark_from_string(data->song->title);
			if(title != 0)
			{
				tuple* tmp = g_slice_new(tuple);
				tmp->artist = artist;
				tmp->name = title;
				g_debug("Add song to blacklist: %s::%s", data->song->artist, data->song->title);
				m_blacklist_song = g_slist_prepend(m_blacklist_song, tmp);
			}
		}
	}
}

void free_blacklists()
{
	g_slist_free(m_blacklist_genre);
	m_blacklist_genre = NULL;

	g_slist_free(m_blacklist_artist);
	m_blacklist_artist = NULL;

	free_blacklists_tuple(m_blacklist_album);
	g_slist_free(m_blacklist_album);
	m_blacklist_album = NULL;

	free_blacklists_tuple(m_blacklist_song);
	g_slist_free(m_blacklist_song);
	m_blacklist_song = NULL;
}

void free_blacklists_tuple(GSList* l_list)
{
	GSList* iter;
	for(iter = l_list; iter != NULL; iter = g_slist_next(iter))
	{
		g_slice_free(tuple, iter->data);
	}
}

/* vim:set ts=4 sw=4: */
