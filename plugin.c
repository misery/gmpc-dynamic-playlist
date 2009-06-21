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
#include <gmpc/playlist3-messages.h>
#include <gmpc/gmpc-easy-command.h>
#include <libmpd/libmpd-internal.h>
#include <glib.h>
#include <glib/gi18n-lib.h>
#include "plugin.h"
#include "blacklist.h"

extern GmpcEasyCommand* gmpc_easy_command;

gint m_keep = -1;
gint m_block_song = 0;
gint m_block_artist = 0;
gint m_similar_songs_max = 0;
gint m_similar_artists_max = 0;
gint m_similar_genre_max = 0;
gint m_similar_artist_same = TRUE;
gint m_similar_genre_same = TRUE;
gboolean m_similar_songs = FALSE;
gboolean m_similar_artists = FALSE;
gboolean m_similar_genre = FALSE;
gboolean m_same_genre = FALSE;
gboolean m_enabled = TRUE;
gboolean m_enabled_search = FALSE;
dbQueue m_lastSongs = G_QUEUE_INIT;
GRand* m_rand = NULL;
static GStaticMutex m_mutex = G_STATIC_MUTEX_INIT;

/* Menu */
GtkWidget* m_menu_item = NULL;
GtkWidget* m_menu = NULL;
GtkWidget* m_menu_search = NULL;

/* Option Dialog */
GtkWidget* m_artist_same_toggle = NULL;

void add_lastSongs(dbSong* l_song)
{
	g_assert(l_song != NULL);

	g_queue_push_head(&m_lastSongs, l_song);
	flush_lastSongs( MAX(m_block_song, m_block_artist) );
}

void flush_lastSongs(gint l_max)
{
	g_assert(l_max >= 0);

	const gint max = g_queue_get_length(&m_lastSongs) - l_max;
	gint i;
	for(i = 0; i < max; ++i)
		free_dbSong( (dbSong*) g_queue_pop_tail(&m_lastSongs) );
}

gboolean exists_lastSongs(const gchar* l_artist, const gchar* l_title)
{
	g_assert(l_artist != NULL);

	if(!g_queue_is_empty(&m_lastSongs))
	{
		dbList* iter = g_queue_peek_head_link(&m_lastSongs);
		gint i;
		for(i = 0; iter != NULL; iter = g_list_next(iter), ++i)
		{
			dbSong* song = (dbSong*) iter->data;
			if(i < m_block_artist && strcasecmp(song->artist, l_artist) == 0)
			{
				g_debug("Artist blocked: %s", l_artist);
				return TRUE;
			}
			if(i < m_block_song && l_title != NULL && strcasecmp(song->artist, l_artist) == 0 && strcasecmp(song->title, l_title) == 0)
			{
				g_debug("Song blocked: %s::%s", l_artist, l_title);
				return TRUE;
			}
		}
	}

	return FALSE;
}

gboolean exists_lastArtists(const gchar* l_artist)
{
	return exists_lastSongs(l_artist, NULL);
}

dbList* database_get_songs(dbList* l_list, const gchar* l_artist, const gchar* l_title, gint* l_out_count)
{
	g_assert(l_artist != NULL && l_title != NULL && l_out_count != NULL && l_out_count >= 0);

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
		if(data->type == MPD_DATA_TYPE_SONG && data->song->artist != NULL && data->song->title != NULL
			&& (data->song->genre == NULL || !is_blacklisted_genre(data->song->genre))
			&& !is_blacklisted_artist(data->song->artist)
			&& !exists_lastSongs(data->song->artist, data->song->title))
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
		if(data->type == MPD_DATA_TYPE_TAG && data->tag_type == MPD_TAG_ITEM_ARTIST &&
				!is_blacklisted_artist(data->tag) && !exists_lastArtists(data->tag))
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
	MpdData* data;
	gboolean first = TRUE;
	for(data = mpd_database_search_commit(connection); data != NULL;)
	{
		if(data->type != MPD_DATA_TYPE_SONG || data->song->artist == NULL || data->song->title == NULL
			|| (data->song->genre != NULL && is_blacklisted_genre(data->song->genre))
			|| exists_lastSongs(data->song->artist, data->song->title))
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
		mpd_data_free(data);
		mpd_playlist_add(connection, song->path);
		add_lastSongs(song);

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

static void tryToAdd_select(const status l_status, mpd_Song* l_song)
{
	g_assert(l_song != NULL);

	if(l_status & Found)
	{
		g_debug("Song added");
		g_static_mutex_unlock(&m_mutex);
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
		{
			g_debug("Added same genre song");
			g_static_mutex_unlock(&m_mutex);
		}
		else if(tryToAdd_random())
		{
			g_debug("Added random song");
			g_static_mutex_unlock(&m_mutex);
		}
		else
		{
			playlist3_show_error_message(_("Dynamic search cannot find a new song"), ERROR_INFO);
			g_debug("Cannot find a new song");
			g_static_mutex_unlock(&m_mutex);
		}
	}
}

void tryToAdd_artists(mpd_Song* l_song, MetaDataResult l_result, MetaData* l_data, gpointer l_last_status)
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
			if(l_song->artist != NULL && m_similar_artist_same && !m_block_artist)
				artistList = database_get_artists(artistList, l_song->artist, NULL, &count);
			if(database_tryToAdd_artists(&artistList, count))
				l_status |= Found;
		}

		if(artistList != NULL)
			free_strList(artistList);
	}

	tryToAdd_select(l_status, l_song);
}

