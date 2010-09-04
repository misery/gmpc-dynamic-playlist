#include <gmpc/plugin.h>
#include <stdlib.h>
#include "fixture_gmpc.h"
#include "fixture_mpd.h"

void test_mpd_init()
{
	fake_mpd_init(CONFIG);
	if(g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDERR))
	{
		fake_mpd_init(CONFIG);
		exit(EXIT_SUCCESS);
	}
	g_test_trap_assert_failed();
	fake_mpd_free(CONFIG);
}

void test_mpd_check_std()
{
	if(g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDERR))
	{
		fake_mpd_init("dummy");
		exit(EXIT_SUCCESS);
	}
	g_test_trap_assert_failed();
}

void test_gmpc_init()
{
	fake_gmpc_init();
	if(g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDERR))
	{
		fake_gmpc_init();
		exit(EXIT_SUCCESS);
	}
	g_test_trap_assert_failed();
	fake_gmpc_free();
}

void test_assert_message()
{
	if(g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDERR))
	{
		g_assert_message("notInList");
		exit(EXIT_SUCCESS);
	}
	g_test_trap_assert_failed();
}

int main(int argc, char** argv)
{
	gtk_test_init(&argc, &argv, NULL);

	g_test_add_func("/fixture/mpd/init", test_mpd_init);
	g_test_add_func("/fixture/mpd/check_std", test_mpd_check_std);
	g_test_add_func("/fixture/gmpc/init", test_gmpc_init);
	g_test_add_func("/fixture/gmpc/assert_message", test_assert_message);

	/* mute standard debug output from plugin */
	g_log_set_handler("dynlist", G_LOG_LEVEL_DEBUG, redirect_log, NULL);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
