#ifndef _DYN_LIST_FIXTURE_MPD
#define _DYN_LIST_FIXTURE_MPD

#include <glib.h>

void fake_mpd_init(const gchar* l_config);
void fake_mpd_kill(const gchar* l_config, gboolean l_try);
void fake_mpd_free(const gchar* l_config);

#endif

/* vim:set ts=4 sw=4: */
