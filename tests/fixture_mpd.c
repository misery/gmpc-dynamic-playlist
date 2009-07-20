#include "fixture_mpd.h"
#include <libmpd/libmpd.h>
#include <sys/wait.h>

#define HOST "localhost"
#define PORT 1904

MpdObj* connection = NULL;

static GError* check_std(gchar* l_out, gchar* l_err)
{
	gboolean failed = FALSE;

	if(l_out != NULL && g_pattern_match_simple("*Failed*", l_out))
		failed = TRUE;
	else if(l_err != NULL && g_pattern_match_simple("*Failed*", l_err))
		failed = TRUE;

	return failed ? g_error_new(G_SPAWN_ERROR, 666, "stdout: %s | stderr: %s", l_out, l_err) : NULL;
}

static GError* spawn(gchar** l_argv)
{
	g_assert(l_argv != NULL);

	gchar* std_out = NULL;
	gchar* std_err = NULL;
	gint result_code = 0;
	GError* err = NULL;

	g_spawn_sync(NULL, l_argv, NULL, 0, NULL, NULL, &std_out, &std_err, &result_code, &err);
	if(err == NULL)
		err = check_std(std_out, std_err);

	if(std_out != NULL)
		g_free(std_out);
	if(std_err != NULL)
		g_free(std_err);

	return err;
}

void fake_mpd_init(const gchar* l_config)
{
	g_assert(l_config != NULL);
	g_assert(connection == NULL);

	/* try to kill previous mpd */
	fake_mpd_kill(l_config, TRUE);

	gchar* argv[4];
	argv[0] = MPD_BINARY;
	argv[1] = "--no-create-db";
	argv[2] = (gchar*) l_config;
	argv[3] = NULL;

	g_assert_no_error(spawn(argv));

	connection = mpd_new(HOST, PORT, NULL);
	g_assert(mpd_connect(connection) == MPD_OK);
}

void fake_mpd_kill(const gchar* l_config, gboolean l_try)
{
	g_assert(l_config != NULL);

	gchar* argv[4];
	argv[0] = MPD_BINARY;
	argv[1] = "--kill";
	argv[2] = (gchar*) l_config;
	argv[3] = NULL;

	GError* err = spawn(argv);
	if(!l_try)
		g_assert_no_error(err);
	else if(err != NULL)
		g_error_free(err);
}

void fake_mpd_free(const gchar* l_config)
{
	g_assert(connection != NULL);

	g_assert(mpd_disconnect(connection) == MPD_OK);
	mpd_free(connection);
	connection = NULL;

	fake_mpd_kill(l_config, FALSE);
}

/* vim:set ts=4 sw=4: */
