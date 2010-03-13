#ifndef _DYN_LIST_PLUGIN_SEARCH
#define _DYN_LIST_PLUGIN_SEARCH

#include <gmpc/plugin.h>
#include <gmpc/playlist3-messages.h>

#define STATUS_COUNT 4
typedef enum
{
	NotFound = 0 << 0,
	Found = 1 << 0,
	Song = 1 << 1,
	Artist = 1 << 2,
	Genre = 1 << 3
} status;

typedef enum
{
	ArtistOf = 0,
	Same = 1
} searchStyle;

gboolean tryToAdd_genre(const gchar* l_genre);
gboolean tryToAdd_comment(const gchar* l_comment);
gboolean tryToAdd_random();
void search(mpd_Song* l_song, gint l_remains, gboolean l_force_no_delay);
void search_easy();
void search_start(mpd_Song* l_song);
void reset_search_delay();

void init_search();
gboolean is_searching();
gboolean is_search_delayed();
gboolean will_search_delay();

void set_search_active(gboolean l_value);
void set_search_active_easy(gpointer l_data, const gchar* l_param);
gboolean get_search_active();

gint get_queue_songs();
void set_queue_songs(gint l_value);

void set_local_search_genre(gboolean l_value);
gboolean get_local_search_genre();

void set_local_search_genre_style(searchStyle l_value);
searchStyle get_local_search_genre_style();

void set_local_search_comment(gboolean l_value);
gboolean get_local_search_comment();

gint get_delay_time();
void set_delay_time(gint l_value);

gboolean get_search_artist();
void set_search_artist(gboolean l_value);
gboolean get_search_artist_same();
void set_search_artist_same(gboolean l_value);
gint get_search_artist_max();
void set_search_artist_max(gint l_value);

gboolean get_search_song();
void set_search_song(gboolean l_value);
gint get_search_song_max();
void set_search_song_max(gint l_value);

gboolean get_search_genre();
void set_search_genre(gboolean l_value);
gboolean get_search_genre_same();
void set_search_genre_same(gboolean l_value);
gint get_search_genre_max();
void set_search_genre_max(gint l_value);

#endif

/* vim:set ts=4 sw=4: */
