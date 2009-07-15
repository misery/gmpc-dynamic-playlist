#include <glib.h>
#include "blacklist.h"

void test_set_blacklist_active()
{
	set_active_blacklist(TRUE);
	g_assert(get_active_blacklist());

	set_active_blacklist(FALSE);
	g_assert(!get_active_blacklist());
}

void test_empty_blacklist()
{
	set_active_blacklist(TRUE);
	const char* artist = "Metallica";
	const char* title = "Battery";
	const char* genre = "Metal";
	const char* album = "Master Of Puppets";

	g_assert(!is_blacklisted_artist(artist));
	g_assert(!is_blacklisted_song(artist, title));
	g_assert(!is_blacklisted_genre(genre));
	g_assert(!is_blacklisted_album(artist, album));
}

int main (int argc, char** argv)
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/blacklist/active", test_set_blacklist_active);
	g_test_add_func("/blacklist/empty", test_empty_blacklist);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
