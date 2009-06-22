#ifndef _DYN_LIST_PLUGIN_BLACKLIST
#define _DYN_LIST_PLUGIN_BLACKLIST

#define BLACKLIST_GENRE "#Blacklist Genre"
#define BLACKLIST_ARTIST "#Blacklist Artist"

#include <glib.h>
#include <libmpd/libmpd.h>

void set_active_blacklist(gboolean l_value);
gboolean get_active_blacklist();

gboolean is_blacklisted(const mpd_Song* l_song);
gboolean is_blacklisted_slist(GSList* l_list, const gchar* l_value);

gboolean is_blacklisted_genre(const gchar* l_genre);
gboolean is_blacklisted_artist(const gchar* l_artist);

void check_for_reload();
void reload_blacklists();
void load_blacklists();
void load_blacklist_genre();
void load_blacklist_artist();
void free_blacklists();

#endif

/* vim:set ts=4 sw=4: */
