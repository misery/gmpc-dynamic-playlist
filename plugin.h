#ifndef _DYN_LIST_PLUGIN
#define _DYN_LIST_PLUGIN

#include "dbSong.h"

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
	similar_song = 1 << 0,
	similar_song_max = 1 << 1,
	similar_artist = 1 << 2,
	similar_artist_max = 1 << 3,
	similar_artist_same = 1 << 4,
	similar_genre = 1 << 5,
	similar_genre_max = 1 << 6,
	similar_genre_same = 1 << 7,
	prune = 1 << 8,
	block_song = 1 << 9,
	block_artist = 1 << 10,
	same_genre = 1 << 11,
	similar_search = 1 << 12
} option;

/* Queue function for m_lastSongs */
static void add_lastSongs(dbSong* l_song);
static void flush_lastSongs(gint l_max);
static gboolean exists_lastSongs(const gchar* l_artist, const gchar* l_title);
static gboolean exists_lastArtists(const gchar* l_artist);

/* main function to find similar songs */
static dbList* database_get_songs(dbList* l_list, const gchar* l_artist, const gchar* l_title, gint* l_out_count);
static strList* database_get_artists(strList* l_list, const gchar* l_artist, const gchar* l_genre, gint* l_out_count);
static gboolean database_tryToAdd_artist(const gchar* l_artist);
static gboolean database_tryToAdd_artists(strList** l_out_list, gint l_count);
static status getNextStatus(status l_status);
static void tryToAdd_artists(mpd_Song* l_song, MetaDataResult l_result, MetaData* l_data, gpointer l_last_status);
static void tryToAdd_songs(mpd_Song* l_song, MetaDataResult l_result, MetaData* l_data, gpointer l_last_status);
static void tryToAdd_multiple_genre(mpd_Song* l_song, MetaDataResult l_result, MetaData* l_data, gpointer l_last_status);
static gboolean tryToAdd_genre(const gchar* l_genre);
static gboolean tryToAdd_random();
static void tryToAdd_select(const status l_status, mpd_Song* l_song);
static void findSimilar_easy();
static void findSimilar(mpd_Song* l_song);
static void prune_playlist(gint l_curPos, gint l_keep);
static void prune_playlist_easy(gpointer l_data, const gchar* l_param);

/* plugin function */
void dyn_changed_status(MpdObj* l_mi, ChangedStatusType l_what, void* l_userdata);
void dyn_init();
void dyn_destroy();
static void dyn_enable_easy(gpointer l_data, const gchar* l_param);
gint dyn_get_enabled();
void dyn_set_enabled(gint l_enabled);
static void dyn_tool_menu_integration_activate(GtkCheckMenuItem* l_menu_item);
static int dyn_tool_menu_integration(GtkMenu* l_menu);

/* preferences */
void pref_destroy(GtkWidget* l_con);
void pref_similar(GtkWidget* l_con, gpointer l_data);
void pref_similar_set(option l_type, gint l_value);
void pref_spins(GtkSpinButton* l_widget, gpointer l_data);
void pref_spins_set(option l_type, gint l_value);
void pref_construct(GtkWidget* l_con);

#endif

/* vim:set ts=4 sw=4: */
