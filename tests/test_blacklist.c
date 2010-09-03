#include <glib.h>
#include "fixture_mpd.h"
#include "fixture_gmpc.h"
#include "../src/blacklist.h"

void test_set_blacklist_active()
{
	fake_gmpc_init();

	set_active_blacklist(TRUE);
	g_assert(get_active_blacklist());

	set_active_blacklist(FALSE);
	g_assert(!get_active_blacklist());

	fake_gmpc_free();
}

void test_empty_blacklist()
{
	fake_mpd_init(CONFIG_EMPTY);
	fake_gmpc_init();

	set_active_blacklist(TRUE);
	const char* artist = "Metallica";
	const char* title = "Bleeding Me";
	const char* genre = "Metal";
	const char* album = "Load";

	g_assert(!is_blacklisted_artist(artist));
	g_assert(!is_blacklisted_song(artist, title));
	g_assert(!is_blacklisted_genre(genre));
	g_assert(!is_blacklisted_album(artist, album));

	fake_gmpc_free();
	fake_mpd_free(CONFIG_EMPTY);
}

int main (int argc, char** argv)
{
	gtk_test_init(&argc, &argv, NULL);

	g_test_add_func("/blacklist/active", test_set_blacklist_active);
	g_test_add_func("/blacklist/empty", test_empty_blacklist);

	/* mute standard debug output from plugin */
	g_log_set_handler("dynlist", G_LOG_LEVEL_DEBUG, redirect_log, NULL);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
