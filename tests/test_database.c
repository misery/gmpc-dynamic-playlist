#include <glib.h>
#include "database.h"
#include "blacklist.h"
#include "fixture_mpd.h"


gint test_database_search_songs_blacklist(dbList** l_list)
{
	g_assert(l_list != NULL);
	g_assert(*l_list == NULL);

	gint count = 0;
	MpdData* data;
	for(data = mpd_database_get_complete(connection); data != NULL; data = mpd_data_get_next(data))
	{
		if(data->song->artist != NULL && data->song->artist != NULL)
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
	fake_mpd_init(CONFIG_BL_ALL);

	set_active_blacklist(TRUE);
	dbList* list = NULL;

	/* all blacklisted by album, genre, artist or song */
	gint count = test_database_search_songs_blacklist(&list);

	g_test_message("count: %d", count);
	g_assert(list == NULL);
	g_assert(count == 0);

	fake_mpd_free(CONFIG_BL_ALL);
}

void test_database_search_songs_blacklist_found_all()
{
	fake_mpd_init(CONFIG_BL_ALL);

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
		if(data->song->artist != NULL && data->song->artist != NULL)
			g_assert(exists_dbList(list, data->song->artist, data->song->title));
	}

	/* check for untagged files (must not found) */
	const gchar* artist = "Ugly Kid Joe";
	g_assert(!exists_dbList(list, artist, "12 Cents"));
	g_assert(!exists_dbList(list, artist, "Oompa"));

	/* 69 unique + 2 doubled (because of EXACT = FALSE)*/
	g_assert(count == 71);

	fake_mpd_free(CONFIG_BL_ALL);
}

void test_database_search_artists_blacklist()
{
	fake_mpd_init(CONFIG_BL_ALL);

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
	/* TODO: finish test! */

	fake_mpd_free(CONFIG_BL_ALL);
}

static void redirect_log(const gchar* l_domain, GLogLevelFlags l_flags, const gchar* l_message, gpointer l_data)
{
	g_test_message("redirected: %s", l_message);
}

int main (int argc, char** argv)
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/database/search/songs/blacklist/nofound", test_database_search_songs_blacklist_found_zero);
	g_test_add_func("/database/search/songs/blacklist/found", test_database_search_songs_blacklist_found_all);
	g_test_add_func("/database/search/artists/blacklist", test_database_search_artists_blacklist);

	/* mute standard debug output from plugin */
	g_log_set_handler("dynlist", G_LOG_LEVEL_DEBUG, redirect_log, NULL);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
