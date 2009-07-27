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

#include "search.h"
#include "blacklist.h"
#include "played.h"
#include "database.h"
#include "plugin.h"
#include "prefs.h"
#include "icon.h"
#include <glib/gi18n-lib.h>

#ifdef TESTMODE
#define g_rand_int_range(obj, min, max) g_test_rand_int_range(min, max)
#endif
#define BUFFER_SECONDS 5
extern GRand* m_rand;

static void tryToAdd_select(const status l_status, mpd_Song* l_song);

static guint m_delay_source = 0;
static guint8 m_delay_timeout = 0;
static gint m_similar_songs_max = 0;
static gint m_similar_artists_max = 0;
static gint m_similar_genre_max = 0;
static gint m_similar_artist_same = TRUE;
static gint m_similar_genre_same = TRUE;
static gboolean m_similar_songs = FALSE;
static gboolean m_similar_artists = FALSE;
static gboolean m_similar_genre = FALSE;
static gboolean m_same_genre = FALSE;
static gboolean m_enabled_search = FALSE;
static gboolean m_is_searching = FALSE;

void init_search()
{
	m_delay_timeout = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "delayTimeout", 0);
	m_similar_songs_max = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "maxSongs", 20);
	m_similar_artists_max = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "maxArtists", 30);
	m_similar_genre_max = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "maxGenres", 20);
	m_similar_songs = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "similar_songs", FALSE);
	m_similar_artists = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "similar_artists", FALSE);
	m_similar_genre = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "similar_genre", FALSE);
	m_similar_artist_same = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "similar_artist_same", TRUE);
	m_similar_genre_same = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "similar_genre_same", TRUE);
	m_same_genre = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "same_genre", FALSE);
	m_enabled_search = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "similar_search", FALSE);
}

static status getNextStatus(status l_status)
{
	status ret = NotFound;

	gint available[STATUS_COUNT-1];
	gint count = 0;
	gint i;
	for(i = 1; i < STATUS_COUNT; ++i) /* index 0 is Found/NotFound */
	{
		if( !(1 << i & l_status) )
			available[count++] = i;
	}

	if(count > 0)
	{
		ret = Found;
		ret |= 1 << available[ g_rand_int_range(m_rand, 0, count) ];
	}

	return ret;
}

static void tryToAdd_artists(mpd_Song* l_song, MetaDataResult l_result, MetaData* l_data, gpointer l_last_status)
{
	if(l_result == META_DATA_FETCHING)
		return;

	status l_status = GPOINTER_TO_INT(l_last_status);
	g_assert(!(l_status & Artist));
	l_status |= Artist;
	if(l_result == META_DATA_AVAILABLE)
	{
		g_assert(l_data != NULL && l_data->type == META_ARTIST_SIMILAR);

		gint count = 0;
		strList* artistList = NULL;
		gint maxIter = 0;
		const GList* iter;
		for(iter = meta_data_get_text_list(l_data); iter != NULL && maxIter < m_similar_artists_max; iter = g_list_next(iter), ++maxIter)
		{
			const gchar* artist = (const gchar*) iter->data;
			artistList = database_get_artists(artistList, artist, NULL, &count);
		}

		if(count > 0)
		{
			// add one artist to artistList (mostly because 'same artist' is also 'similar')
			if(l_song->artist != NULL && m_similar_artist_same && get_played_limit_artist() == 0)
				artistList = database_get_artists(artistList, l_song->artist, NULL, &count);
			if(database_tryToAdd_artists(&artistList, count))
				l_status |= Found;
		}

		if(artistList != NULL)
			free_strList(artistList);
	}

	tryToAdd_select(l_status, l_song);
}

static void tryToAdd_songs(mpd_Song* l_song, MetaDataResult l_result, MetaData* l_data, gpointer l_last_status)
{
	if(l_result == META_DATA_FETCHING)
		return;

	status l_status = GPOINTER_TO_INT(l_last_status);
	g_assert(!(l_status & Song));
	l_status |= Song;
	if(l_result == META_DATA_AVAILABLE)
	{
		g_assert(l_data != NULL && l_data->type == META_SONG_SIMILAR);

		gint count = 0;
		dbList* songList = NULL;
		gint maxIter = 0;
		const GList* iter;
		for(iter = meta_data_get_text_list(l_data); iter != NULL && maxIter < m_similar_songs_max; iter = g_list_next(iter), ++maxIter)
		{
			gchar** song = g_strsplit(iter->data, "::", 2);
			if(song[0] != NULL && song[1] != NULL)
				songList = database_get_songs(songList, song[0], song[1], &count);
			g_strfreev(song);
		}

		if(count > 0)
		{
			g_assert(songList != NULL);

			gint random = g_rand_int_range(m_rand, 0, count);
			gint i = 0;
			dbList* songListIter;
			for(songListIter = songList; i < random; ++i)
				songListIter = g_list_next(songListIter);

			dbSong* song = (dbSong*) songListIter->data;
			mpd_playlist_add(connection, song->path);
			add_played_song(song);
			g_debug("Added via song | artist: %s | title: %s", song->artist, song->title);

			// Remove added dbSong* from dbList so it won't be freed
			songList = g_list_delete_link(songList, songListIter);
			if(songList != NULL)
				free_dbList(songList);
			l_status |= Found;
		}
	}

	tryToAdd_select(l_status, l_song);
}

