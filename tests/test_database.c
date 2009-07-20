#include <glib.h>
#include "database.h"
#include "blacklist.h"
#include "fixture_mpd.h"


/* Check if every searched file in database is somehow blacklisted
   It must find nothing!
*/
void test_database_search_songs_blacklist()
{
	fake_mpd_init(CONFIG_BL_ALL);

	set_active_blacklist(TRUE);
	dbList* list = NULL;
	gint count = 0;

	/* all blacklisted by album, genre or artist */
	MpdData* data;
	for(data = mpd_database_get_complete(connection); data != NULL; data = mpd_data_get_next(data))
	{
		if(data->song->artist != NULL && data->song->artist != NULL)
			list = database_get_songs(list, data->song->artist, data->song->title, &count);
	}

	/* check for untagged files */
	const gchar* artist = "Ugly Kid Joe";
	list = database_get_songs(list, artist, "12 Cents", &count);
	list = database_get_songs(list, artist, "Oompa", &count); /* not blacklisted */

	g_test_message("count: %d", count);
	g_assert(list == NULL);
	g_assert(count == 0);

	fake_mpd_free(CONFIG_BL_ALL);
}

void test_database_search_artists_blacklist()
{
	fake_mpd_init(CONFIG_BL_ALL);

	set_active_blacklist(TRUE);
	/* TODO: add test for database_get_artists() */

	fake_mpd_free(CONFIG_BL_ALL);
}

static void redirect_log(const gchar* l_domain, GLogLevelFlags l_flags, const gchar* l_message, gpointer l_data)
{

}

int main (int argc, char** argv)
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/database/search/songs/blacklist", test_database_search_songs_blacklist);
	g_test_add_func("/database/search/artists/blacklist", test_database_search_artists_blacklist);

	/* mute standard debug output from plugin */
	g_log_set_handler("dynlist", G_LOG_LEVEL_DEBUG, redirect_log, NULL);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