void tryToAdd_songs(mpd_Song* l_song, MetaDataResult l_result, MetaData* l_data, gpointer l_last_status)
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
			add_lastSongs(song);

			// Remove added dbSong* from dbList so it won't be freed
			songList = g_list_delete_link(songList, songListIter);
			if(songList != NULL)
				free_dbList(songList);
			l_status |= Found;
		}
	}

	tryToAdd_select(l_status, l_song);
}

void tryToAdd_multiple_genre(mpd_Song* l_song, MetaDataResult l_result, MetaData* l_data, gpointer l_last_status)
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

void findSimilar_easy()
{
	if(!m_enabled)
	{
		playlist3_show_error_message(_("Dynamic playlist is disabled"), ERROR_INFO);
		return;
	}

	if(!g_static_mutex_trylock(&m_mutex))
	{
		playlist3_show_error_message(_("Dynamic search is already busy"), ERROR_INFO);
		return;
	}

	mpd_Song* curSong = mpd_playlist_get_current_song(connection);
	if(curSong == NULL)
	{
		playlist3_show_error_message(_("You need to play a song that will be used"), ERROR_INFO);
		g_static_mutex_unlock(&m_mutex);
		return;
	}

	findSimilar(curSong);
}

void findSimilar(mpd_Song* l_song)
{
	g_assert(l_song != NULL);

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

void prune_playlist(gint l_curPos, gint l_keep)
{
	if(l_keep < 0 || l_curPos < 1)
		return;

	gint del;
	for(del = 0; del < l_curPos - l_keep; ++del)
		mpd_playlist_queue_delete_pos(connection, 0);

	mpd_playlist_queue_commit(connection);
}

void prune_playlist_easy(gpointer l_data, const gchar* l_param)
{
	g_assert(l_param != NULL);

	if(!m_enabled)
	{
		playlist3_show_error_message(_("Dynamic playlist is disabled"), ERROR_INFO);
		return;
	}

	mpd_Song* curSong = mpd_playlist_get_current_song(connection);
	if(curSong == NULL)
	{
		playlist3_show_error_message(_("Cannot prune playlist! There is no current song as position"), ERROR_INFO);
		return;
	}

	if(l_param[0] == '\0')
		prune_playlist(curSong->pos, m_keep);
	else
		prune_playlist(curSong->pos, atoi(l_param));
}

void dyn_changed_status(MpdObj* l_mi, ChangedStatusType l_what, void* l_userdata)
{
	if(m_enabled && (l_what & MPD_CST_PLAYLIST || l_what & MPD_CST_SONGPOS ||
			(l_what & MPD_CST_STATE && mpd_player_get_state(connection) == MPD_PLAYER_PLAY)))
	{
		mpd_Song* curSong = mpd_playlist_get_current_song(connection);
		if(curSong != NULL)
		{
			const gint curPos = curSong->pos;
			if(m_enabled_search)
			{
				const gint remains = mpd_playlist_get_playlist_length(connection) - curPos - 1;
				if(remains < 1 && g_static_mutex_trylock(&m_mutex))
					findSimilar(curSong);
			}

			prune_playlist(curPos, m_keep);
		}
	}
	else if(m_enabled && l_what & MPD_CST_STORED_PLAYLIST)
		reload_blacklists();
}

void dyn_init()
{
	g_assert(m_rand == NULL);

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	m_keep = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "keep", -1);
	m_block_song = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "block", 100);
	m_block_artist = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "block_artist", 0);
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
	m_enabled = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "enable", TRUE);
	m_rand = g_rand_new();
	if(m_enabled)
		reload_blacklists();

	gmpc_easy_command_add_entry(gmpc_easy_command, _("prune"), "[0-9]*",  _("Prune playlist"), (GmpcEasyCommandCallback*) prune_playlist_easy, NULL);
	gmpc_easy_command_add_entry(gmpc_easy_command, _("dynamic"), "(on|off|)",  _("Dynamic search (on|off)"), (GmpcEasyCommandCallback*) dyn_enable_easy, NULL);
	gmpc_easy_command_add_entry(gmpc_easy_command, _("similar"), "",  _("Search for similar song/artist/genre"), (GmpcEasyCommandCallback*) findSimilar_easy, NULL);
}