static void tryToAdd_multiple_genre(mpd_Song* l_song, MetaDataResult l_result, MetaData* l_data, gpointer l_last_status)
{
	if(l_result == META_DATA_FETCHING)
		return;

	status l_status = (status) l_last_status;
	g_assert(!(l_status & Genre));
	l_status |= Genre;
	if(l_result == META_DATA_AVAILABLE)
	{
		g_assert(l_data != NULL && l_data->type == META_GENRE_SIMILAR);

		gint count = 0;
		strList* artistList = NULL;
		gint maxIter = 0;
		const GList* iter;
		for(iter = meta_data_get_text_list(l_data); iter != NULL && maxIter < m_similar_genre_max; iter = g_list_next(iter), ++maxIter)
		{
			const gchar* genre = (const gchar*) iter->data;
			artistList = database_get_artists(artistList, NULL, genre, &count);
		}

		// add one genre to artistList (mostly because 'same genre' is also 'similar')
		if(m_similar_genre_same)
			artistList = database_get_artists(artistList, NULL, l_song->genre, &count);

		if(count > 0 && database_tryToAdd_artists(&artistList, count))
				l_status |= Found;

		if(artistList != NULL)
			free_strList(artistList);
	}

	tryToAdd_select(l_status, l_song);
}

void tryToAdd_select(const status l_status, mpd_Song* l_song)
{
	g_assert(l_song != NULL);
	g_assert(m_is_searching);

	if(l_status & Found)
	{
		m_is_searching = FALSE;
		return;
	}

	status next = getNextStatus(l_status);
	if(next & Found)
	{
		if(next & Song)
		{
			g_debug("Try similar song... %s - %s", l_song->artist, l_song->title);
			gmpc_meta_watcher_get_meta_path_callback(gmw, l_song, META_SONG_SIMILAR, tryToAdd_songs, GINT_TO_POINTER(l_status));
		}
		else if(next & Artist)
		{
			g_debug("Try similar artist... %s", l_song->artist);
			gmpc_meta_watcher_get_meta_path_callback(gmw, l_song, META_ARTIST_SIMILAR, tryToAdd_artists, GINT_TO_POINTER(l_status));
		}
		else if(next & Genre)
		{
			g_debug("Try similar genre... %s", l_song->genre);
			gmpc_meta_watcher_get_meta_path_callback(gmw, l_song, META_GENRE_SIMILAR, tryToAdd_multiple_genre, GINT_TO_POINTER(l_status));
		}
		else
			g_assert_not_reached();
	}
	else
	{
		if(m_same_genre && !m_similar_genre && l_song->genre != NULL && !is_blacklisted_genre(l_song->genre) && tryToAdd_genre(l_song->genre))
			g_debug("Added same genre song");
		else if(tryToAdd_random())
			g_debug("Added random song");
		else
		{
			playlist3_show_error_message(_("Dynamic search cannot find a new song"), ERROR_INFO);
			g_debug("Cannot find a new song");
		}
		m_is_searching = FALSE;
	}
}

gboolean tryToAdd_genre(const gchar* l_genre)
{
	gboolean ret = FALSE;
	gint count = 0;
	strList* artistList = database_get_artists(NULL, NULL, l_genre, &count);
	if(count > 0)
		ret = database_tryToAdd_artists(&artistList, count);

	if(artistList != NULL)
		free_strList(artistList);

	return ret;
}

gboolean tryToAdd_random()
{
	return tryToAdd_genre(NULL);
}

static gboolean search_delayed(mpd_Song* l_song)
{
	search_start(l_song);
	m_delay_source = 0;
	return FALSE;
}

static void set_search_delay(mpd_Song* l_song)
{
	g_assert(l_song != NULL);
	reset_search_delay();

	gint timeout;
	if(l_song->time == MPD_SONG_NO_TIME || l_song->time > m_delay_timeout + BUFFER_SECONDS)
		timeout = m_delay_timeout;
	else
		timeout = l_song->time - BUFFER_SECONDS;

	m_delay_source = g_timeout_add_seconds(timeout,
						(GSourceFunc) search_delayed, l_song);

	/*
	m_delay_source = g_timeout_add_seconds_full(G_PRIORITY_DEFAULT,
			m_delay_timeout, (GSourceFunc) findSimilar_delayed,
			mpd_songDup(l_song), (GDestroyNotify) mpd_freeSong);
	*/
}

