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
#include "plugin.h"

extern GmpcEasyCommand* gmpc_easy_command;

gint m_keep = -1;
gint m_block = 0;
gint m_similar_songs_max = 0;
gint m_similar_artists_max = 0;
gint m_similar_genre_max = 0;
gboolean m_similar_songs = FALSE;
gboolean m_similar_artists = FALSE;
gboolean m_similar_genre = FALSE;
gboolean m_same_genre = FALSE;
gboolean m_enabled = FALSE;
dbQueue m_lastSongs = G_QUEUE_INIT;
GRand* m_rand = NULL;
mpd_Song* m_curSong = NULL;
static GStaticMutex m_mutex = G_STATIC_MUTEX_INIT;
GtkWidget* m_menu_item = NULL;

void add_lastSongs(dbSong* l_song)
{
	g_queue_push_head(&m_lastSongs, l_song);
	if(g_queue_get_length(&m_lastSongs) > m_block)
		free_dbSong( (dbSong*) g_queue_pop_tail(&m_lastSongs) );
}

gboolean exists_lastSongs(const xmlChar* l_artist, const xmlChar* l_title)
{
	g_assert(l_artist != NULL && l_title != NULL);

	if(!g_queue_is_empty(&m_lastSongs))
	{
		dbList* iter = g_queue_peek_head_link(&m_lastSongs);
		for(; iter != NULL; iter = g_list_next(iter))
		{
			dbSong* song = (dbSong*) iter->data;
			if(xmlStrcasecmp((xmlChar*) song->artist, l_artist) == 0 && xmlStrcasecmp((xmlChar*) song->title, l_title) == 0)
				return TRUE;
		}
	}

	return FALSE;
}

dbList* database_get_songs(dbList* l_list, const gchar* l_artist, const gchar* l_title, gint* l_out_count)
{
	g_assert(l_artist != NULL && l_title != NULL && l_out_count != NULL && l_out_count >= 0);

	mpd_database_search_start(connection, FALSE);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, l_artist);
	mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_TITLE, l_title);

	MpdData* data;
	for(data = mpd_database_search_commit(connection); data != NULL; data = mpd_data_get_next(data))
	{
		if(data->type == MPD_DATA_TYPE_SONG && data->song->artist != NULL && data->song->title != NULL
			&& !exists_lastSongs((xmlChar*) data->song->artist, (xmlChar*) data->song->title))
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
	g_assert( (l_artist != NULL || l_genre != NULL) && l_out_count != NULL && *l_out_count >= 0 );

	mpd_database_search_field_start(connection, MPD_TAG_ITEM_ARTIST);
	if(l_artist != NULL)
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_ARTIST, l_artist);
	if(l_genre != NULL)
		mpd_database_search_add_constraint(connection, MPD_TAG_ITEM_GENRE, l_genre);

	MpdData* data;
	for(data = mpd_database_search_commit(connection); data != NULL; data = mpd_data_get_next(data))
	{
		if(data->type == MPD_DATA_TYPE_TAG && data->tag_type == MPD_TAG_ITEM_ARTIST)
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
			|| exists_lastSongs((xmlChar*) data->song->artist, (xmlChar*) data->song->title))
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

static void tryToAdd_select(status l_status)
{
	g_assert(m_curSong != NULL);

	if(l_status & Found)
	{
		mpd_freeSong(m_curSong);
		m_curSong = NULL;
		g_static_mutex_unlock(&m_mutex);
	}
	else if(m_similar_songs && l_status != FromSong && l_status != FromArtist && m_curSong->artist != NULL && m_curSong->title != NULL)
		lastfm_get_song_async(tryToAdd_songs, m_curSong->artist, m_curSong->title);
	else if(m_similar_artists && l_status != FromArtist && m_curSong->artist != NULL)
		lastfm_get_artist_async(tryToAdd_artists, m_curSong->artist, m_similar_artists_max);
	else if(m_same_genre && m_curSong->genre != NULL && tryToAdd_genre(m_curSong->genre))
		tryToAdd_select(Found);
	else
	{
		playlist3_show_error_message("Dynamic playlist cannot find a »similar« song", ERROR_INFO);
		mpd_freeSong(m_curSong);
		m_curSong = NULL;
		g_static_mutex_unlock(&m_mutex);
	}
}