void dyn_destroy()
{
	if(!g_queue_is_empty(&m_lastSongs))
		clear_dbQueue(&m_lastSongs);

	free_blacklists();
	g_rand_free(m_rand);
}

void dyn_enable_easy(gpointer l_data, const gchar* l_param)
{
	if(g_str_has_prefix(l_param, "on"))
		pref_similar_set(similar_search, TRUE);
	else if(g_str_has_prefix(l_param, "off"))
		pref_similar_set(similar_search, FALSE);
	else
		pref_similar_set(similar_search, !m_enabled_search);
}

gint dyn_get_enabled()
{
	return m_enabled;
}

void dyn_set_enabled(gint l_enabled)
{
	g_assert(m_menu_item != NULL);

	m_enabled = l_enabled;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "enable", m_enabled);
	gtk_widget_set_sensitive(m_menu_item, m_enabled);
	if(m_enabled)
		reload_blacklists();
}

void dyn_tool_menu_integration_activate(GtkCheckMenuItem* l_menu_item)
{
	m_enabled_search = gtk_check_menu_item_get_active(l_menu_item);
	cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_search", m_enabled_search);
}

int dyn_tool_menu_integration(GtkMenu* l_menu)
{
	m_menu_item = gtk_image_menu_item_new_with_label(_("Dynamic Playlist"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(m_menu_item), gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU));
	m_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(m_menu_item), m_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(l_menu), m_menu_item);
	gtk_widget_set_sensitive(m_menu_item, m_enabled);

	m_menu_search = gtk_check_menu_item_new_with_label(_("Dynamic search"));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_menu_search), m_enabled_search);
	g_signal_connect(G_OBJECT(m_menu_search), "activate", G_CALLBACK(dyn_tool_menu_integration_activate), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), m_menu_search);

	GtkWidget* menu_add_song = gtk_image_menu_item_new_with_label(_("Add similar song"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_add_song), gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
	g_signal_connect(G_OBJECT(menu_add_song), "activate", G_CALLBACK(findSimilar_easy), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), menu_add_song);
	return 1;
}

void pref_destroy(GtkWidget* l_con)
{
	GtkWidget* child = gtk_bin_get_child(GTK_BIN(l_con));
	if(child)
		gtk_container_remove(GTK_CONTAINER(l_con), child);
}

void pref_similar(GtkWidget* l_con, gpointer l_data)
{
	option type = (option) GPOINTER_TO_INT(l_data);
	gint value = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(l_con));
	pref_similar_set(type, value);
}

void pref_similar_set(option l_type, gint l_value)
{
	if(l_type == similar_artist)
	{
		m_similar_artists = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_artists", m_similar_artists);
	}
	else if(l_type == similar_artist_same)
	{
		m_similar_artist_same = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_artist_same", m_similar_artist_same);
	}
	else if(l_type == similar_song)
	{
		m_similar_songs = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_songs", m_similar_songs);
	}
	else if(l_type == similar_genre)
	{
		m_similar_genre = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_genre", m_similar_genre);
	}
	else if(l_type == similar_genre_same)
	{
		m_similar_genre_same = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_genre_same", m_similar_genre_same);
	}
	else if(l_type == same_genre)
	{
		m_same_genre = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "same_genre", m_same_genre);
	}
	else if(l_type == similar_search)
	{
		m_enabled_search = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "similar_search", m_enabled_search);
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_menu_search), m_enabled_search);
	}
	else
		g_assert_not_reached();
}

void pref_spins(GtkSpinButton* l_widget, gpointer l_data)
{
	option type = (option) GPOINTER_TO_INT(l_data);
	gint value = gtk_spin_button_get_value_as_int(l_widget);
	pref_spins_set(type, value);
}

