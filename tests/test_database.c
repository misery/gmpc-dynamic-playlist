#include <glib.h>
#include "fixture_mpd.h"
#include "fixture_gmpc.h"
#include "../src/database.h"
#include "../src/blacklist.h"

void setup()
{
	fake_mpd_init(CONFIG_BL_ALL);
	fake_gmpc_init();
}

void tear_down()
{
	fake_gmpc_free();
	fake_mpd_free(CONFIG_BL_ALL);
}

gint test_database_search_songs_blacklist(dbList** l_list)
{
	g_assert(l_list != NULL);
	g_assert(*l_list == NULL);

	gint count = 0;
	MpdData* data;
	for(data = mpd_database_get_complete(connection); data != NULL; data = mpd_data_get_next(data))
	{
		if(data->song->artist != NULL && data->song->title != NULL)
			*l_list = database_get_songs(*l_list, data->song->artist, data->song->title, &count);
	}

	/* check for untagged files */
	const gchar* artist = "Ugly Kid Joe";
	*l_list = database_get_songs(*l_list, artist, "12 Cents", &count);
	*l_list = database_get_songs(*l_list, artist, "Oompa", &count); /* not blacklisted */

	return count;
}

/* Check if every searched file in database is somehow blacklisted
   It must find nothing!
*/
void test_database_search_songs_blacklist_found_zero()
{
	set_active_blacklist(TRUE);
	dbList* list = NULL;

	/* all blacklisted by album, genre, artist or song */
	gint count = test_database_search_songs_blacklist(&list);

	g_test_message("count: %d", count);
	g_assert(list == NULL);
	g_assert(count == 0);
}

void test_database_search_songs_blacklist_found_all()
{
	set_active_blacklist(FALSE);
	dbList* list = NULL;

	/* nothing is blacklisted */
	gint count = test_database_search_songs_blacklist(&list);
	g_test_message("count: %d", count);
	g_assert(list != NULL);
	g_assert(count > 0);

	/* check that EVERY song was found */
	MpdData* data;
	for(data = mpd_database_get_complete(connection); data != NULL; data = mpd_data_get_next(data))
	{
		if(data->song->artist != NULL && data->song->title != NULL)
			g_assert(exists_dbList(list, data->song->artist, data->song->title));
	}

	/* check for untagged files (must not found) */
	const gchar* artist = "Ugly Kid Joe";
	g_assert(!exists_dbList(list, artist, "12 Cents"));
	g_assert(!exists_dbList(list, artist, "Oompa"));

	/* 69 unique + 2 doubled (because of EXACT = FALSE)*/
	g_assert(count == 71);
	free_dbList(list);
}

void test_database_search_artists_blacklist()
{
	set_active_blacklist(TRUE);
	strList* list = NULL;
	gint count = 0;

	MpdData* data;
	for(data = mpd_database_get_artists(connection); data != NULL; data = mpd_data_get_next(data))
	{
		if(data->tag != NULL && data->tag[0] != '\0')
			list = database_get_artists(list, data->tag, NULL, &count);
	}

	g_test_message("count: %d", count);
	g_assert(list != NULL);
	g_assert(!exists_strList(list, "Bela B."));
	g_assert(exists_strList(list, "Metallica"));
	g_assert(exists_strList(list, "Ugly Kid Joe"));
	g_assert(exists_strList(list, "The Offspring"));
	g_assert(count == 3);
	free_strList(list);
}

void test_database_search_artists_parameter()
{
	set_active_blacklist(FALSE);
	strList* list = NULL;
	gint count = 0;

	list = database_get_artists(list, "Metallica", NULL, &count);
	g_assert(count == 1);
	g_assert(exists_strList(list, "Metallica"));
	free_strList(list);
	list = NULL;
	count = 0;

	list = database_get_artists(list, "Metallica", "Metal", &count);
	g_assert(count == 1);
	g_assert(exists_strList(list, "Metallica"));
	free_strList(list);
	list = NULL;
	count = 0;

	list = database_get_artists(list, "Metallica", "Thrash Metal", &count);
	g_assert(count == 1);
	g_assert(exists_strList(list, "Metallica"));
	free_strList(list);
	list = NULL;
	count = 0;

	list = database_get_artists(list, "Metallica", "Trance", &count);
	g_assert(count == 0 && list == NULL);

	list = database_get_artists(list, NULL, "Metal", &count);
	g_assert(count == 1);
	g_assert(exists_strList(list, "Metallica"));
	free_strList(list);
	list = NULL;
	count = 0;

	list = database_get_artists(list, NULL, "NoGenre", &count);
	g_assert(count == 0 && list == NULL);

	list = database_get_artists(list, "NoArtist", "NoGenre", &count);
	g_assert(count == 0 && list == NULL);

	list = database_get_artists(list, "NoArtist", NULL, &count);
	g_assert(count == 0 && list == NULL);

	list = database_get_artists(list, NULL, NULL, &count);
	g_assert(count == 4);
	g_assert(exists_strList(list, "Metallica"));
	g_assert(exists_strList(list, "The Offspring"));
	g_assert(exists_strList(list, "Bela B."));
	g_assert(exists_strList(list, "Ugly Kid Joe"));
	free_strList(list);
	list = NULL;
	count = 0;

	list = database_get_artists(list, NULL, "Rock", &count);
	g_assert(count == 2);
	g_assert(exists_strList(list, "The Offspring"));
	g_assert(exists_strList(list, "Ugly Kid Joe"));
	free_strList(list);
	list = NULL;
	count = 0;

	list = database_get_artists(list, "NoArtist", "Rock", &count);
	g_assert(count == 0 && list == NULL);
}

int main (int argc, char** argv)
{
	gtk_test_init(&argc, &argv, NULL);

	g_test_add("/database/search/songs/blacklist/nofound",  void, NULL, setup, test_database_search_songs_blacklist_found_zero, tear_down);
	g_test_add("/database/search/songs/blacklist/found",  void, NULL, setup, test_database_search_songs_blacklist_found_all, tear_down);
	g_test_add("/database/search/artists/blacklist",  void, NULL, setup, test_database_search_artists_blacklist, tear_down);
	g_test_add("/database/search/artists/parameter",  void, NULL, setup, test_database_search_artists_parameter, tear_down);

	/* mute standard debug output from plugin */
	g_log_set_handler("dynlist", G_LOG_LEVEL_DEBUG, redirect_log, NULL);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