void tryToAdd_artists(fmList* l_list)
{
	status ret = FromArtist;
	if(l_list != NULL)
	{
		gint count = 0;
		strList* artistList = NULL;
		const fmList* iter;
		for(iter = l_list; iter != NULL; iter = g_slist_next(iter))
		{
			fmSong* song = (fmSong*) iter->data;
			artistList = database_get_artists(artistList, (const gchar*) song->artist, NULL, &count);
		}

		if(count > 0)
		{
			if(m_curSong->artist != NULL) // add one artist to artistList (mostly because 'same artist' is also 'similar')
				artistList = database_get_artists(artistList, m_curSong->artist, NULL, &count);
			if(database_tryToAdd_artists(&artistList, count))
				ret |= Found;
		}

		if(artistList != NULL)
			free_strList(artistList);

		free_fmList(l_list);
	}

	tryToAdd_select(ret);
}

void tryToAdd_songs(fmList* l_list)
{
	status ret = FromSong;
	if(l_list != NULL)
	{
		gint count = 0;
		dbList* songList = NULL;
		gint maxIter = 0;
		const fmList* iter;
		for(iter = l_list; iter != NULL && maxIter < m_similar_songs_max; iter = g_slist_next(iter), ++maxIter)
		{
			fmSong* song = (fmSong*) iter->data;
			songList = database_get_songs(songList, (const gchar*) song->artist, (const gchar*) song->title, &count);
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
			ret |= Found;
		}
		free_fmList(l_list);
	}

	tryToAdd_select(ret);
}

gboolean tryToAdd_genre(const gchar* l_genre)
{
	g_assert(l_genre != NULL);

	gboolean ret = FALSE;
	gint count = 0;
	strList* artistList = database_get_artists(NULL, NULL, l_genre, &count);
	if(count > 0)
		ret = database_tryToAdd_artists(&artistList, count);

	if(artistList != NULL)
		free_strList(artistList);

	return ret;
}

void findSimilar_easy()
{
	if(!g_static_mutex_trylock(&m_mutex))
	{
		playlist3_show_error_message("Dynamic playlist already search for a »similar« song", ERROR_INFO);
		return;
	}

	mpd_Song* curSong = mpd_playlist_get_current_song(connection);
	if(curSong == NULL)
	{
		playlist3_show_error_message("You need to play a song that will be used", ERROR_INFO);
		g_static_mutex_unlock(&m_mutex);
		return;
	}

	m_curSong = mpd_songDup(curSong);
	tryToAdd_select(NotFound);
}

void findSimilar(const mpd_Song* l_song)
{
	g_assert(l_song != NULL);

	if(!g_static_mutex_trylock(&m_mutex))
		return;
	g_assert(m_curSong == NULL);

	if(l_song->artist != NULL || l_song->genre != NULL)
	{
		m_curSong = mpd_songDup(l_song);
		tryToAdd_select(NotFound);
	}
	else
	{
		playlist3_show_error_message("Dynamic playlist cannot find a »similar« song "
				"because current song has no useable artist or genre tag", ERROR_INFO);
		g_static_mutex_unlock(&m_mutex);
	}
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

	mpd_Song* curSong = mpd_playlist_get_current_song(connection);
	if(curSong == NULL)
	{
		playlist3_show_error_message("Cannot prune playlist! There is no current song as position.", ERROR_INFO);
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
			const gint remains = mpd_playlist_get_playlist_length(connection) - curSong->pos - 1;

			if(remains < 1)
				findSimilar(curSong);

			prune_playlist(curSong->pos, m_keep);
		}
	}
}

