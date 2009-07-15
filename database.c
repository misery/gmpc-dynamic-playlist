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

#include <gmpc/plugin.h>
#include "database.h"
#include "blacklist.h"
#include "played.h"

extern GRand* m_rand;

dbList* database_get_songs(dbList* l_list, const gchar* l_artist, const gchar* l_title, gint* l_out_count)
{
	g_assert(l_artist != NULL && l_title != NULL && l_out_count != NULL && l_out_count >= 0);

	if(is_blacklisted_artist(l_artist) || is_blacklisted_song(l_artist, l_title))
		return l_list;

	mpd_database_search_start(connection, FALSE);
	gchar** artist_split = g_strsplit(l_artist, " ", -1);
	gint i;
	for(i = 0; artist_split != NULL && artist_split[i] != NULL; ++i)
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, artist_split[i]);
	g_strfreev(artist_split);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_TITLE, l_title);

	MpdData* data;
	for(data = mpd_database_search_commit(connection); data != NULL; data = mpd_data_get_next(data))
	{
		if(data->song->artist != NULL && data->song->title != NULL
			&& !is_blacklisted(data->song)
			&& !is_played_song(data->song->artist, data->song->title))
		{
			dbSong* song = new_dbSong(data->song->artist, data->song->title, data->song->file);
			l_list = g_list_prepend(l_list, song);
			++(*l_out_count);
		}
	}

	return l_list;
}

strList* database_get_artists(strList* l_list, const gchar* l_artist, const gchar* l_genre, gint* l_out_count)
{
	g_assert(l_out_count != NULL && *l_out_count >= 0);

	if(is_blacklisted_genre(l_genre) || is_blacklisted_artist(l_artist))
		return l_list;

	mpd_database_search_field_start(connection, MPD_TAG_ITEM_ARTIST);
	if(l_artist != NULL)
	{
		gchar** artist_split = g_strsplit(l_artist, " ", -1);
		gint i;
		for(i = 0; artist_split != NULL && artist_split[i] != NULL; ++i)
			mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, artist_split[i]);
		g_strfreev(artist_split);
	}
	if(l_genre != NULL)
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_GENRE, l_genre);

	MpdData* data;
	for(data = mpd_database_search_commit(connection); data != NULL; data = mpd_data_get_next(data))
	{
		if(data->tag_type == MPD_TAG_ITEM_ARTIST
				&& !is_blacklisted_artist(data->tag)
				&& !is_played_artist(data->tag))
		{
			l_list = new_strListItem(l_list, data->tag);
			++(*l_out_count);
		}
	}

	return l_list;
}

gboolean database_tryToAdd_artist(const gchar* l_artist)
{
	g_assert(l_artist != NULL);

	mpd_database_search_start(connection, FALSE);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, l_artist);

	gint count = 0;
	MpdData* prev = NULL;
	gboolean first = TRUE;
	MpdData* data = mpd_database_search_commit(connection);
	while(data != NULL)
	{
		const gchar* artist = data->song->albumartist == NULL ? data->song->artist : data->song->albumartist;
		if(data->song->artist == NULL || data->song->title == NULL
			|| is_blacklisted_genre(data->song->genre)
			|| is_blacklisted_album(artist, data->song->album)
			|| is_blacklisted_song(data->song->artist, data->song->title)
			|| is_played_song(data->song->artist, data->song->title))
		{
			data = mpd_data_delete_item(data);
			if( data == NULL || (first && ((MpdData_real*) data)->prev == NULL) )
				continue;
		}
		else
			++count;

		first = FALSE;
		prev = data;
		data = mpd_data_get_next_real(data, FALSE);
	}

	if(count > 0)
	{
		g_assert(prev != NULL);

		gint random = g_rand_int_range(m_rand, 0, count);
		gint i = 0;
		for(data = mpd_data_get_first(prev); i < random; ++i)
			data = mpd_data_get_next_real(data, FALSE);
		g_assert(data != NULL);

		dbSong* song = new_dbSong(data->song->artist, data->song->title, data->song->file);
		mpd_playlist_add(connection, song->path);
		add_played_song(song);
		g_debug("Added via artist | artist: %s | title: %s | genre: %s",
				data->song->artist, data->song->title, data->song->genre);
		mpd_data_free(data);

		return TRUE;
	}

	return FALSE;
}

gboolean database_tryToAdd_artists(strList** l_out_list, gint l_count)
{
	g_assert(l_out_list != NULL && l_count > 0);

	gboolean found = FALSE;
	do
	{
		gint random = g_rand_int_range(m_rand, 0, l_count);
		gint i = 0;
		strList* prev = NULL;
		strList* iter;
		for(iter = *l_out_list; i < random; ++i)
		{
			prev = iter;
			iter = g_slist_next(iter);
		}

		--l_count;
		found = database_tryToAdd_artist( (gchar*) iter->data );
		if(prev == NULL) // first element will be freed
		{
			strList* tmp = *l_out_list;
			*l_out_list = g_slist_next(*l_out_list);
			clear_strListItem(tmp);
			g_slist_free_1(tmp);
		}
		else
			free_next_strListItem(prev);
	}
	while(!found && l_count > 0);

	return found;
}

/* vim:set ts=4 sw=4: */
