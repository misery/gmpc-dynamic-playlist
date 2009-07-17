#include "fixture_mpd.h"
#include <libmpd/libmpd.h>
#include <sys/wait.h>

#define HOST "localhost"
#define PORT 1904

MpdObj* connection = NULL;

static void check_and_free_std(gchar* l_out, gchar* l_err, gboolean l_fail)
{
	if(l_fail)
		g_debug("forced fail");

	if(l_out != NULL)
	{
		if(l_fail || g_pattern_match_simple("*Failed*", l_out))
		{
			g_debug("stdout: %s", l_out);
			l_fail = TRUE;
		}
		g_free(l_out);
	}

	if(l_err != NULL)
	{
		if(l_fail || g_pattern_match_simple("*Failed*", l_err))
		{
			g_debug("stderr: %s", l_err);
			l_fail = TRUE;
		}
		g_free(l_err);
	}

	g_assert(!l_fail);
}

static void spawn(gchar** l_argv)
{
	g_assert(l_argv != NULL);

	gchar* std_out = NULL;
	gchar* std_err = NULL;
	gint result_code = 0;
	GError* err = NULL;

	if(!g_spawn_sync(NULL, l_argv, NULL, 0, NULL, NULL, &std_out, &std_err, &result_code, &err))
		g_assert_no_error(err);
	else if(!WIFEXITED(result_code))
		check_and_free_std(std_out, std_err, TRUE);
	else
		check_and_free_std(std_out, std_err, FALSE);
}

void fake_mpd_init(const gchar* l_config)
{
	g_assert(l_config != NULL);
	g_assert(connection == NULL);

	gchar* argv[4];
	argv[0] = MPD_BINARY;
	argv[1] = "--no-create-db";
	argv[2] = (gchar*) l_config;
	argv[3] = NULL;

	spawn(argv);

	connection = mpd_new(HOST, PORT, NULL);
	g_assert(mpd_connect(connection) == MPD_OK);
}

void fake_mpd_free(const gchar* l_config)
{
	g_assert(l_config != NULL);
	g_assert(connection != NULL);

	g_assert(mpd_disconnect(connection) == MPD_OK);
	mpd_free(connection);
	connection = NULL;

	gchar* argv[4];
	argv[0] = MPD_BINARY;
	argv[1] = "--kill";
	argv[2] = (gchar*) l_config;
	argv[3] = NULL;

	spawn(argv);
}

/* vim:set ts=4 sw=4: */