void dyn_init()
{
	g_assert(m_rand == NULL);

	m_keep = cfg_get_single_value_as_int_with_default(config, "dynlist-lastfm", "keep", -1);
	m_block = cfg_get_single_value_as_int_with_default(config, "dynlist-lastfm", "block", 100);
	m_similar_songs_max = cfg_get_single_value_as_int_with_default(config, "dynlist-lastfm", "maxSongs", 20);
	m_similar_artists_max = cfg_get_single_value_as_int_with_default(config, "dynlist-lastfm", "maxArtists", 30);
	m_similar_genre_max = cfg_get_single_value_as_int_with_default(config, "dynlist-lastfm", "maxGenres", 5);
	m_similar_songs = cfg_get_single_value_as_int_with_default(config, "dynlist-lastfm", "similar_songs", FALSE);
	m_similar_artists = cfg_get_single_value_as_int_with_default(config, "dynlist-lastfm", "similar_artists", FALSE);
	m_similar_genre = cfg_get_single_value_as_int_with_default(config, "dynlist-lastfm", "similar_genre", FALSE);
	m_same_genre = cfg_get_single_value_as_int_with_default(config, "dynlist-lastfm", "same_genre", FALSE);
	m_rand = g_rand_new();

	gmpc_easy_command_add_entry(gmpc_easy_command, "prune", "[0-9]*",  "Prune playlist", (GmpcEasyCommandCallback*) prune_playlist_easy, NULL);
	gmpc_easy_command_add_entry(gmpc_easy_command, "dynlist", "(on|off|)",  "Dynamic playlist (on|off)", (GmpcEasyCommandCallback*) dyn_enable_easy, NULL);
	gmpc_easy_command_add_entry(gmpc_easy_command, "similar", "",  "Search for similar song/artist/genre", (GmpcEasyCommandCallback*) findSimilar_easy, NULL);

	if(mpd_check_connected(connection) && !mpd_server_check_version(connection, 0, 12, 0))
	{
		m_enabled = FALSE;
		playlist3_show_error_message("Dynamic playlist disabled because of too old mpd (< 0.12)'", ERROR_INFO);
	}
	else
		m_enabled = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "enable", TRUE);
}

void dyn_destroy()
{
	if(!g_queue_is_empty(&m_lastSongs))
		clear_dbQueue(&m_lastSongs);

	g_rand_free(m_rand);
}

void dyn_enable_easy(gpointer l_data, const gchar* l_param)
{
	if(g_str_has_prefix(l_param, "on"))
		dyn_set_enabled(TRUE);
	else if(g_str_has_prefix(l_param, "off"))
		dyn_set_enabled(FALSE);
	else
		dyn_set_enabled(!m_enabled);
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
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_menu_item), m_enabled);
}

void dyn_tool_menu_integration_activate(GtkCheckMenuItem* l_menu_item)
{
	gboolean active = gtk_check_menu_item_get_active(l_menu_item);
	dyn_set_enabled(active);
}

int dyn_tool_menu_integration(GtkMenu* l_menu)
{
	m_menu_item = gtk_check_menu_item_new_with_label("Dynamic playlist");
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_menu_item), dyn_get_enabled());
	g_signal_connect(G_OBJECT(m_menu_item), "activate", G_CALLBACK(dyn_tool_menu_integration_activate), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(l_menu), m_menu_item);

	GtkWidget* similar_item = gtk_image_menu_item_new_with_label("Add similar song");
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(similar_item), gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
	g_signal_connect(G_OBJECT(similar_item), "activate", G_CALLBACK(findSimilar_easy), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(l_menu), similar_item);
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
	if(type == similar_artist)
	{
		m_similar_artists = value;
		cfg_set_single_value_as_int(config, "dynlist-lastfm", "similar_artists", m_similar_artists);
	}
	else if(type == similar_song)
	{
		m_similar_songs = value;
		cfg_set_single_value_as_int(config, "dynlist-lastfm", "similar_songs", m_similar_songs);
	}
	else if(type == similar_genre)
	{
		m_similar_genre = value;
		cfg_set_single_value_as_int(config, "dynlist-lastfm", "similar_genre", m_similar_genre);
	}
	else if(type == same_genre)
	{
		m_same_genre = value;
		cfg_set_single_value_as_int(config, "dynlist-lastfm", "same_genre", m_same_genre);
	}
	else
		g_assert_not_reached();
}

