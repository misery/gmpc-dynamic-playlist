#ifndef _DYN_LIST_PLUGIN_FMSONG
#define _DYN_LIST_PLUGIN_FMSONG

#include <libxml/tree.h>
#include <glib.h>

typedef struct _fmSong
{
	xmlChar* artist;
	xmlChar* title;
//	xmlChar* match;
} fmSong;

typedef GSList fmList;

fmSong* new_fmSong(xmlChar* l_artist, xmlChar* l_title/*, xmlChar* l_match*/);
void free_fmSong(fmSong* l_song);
void free_fmList(fmList* l_list);
void clear_fmList(fmList* l_list, gboolean l_free_list);


/* Modified from gmpc-last.fm-plugin (Qball Cow)
 * ***************************************************************************************/
xmlNodePtr get_first_node_by_name(xmlNodePtr l_xml, const gchar* l_name);
fmList* lastfm_get_artist_parse(const gchar* l_data, gint l_size);
fmList* lastfm_get_artist(const gchar* l_artist);
fmList* lastfm_get_song_parse(const gchar* l_data, gint l_size);
fmList* lastfm_get_song(const gchar* l_artist, const gchar* l_title);
/* ***************************************************************************************/

#endif

/* vim:set ts=4 sw=4: */
