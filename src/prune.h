#ifndef _DYN_LIST_PLUGIN_PRUNE
#define _DYN_LIST_PLUGIN_PRUNE

#include <glib.h>

void init_prune();
void set_prune_value(gint l_value);
gint get_prune_value();

void prune_playlist(gint l_curPos);
void prune_playlist_value(gint l_curPos, gint l_keep);
void prune_playlist_easy(gpointer l_data, const gchar* l_param);

#endif

/* vim:set ts=4 sw=4: */
