#ifndef _DYN_LIST_PLUGIN
#define _DYN_LIST_PLUGIN

#include "fmSong.h"
#include "dbSong.h"

typedef enum
{
	NotFound = 0 << 0,
	Found = 1 << 0,
	FromSong = 1 << 1,
	FromArtist = 1 << 2
} status;

/* Queue function for m_lastSongs */
static void add_lastSongs(dbSong* l_song);
static gboolean exists_lastSongs(const xmlChar* l_artist, const xmlChar* l_title);

/* main function to find similar songs */
static dbList* database_get_songs(dbList* l_list, const gchar* l_artist, const gchar* l_title, gint* l_out_count);
static strList* database_get_artists(strList* l_list, const gchar* l_artist, const gchar* l_genre, gint* l_out_count);
static gboolean database_tryToAdd_artist(const gchar* l_artist);
static gboolean database_tryToAdd_artists(strList** l_out_list, gint l_count);
static void tryToAdd_artists(fmList* l_list);
static void tryToAdd_songs(fmList* l_list);
static gboolean tryToAdd_genre(const gchar* l_genre);
static void tryToAdd_select(status l_status);
static void findSimilar(const mpd_Song* l_song);
static void prune_playlist(gint l_curPos, gint l_keep);
static void prune_playlist_easy(gpointer l_data, const gchar* l_param);

/* plugin function */
void dyn_changed_status(MpdObj* l_mi, ChangedStatusType l_what, void* l_userdata);
void dyn_init();
void dyn_destroy();
gint dyn_get_enabled();
void dyn_set_enabled(gint l_enabled);

/* preferences */
void pref_destroy(GtkWidget* l_con);
void pref_similar(GtkWidget* l_con, gpointer l_data);
void pref_spins(GtkSpinButton* l_widget, gpointer l_data);
void pref_construct(GtkWidget* l_con);

#endif

/* vim:set ts=4 sw=4: */
