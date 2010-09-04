#include <gtk/gtk.h>
#include <libmpd/libmpd.h>
#include "fixture_mpd.h"
#include "fixture_gmpc.h"
#include "../src/plugin.h"
#include "../src/prune.h"

void clear_playlist()
{
	g_assert_cmpint(mpd_player_stop(connection), ==, 0);
	g_assert_cmpint(mpd_playlist_clear(connection), ==, 0);
	g_assert_cmpint(mpd_playlist_get_playlist_length(connection), ==, 0);
}

void fill_playlist(int l_count)
{
	const int count = mpd_playlist_get_playlist_length(connection);
	int i;
	for(i = 0; i < l_count; ++i)
	{
		mpd_playlist_add(connection, "Metallica/Death Magnetic/Metallica - 01 - That Was Just Your Life.mp3");
	}
	g_assert_cmpint(mpd_playlist_get_playlist_length(connection), ==, count + l_count);
}

void setup()
{
	fake_mpd_init(CONFIG);
	fake_gmpc_init();
	clear_playlist();
}

void tear_down()
{
	fake_gmpc_free();
	fake_mpd_free(CONFIG);
}



void test_set_prune_value()
{
	set_prune_value(1904);
	g_assert_cmpint(get_prune_value(), ==, 1904);
}

void test_prune_playlist()
{
	fill_playlist(10);
	set_prune_value(1); // keep one already played song in playlist
	prune_playlist(5); // 0-3 = remove songs, 4 = keeping played song, 5 = current playing song, 6-9 = next songs
	g_assert_cmpint(mpd_playlist_get_playlist_length(connection), ==, 6);
}

void test_prune_playlist_all()
{
	fill_playlist(15);
	prune_playlist_value(20, 0); // keep no played song
	g_assert_cmpint(mpd_playlist_get_playlist_length(connection), ==, 0);
}

void test_prune_playlist_disabled()
{
	fill_playlist(15);
	prune_playlist_value(20, -1); // disabled pruning
	g_assert_cmpint(mpd_playlist_get_playlist_length(connection), ==, 15);
}

void test_prune_playlist_stopped()
{
	fill_playlist(15);
	prune_playlist_value(0, 10); // stopped player or playing first song
	g_assert_cmpint(mpd_playlist_get_playlist_length(connection), ==, 15);
}

void test_prune_playlist_easy_disabled()
{
	fill_playlist(1);
	dyn_set_enabled(FALSE);
	prune_playlist_easy(NULL, "10");
	g_assert_cmpint(mpd_playlist_get_playlist_length(connection), ==, 1);
	g_assert_message("Dynamic playlist is disabled");
}

void test_prune_playlist_easy_stopped()
{
	fill_playlist(1);
	dyn_set_enabled(TRUE);
	prune_playlist_easy(NULL, "10");
	g_assert_cmpint(mpd_playlist_get_playlist_length(connection), ==, 1);
	g_assert_message("Cannot prune playlist! You need to play a song for pruning.");
}

int main(int argc, char** argv)
{
	gtk_test_init(&argc, &argv, NULL);

	g_test_add("/prune/set_prune_value", void, NULL, setup, test_set_prune_value, tear_down);
	g_test_add("/prune/prune_playlist", void, NULL, setup, test_prune_playlist, tear_down);
	g_test_add("/prune/prune_playlist_all", void, NULL, setup, test_prune_playlist_all, tear_down);
	g_test_add("/prune/prune_playlist_disabled", void, NULL, setup, test_prune_playlist_disabled, tear_down);
	g_test_add("/prune/prune_playlist_stopped", void, NULL, setup, test_prune_playlist_stopped, tear_down);
	g_test_add("/prune/prune_playlist_easy_disabled", void, NULL, setup, test_prune_playlist_easy_disabled, tear_down);
	g_test_add("/prune/prune_playlist_easy_stopped", void, NULL, setup, test_prune_playlist_easy_stopped, tear_down);

	/* mute standard debug output from plugin */
	g_log_set_handler("dynlist", G_LOG_LEVEL_DEBUG, redirect_log, NULL);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