void pref_spins_set(option l_type, gint l_value)
{
	if(l_type == prune)
	{
		m_keep = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "keep", m_keep);
	}
	else if(l_type == block_song)
	{
		m_block_song = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "block", m_block_song);
	}
	else if(l_type == block_artist)
	{
		m_block_artist = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "block_artist", m_block_artist);
		if(m_artist_same_toggle != NULL)
			gtk_widget_set_sensitive(m_artist_same_toggle, !m_block_artist);
	}
	else if(l_type == similar_song_max)
	{
		m_similar_songs_max = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "maxSongs", m_similar_songs_max);
	}
	else if(l_type == similar_artist_max)
	{
		m_similar_artists_max = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "maxArtists", m_similar_artists_max);
	}
	else if(l_type == similar_genre_max)
	{
		m_similar_genre_max = l_value;
		cfg_set_single_value_as_int(config, "dynamic-playlist", "maxGenres", m_similar_genre_max);
	}
	else
		g_assert_not_reached();
}

void pref_construct(GtkWidget* l_con)
{
	GtkWidget* frame = gtk_frame_new("");
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
	GtkWidget* vbox = gtk_vbox_new(FALSE, 2);
	gtk_container_set_border_width(GTK_CONTAINER(vbox), 12);
	gtk_container_add(GTK_CONTAINER(frame), vbox);

	/* last.fm label */
	GtkWidget* label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Meta-Data options</b>"));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* Search for similar songs */
	GtkWidget* song_toggle = gtk_check_button_new_with_label(_("Search songs in 'similar songs'"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(song_toggle), m_similar_songs);
	gtk_box_pack_start(GTK_BOX(vbox), song_toggle, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(song_toggle), "toggled", G_CALLBACK(pref_similar), GINT_TO_POINTER(similar_song));

	/* Search for max similar songs */
	GtkWidget* song_hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget* song_label = gtk_label_new(_("Search max. songs in database:"));
	GtkAdjustment* song_adj = (GtkAdjustment*) gtk_adjustment_new(m_similar_songs_max, 1.0, 200, 1.0, 5.0, 0.0);
	GtkWidget* song_spin = gtk_spin_button_new(song_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(song_spin), "value-changed", G_CALLBACK(pref_spins), GINT_TO_POINTER(similar_song_max));

	gtk_box_pack_start(GTK_BOX(song_hbox), song_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(song_hbox), song_spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), song_hbox, FALSE, FALSE, 0);

	/* Search for similar artists */
	GtkWidget* artist_toggle = gtk_check_button_new_with_label(_("Search songs in 'similar artists'"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(artist_toggle), m_similar_artists);
	gtk_box_pack_start(GTK_BOX(vbox), artist_toggle, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(artist_toggle), "toggled", G_CALLBACK(pref_similar), GINT_TO_POINTER(similar_artist));

	/* Search for same similar artist */
	m_artist_same_toggle = gtk_check_button_new_with_label(_("Same artist is also 'similar'"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(m_artist_same_toggle), m_similar_artist_same);
	gtk_widget_set_sensitive(m_artist_same_toggle, !m_block_artist);
	gtk_box_pack_start(GTK_BOX(vbox), m_artist_same_toggle, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(m_artist_same_toggle), "toggled", G_CALLBACK(pref_similar), GINT_TO_POINTER(similar_artist_same));

	/* Search for max similar artists */
	GtkWidget* artist_hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget* artist_label = gtk_label_new(_("Search max. artists in database:"));
	GtkAdjustment* artist_adj = (GtkAdjustment*) gtk_adjustment_new(m_similar_artists_max, 1.0, 200, 1.0, 5.0, 0.0);
	GtkWidget* artist_spin = gtk_spin_button_new(artist_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(artist_spin), "value-changed", G_CALLBACK(pref_spins), GINT_TO_POINTER(similar_artist_max));

	gtk_box_pack_start(GTK_BOX(artist_hbox), artist_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(artist_hbox), artist_spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), artist_hbox, FALSE, FALSE, 0);

	/* Search for similar genre */
	GtkWidget* genre_toggle = gtk_check_button_new_with_label(_("Search songs in 'similar genre'"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(genre_toggle), m_similar_genre);
	gtk_box_pack_start(GTK_BOX(vbox), genre_toggle, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(genre_toggle), "toggled", G_CALLBACK(pref_similar), GINT_TO_POINTER(similar_genre));

	/* Search for same similar genre */
	GtkWidget* genre_same_toggle = gtk_check_button_new_with_label(_("Same genre is also 'similar'"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(genre_same_toggle), m_similar_genre_same);
	gtk_box_pack_start(GTK_BOX(vbox), genre_same_toggle, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(genre_same_toggle), "toggled", G_CALLBACK(pref_similar), GINT_TO_POINTER(similar_genre_same));

	/* Search for max similar genre */
	GtkWidget* genre_hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget* genre_label = gtk_label_new(_("Search max. genre in database:"));
	GtkAdjustment* genre_adj = (GtkAdjustment*) gtk_adjustment_new(m_similar_genre_max, 1.0, 20, 1.0, 5.0, 0.0);
	GtkWidget* genre_spin = gtk_spin_button_new(genre_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(genre_spin), "value-changed", G_CALLBACK(pref_spins), GINT_TO_POINTER(similar_genre_max));

	gtk_box_pack_start(GTK_BOX(genre_hbox), genre_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(genre_hbox), genre_spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), genre_hbox, FALSE, FALSE, 0);


	/* other options label */
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), _("<b>Local options</b>"));
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* Enable similar/dynamic search */
	GtkWidget* similar_search_toggle = gtk_check_button_new_with_label(_("Enable dynamic search"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(similar_search_toggle), m_enabled_search);
	gtk_box_pack_start(GTK_BOX(vbox), similar_search_toggle, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(similar_search_toggle), "toggled", G_CALLBACK(pref_similar), GINT_TO_POINTER(similar_search));

	/* Search in same genre */
	GtkWidget* same_genre_toggle = gtk_check_button_new_with_label(_("Search songs in same genre"));
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(same_genre_toggle), m_same_genre);
	gtk_box_pack_start(GTK_BOX(vbox), same_genre_toggle, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(same_genre_toggle), "toggled", G_CALLBACK(pref_similar), GINT_TO_POINTER(same_genre));

	/* Prune Playlist - SpinButton */
	GtkWidget* prune_hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget* prune_label = gtk_label_new(_("Keep x songs after current:"));
	GtkAdjustment* prune_adj = (GtkAdjustment*) gtk_adjustment_new(m_keep, -1.0, 100, 1.0, 5.0, 0.0);
	GtkWidget* prune_spin = gtk_spin_button_new(prune_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(prune_spin), "value-changed", G_CALLBACK(pref_spins), GINT_TO_POINTER(prune));

	gtk_box_pack_start(GTK_BOX(prune_hbox), prune_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(prune_hbox), prune_spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), prune_hbox, FALSE, FALSE, 0);

	/* Block Songs - SpinButton */
	GtkWidget* block_hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget* block_label = gtk_label_new(_("Block played song for at least x songs:"));
	GtkAdjustment* block_adj = (GtkAdjustment*) gtk_adjustment_new(m_block_song, 0.0, 500, 1.0, 5.0, 0.0);
	GtkWidget* block_spin = gtk_spin_button_new(block_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(block_spin), "value-changed", G_CALLBACK(pref_spins), GINT_TO_POINTER(block_song));

	gtk_box_pack_start(GTK_BOX(block_hbox), block_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(block_hbox), block_spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), block_hbox, FALSE, FALSE, 0);

	/* Block Artists - SpinButton */
	GtkWidget* block_artist_hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget* block_artist_label = gtk_label_new(_("Block played artist for at least x songs:"));
	GtkAdjustment* block_artist_adj = (GtkAdjustment*) gtk_adjustment_new(m_block_artist, 0.0, 100, 1.0, 5.0, 0.0);
	GtkWidget* block_artist_spin = gtk_spin_button_new(block_artist_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(block_artist_spin), "value-changed", G_CALLBACK(pref_spins), GINT_TO_POINTER(block_artist));

	gtk_box_pack_start(GTK_BOX(block_artist_hbox), block_artist_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(block_artist_hbox), block_artist_spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), block_artist_hbox, FALSE, FALSE, 0);


	if(!dyn_get_enabled())
		gtk_widget_set_sensitive(GTK_WIDGET(vbox), FALSE);

	gtk_widget_show_all(frame);
	gtk_container_add(GTK_CONTAINER(l_con), frame);
}

static const gchar* dyn_get_translation_domain()
{
	return GETTEXT_PACKAGE;
}

gmpcPrefPlugin dyn_pref =
{
	.construct = pref_construct,
	.destroy   = pref_destroy
};

gint plugin_api_version = PLUGIN_API_VERSION;

gmpcPlugin plugin = {
	.name               = N_("Dynamic Playlist"),
	.version            = {VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH},
	.plugin_type        = GMPC_PLUGIN_NO_GUI,
	.init               = dyn_init,
	.destroy            = dyn_destroy,
	.mpd_status_changed = dyn_changed_status,
	.pref               = &dyn_pref,
	.get_enabled        = dyn_get_enabled,
	.set_enabled        = dyn_set_enabled,
	.tool_menu_integration = dyn_tool_menu_integration,
	.get_translation_domain = dyn_get_translation_domain
};

/* vim:set ts=4 sw=4: */