void reset_search_delay()
{
	if(m_delay_source != 0)
		g_source_remove(m_delay_source);
}

void search(mpd_Song* l_song, gint l_remains)
{
	g_assert(l_song != NULL);
	g_assert(l_remains >= 0);

	if(l_remains < 1 && !m_is_searching)
	{
		if(m_delay_timeout > 0)
			set_search_delay(l_song);
		else
			search_start(l_song);
	}
	else if(m_delay_timeout > 0)
		reset_search_delay();
}

void search_easy()
{
	if(!dyn_get_enabled())
	{
		playlist3_show_error_message(_("Dynamic playlist is disabled"), ERROR_INFO);
		return;
	}

	if(m_is_searching)
	{
		playlist3_show_error_message(_("Dynamic search is already busy"), ERROR_INFO);
		return;
	}

	mpd_Song* curSong = mpd_playlist_get_current_song(connection);
	if(curSong == NULL)
	{
		playlist3_show_error_message(_("You need to play a song that will be used"), ERROR_INFO);
		return;
	}

	search_start(curSong);
}

void search_start(mpd_Song* l_song)
{
	g_assert(l_song != NULL);
	g_assert(!m_is_searching);

	m_is_searching = TRUE;
	status start = NotFound;
	if(!m_similar_songs || l_song->artist == NULL || l_song->title == NULL)
		start |= Song;

	if(!m_similar_artists || l_song->artist == NULL)
		start |= Artist;

	if(!m_similar_genre || l_song->genre == NULL)
		start |= Genre;

	g_debug("Search | song: %d | artist: %d | genre: %d | artist: %s | title: %s | genre: %s",
			!(start & Song), !(start & Artist), !(start & Genre),
			l_song->artist, l_song->title, l_song->genre);

	tryToAdd_select(start, l_song);
}

gboolean is_searching()
{
	return m_is_searching;
}

gboolean is_search_delayed()
{
	return m_delay_source != 0;
}

gboolean will_search_delay()
{
	return m_delay_timeout > 0;
}

gboolean get_search_active()
{
	return m_enabled_search;
}

void set_search_active(gboolean l_value)
{
	if(l_value == m_enabled_search)
		return;

	m_enabled_search = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_search", m_enabled_search);
	reload_menu_list();
	reload_icon();
	if(!m_enabled_search)
		reset_search_delay();
}

void set_search_active_easy(gpointer l_data, const gchar* l_param)
{
	g_assert(l_param != NULL);

	if(g_str_has_prefix(l_param, _("on")))
		set_search_active(TRUE);
	else if(g_str_has_prefix(l_param, _("off")))
		set_search_active(FALSE);
	else
		set_search_active(!m_enabled_search);
}

void set_search_same_genre(gboolean l_value)
{
	m_same_genre = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "same_genre", m_same_genre);
}

gboolean get_search_same_genre()
{
	return m_same_genre;
}

gint get_delay_time()
{
	return m_delay_timeout;
}

void set_delay_time(gint l_value)
{
	m_delay_timeout = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "delayTimeout", m_delay_timeout);
}

gboolean get_search_artist()
{
	return m_similar_artists;
}

void set_search_artist(gboolean l_value)
{
	m_similar_artists = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_artists", m_similar_artists);
}

gboolean get_search_artist_same()
{
	return m_similar_artist_same;
}

void set_search_artist_same(gboolean l_value)
{
	m_similar_artist_same = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_artist_same", m_similar_artist_same);
}

gint get_search_artist_max()
{
	return m_similar_artists_max;
}

void set_search_artist_max(gint l_value)
{
	m_similar_artists_max = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "maxArtists", m_similar_artists_max);
}

gboolean get_search_song()
{
	return m_similar_songs;
}

void set_search_song(gboolean l_value)
{
	m_similar_songs = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_songs", m_similar_songs);
}

gint get_search_song_max()
{
	return m_similar_songs_max;
}

void set_search_song_max(gint l_value)
{
	m_similar_songs_max = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "maxSongs", m_similar_songs_max);
}

gboolean get_search_genre()
{
	return m_similar_genre;
}

void set_search_genre(gboolean l_value)
{
	m_similar_genre = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_genre", m_similar_genre);
}

gboolean get_search_genre_same()
{
	return m_similar_genre_same;
}

void set_search_genre_same(gboolean l_value)
{
	m_similar_genre_same = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_genre_same", m_similar_genre_same);
}

gint get_search_genre_max()
{
	return m_similar_genre_max;
}

void set_search_genre_max(gint l_value)
{
	m_similar_genre_max = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "maxGenres", m_similar_genre_max);
}

/* vim:set ts=4 sw=4: */
