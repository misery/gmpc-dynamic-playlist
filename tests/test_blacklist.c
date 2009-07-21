#include <glib.h>
#include "blacklist.h"
#include "fixture_mpd.h"

void test_set_blacklist_active()
{
	set_active_blacklist(TRUE);
	g_assert(get_active_blacklist());

	set_active_blacklist(FALSE);
	g_assert(!get_active_blacklist());
}

void test_empty_blacklist()
{
	fake_mpd_init(CONFIG_EMPTY);

	set_active_blacklist(TRUE);
	const char* artist = "Metallica";
	const char* title = "Bleeding Me";
	const char* genre = "Metal";
	const char* album = "Load";

	g_assert(!is_blacklisted_artist(artist));
	g_assert(!is_blacklisted_song(artist, title));
	g_assert(!is_blacklisted_genre(genre));
	g_assert(!is_blacklisted_album(artist, album));

	fake_mpd_free(CONFIG_EMPTY);
}

static void redirect_log(const gchar* l_domain, GLogLevelFlags l_flags, const gchar* l_message, gpointer l_data)
{
	g_test_message("redirected: %s", l_message);
}

int main (int argc, char** argv)
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/blacklist/active", test_set_blacklist_active);
	g_test_add_func("/blacklist/empty", test_empty_blacklist);

	/* mute standard debug output from plugin */
	g_log_set_handler("dynlist", G_LOG_LEVEL_DEBUG, redirect_log, NULL);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
