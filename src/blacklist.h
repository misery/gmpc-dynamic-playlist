#ifndef _DYN_LIST_PLUGIN_BLACKLIST
#define _DYN_LIST_PLUGIN_BLACKLIST

#include <gmpc/plugin.h>
#include <libmpd/libmpd.h>

void set_active_blacklist(gboolean l_value);
gboolean get_active_blacklist();

gboolean is_blacklisted(const mpd_Song* l_song);
gboolean is_blacklisted_single(const GSList* l_list, const gchar* l_value);
gboolean is_blacklisted_tuple(const GSList* l_list, const gchar* l_artist, const gchar* l_name);

gboolean is_blacklisted_genre(const gchar* l_genre);
gboolean is_blacklisted_artist(const gchar* l_artist);
gboolean is_blacklisted_album(const gchar* l_artist, const gchar* l_album);
gboolean is_blacklisted_song(const gchar* l_artist, const gchar* l_title);

void create_blacklists();
gboolean create_blacklists_search(MpdData** l_out_lists, const gchar* l_blacklist);
void check_for_reload();
void reload_blacklists();

void load_blacklists();
void load_blacklist_genre();
void load_blacklist_artist();
void load_blacklist_album();
void load_blacklist_song();

void init_blacklists();
void free_blacklists();
void free_blacklists_tuple(GSList* l_list);

#endif

/* vim:set ts=4 sw=4: */