void pref_spins(GtkSpinButton* l_widget, gpointer l_data)
{
	option spin = (option) GPOINTER_TO_INT(l_data);
	gint value = gtk_spin_button_get_value_as_int(l_widget);
	if(spin == prune)
	{
		m_keep = value;
		cfg_set_single_value_as_int(config, "dynlist-lastfm", "keep", m_keep);
	}
	else if(spin == block)
	{
		m_block = value;
		cfg_set_single_value_as_int(config, "dynlist-lastfm", "block", m_block);
	}
	else if(spin == similar_song_max)
	{
		m_similar_songs_max = value;
		cfg_set_single_value_as_int(config, "dynlist-lastfm", "maxSongs", m_similar_songs_max);
	}
	else if(spin == similar_artist_max)
	{
		m_similar_artists_max = value;
		cfg_set_single_value_as_int(config, "dynlist-lastfm", "maxArtists", m_similar_artists_max);
	}
	else if(spin == similar_genre_max)
	{
		m_similar_genre_max = value;
		cfg_set_single_value_as_int(config, "dynlist-lastfm", "maxGenres", m_similar_genre_max);
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
	gtk_label_set_markup(GTK_LABEL(label), "<b>Last.FM options</b>");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* Search for similar songs */
	GtkWidget* song_toggle = gtk_check_button_new_with_label("Search songs in »similar songs«");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(song_toggle), m_similar_songs);
	gtk_box_pack_start(GTK_BOX(vbox), song_toggle, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(song_toggle), "toggled", G_CALLBACK(pref_similar), GINT_TO_POINTER(similar_song));

	/* Search for max similar songs */
	GtkWidget* song_hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget* song_label = gtk_label_new("Search max. songs in database:");
	GtkAdjustment* song_adj = (GtkAdjustment*) gtk_adjustment_new(m_similar_songs_max, 1.0, 200, 1.0, 5.0, 0.0);
	GtkWidget* song_spin = gtk_spin_button_new(song_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(song_spin), "value-changed", G_CALLBACK(pref_spins), GINT_TO_POINTER(similar_song_max));

	gtk_box_pack_start(GTK_BOX(song_hbox), song_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(song_hbox), song_spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), song_hbox, FALSE, FALSE, 0);

	/* Search for similar artists */
	GtkWidget* artist_toggle = gtk_check_button_new_with_label("Search songs in »similar artists«");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(artist_toggle), m_similar_artists);
	gtk_box_pack_start(GTK_BOX(vbox), artist_toggle, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(artist_toggle), "toggled", G_CALLBACK(pref_similar), GINT_TO_POINTER(similar_artist));

	/* Search for max similar artists */
	GtkWidget* artist_hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget* artist_label = gtk_label_new("Search max. artists in database:");
	GtkAdjustment* artist_adj = (GtkAdjustment*) gtk_adjustment_new(m_similar_artists_max, 1.0, 200, 1.0, 5.0, 0.0);
	GtkWidget* artist_spin = gtk_spin_button_new(artist_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(artist_spin), "value-changed", G_CALLBACK(pref_spins), GINT_TO_POINTER(similar_artist_max));

	gtk_box_pack_start(GTK_BOX(artist_hbox), artist_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(artist_hbox), artist_spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), artist_hbox, FALSE, FALSE, 0);

	/* Search for similar genre */
	GtkWidget* genre_toggle = gtk_check_button_new_with_label("Search songs in »similar genre«");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(genre_toggle), m_similar_genre);
	gtk_box_pack_start(GTK_BOX(vbox), genre_toggle, FALSE, FALSE, 0);
	gtk_widget_set_sensitive(genre_toggle, FALSE);
	g_signal_connect(G_OBJECT(genre_toggle), "toggled", G_CALLBACK(pref_similar), GINT_TO_POINTER(similar_genre));

	/* Search for max similar genre */
	GtkWidget* genre_hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget* genre_label = gtk_label_new("Search max. genre in database:");
	GtkAdjustment* genre_adj = (GtkAdjustment*) gtk_adjustment_new(m_similar_genre_max, 1.0, 20, 1.0, 5.0, 0.0);
	GtkWidget* genre_spin = gtk_spin_button_new(genre_adj, 1.0, 0);
	gtk_widget_set_sensitive(genre_spin, FALSE);
	g_signal_connect(G_OBJECT(genre_spin), "value-changed", G_CALLBACK(pref_spins), GINT_TO_POINTER(similar_genre_max));

	gtk_box_pack_start(GTK_BOX(genre_hbox), genre_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(genre_hbox), genre_spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), genre_hbox, FALSE, FALSE, 0);


	/* other options label */
	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label), "<b>Local options</b>");
	gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);

	/* Search in same genre */
	GtkWidget* same_genre_toggle = gtk_check_button_new_with_label("Search songs in same genre");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(same_genre_toggle), m_same_genre);
	gtk_box_pack_start(GTK_BOX(vbox), same_genre_toggle, FALSE, FALSE, 0);
	g_signal_connect(G_OBJECT(same_genre_toggle), "toggled", G_CALLBACK(pref_similar), GINT_TO_POINTER(same_genre));

	/* Prune Playlist - SpinButton */
	GtkWidget* prune_hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget* prune_label = gtk_label_new("Keep x songs after current:");
	GtkAdjustment* prune_adj = (GtkAdjustment*) gtk_adjustment_new(m_keep, -1.0, 100, 1.0, 5.0, 0.0);
	GtkWidget* prune_spin = gtk_spin_button_new(prune_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(prune_spin), "value-changed", G_CALLBACK(pref_spins), GINT_TO_POINTER(prune));

	gtk_box_pack_start(GTK_BOX(prune_hbox), prune_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(prune_hbox), prune_spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), prune_hbox, FALSE, FALSE, 0);

	/* Block Songs - SpinButton */
	GtkWidget* block_hbox = gtk_hbox_new(FALSE, 5);
	GtkWidget* block_label = gtk_label_new("Block played song for at least x songs:");
	GtkAdjustment* block_adj = (GtkAdjustment*) gtk_adjustment_new(m_block, 0.0, 500, 1.0, 5.0, 0.0);
	GtkWidget* block_spin = gtk_spin_button_new(block_adj, 1.0, 0);
	g_signal_connect(G_OBJECT(block_spin), "value-changed", G_CALLBACK(pref_spins), GINT_TO_POINTER(block));

	gtk_box_pack_start(GTK_BOX(block_hbox), block_label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(block_hbox), block_spin, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), block_hbox, FALSE, FALSE, 0);


	if(!dyn_get_enabled())
		gtk_widget_set_sensitive(GTK_WIDGET(vbox), FALSE);

	gtk_widget_show_all(frame);
	gtk_container_add(GTK_CONTAINER(l_con), frame);
}

gmpcPrefPlugin dyn_pref =
{
	.construct = pref_construct,
	.destroy   = pref_destroy
};

gint plugin_api_version = PLUGIN_API_VERSION;

gmpcPlugin plugin = {
	.name               = "Dynamic Playlist (Last.FM)",
	.version            = {0,9,1},
	.plugin_type        = GMPC_PLUGIN_NO_GUI,
	.init               = dyn_init,
	.destroy            = dyn_destroy,
	.mpd_status_changed = dyn_changed_status,
	.pref               = &dyn_pref,
	.get_enabled        = dyn_get_enabled,
	.set_enabled        = dyn_set_enabled,
	.tool_menu_integration = dyn_tool_menu_integration
};

/* vim:set ts=4 sw=4: */
