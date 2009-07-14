#ifndef _DYN_LIST_PLUGIN_PLAYED
#define _DYN_LIST_PLUGIN_PLAYED

#include "dbSong.h"

void add_played_song(dbSong* l_song);
gboolean is_played_song(const gchar* l_artist, const gchar* l_title);
gboolean is_played_artist(const gchar* l_artist);

void set_played_limit_song(gint l_song);
void set_played_limit_artist(gint l_artist);
gint get_played_limit_song();
gint get_played_limit_artist();

void free_played_list();

#endif

/* vim:set ts=4 sw=4: */
