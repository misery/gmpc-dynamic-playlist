#include <glib.h>
#include <stdlib.h>
#include "played.h"

static void test_set_limits_allowed()
{
	int set1 = 0;
	int set2 = 500;

	g_assert_cmpint(get_played_limit_song(), ==, 0);
	g_assert_cmpint(get_played_limit_artist(), ==, 0);

	set_played_limit_song(set1);
	set_played_limit_artist(set1);
	g_assert_cmpint(get_played_limit_song(), ==, set1);
	g_assert_cmpint(get_played_limit_artist(), ==, set1);

	set_played_limit_song(set2);
	set_played_limit_artist(set2);
	g_assert_cmpint(get_played_limit_song(), ==, set2);
	g_assert_cmpint(get_played_limit_artist(), ==, set2);
}

static void test_set_limits_assert_song()
{
	if(g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR))
	{
		set_played_limit_song(-1);
		exit(0);
	}
	g_test_trap_assert_failed();
}

static void test_set_limits_assert_artist()
{
	if(g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDERR))
	{
		set_played_limit_artist(-1);
		exit(0);
	}
	g_test_trap_assert_failed();
}

int main (int argc, char** argv)
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/played/set_limits/allowed", test_set_limits_allowed);
	g_test_add_func("/played/set_limits/assert/song", test_set_limits_assert_song);
	g_test_add_func("/played/set_limits/assert/artist", test_set_limits_assert_artist);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
