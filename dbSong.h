#ifndef _DYN_LIST_PLUGIN_DBSONG
#define _DYN_LIST_PLUGIN_DBSONG

#include <glib.h>

typedef struct _dbSong
{
	gchar* artist;
	gchar* title;
	gchar* path;
} dbSong;

typedef GList dbList;
typedef GSList strList;
typedef GQueue dbQueue;

dbSong* new_dbSong(const gchar* l_artist, const gchar* l_title, const gchar* l_path);
void free_dbSong(dbSong* l_song);
void free_dbList(dbList* l_list);
void clear_dbList(dbList* l_list, gboolean l_free_list);

void clear_dbQueue(dbQueue* l_queue);

strList* new_strListItem(strList* l_list, const gchar* l_str);
void free_next_strListItem(strList* l_list);
void clear_strListItem(strList* l_list);
void free_strList(strList* l_list);
void clear_strList(strList* l_list, gboolean l_free_list);

#endif

/* vim:set ts=4 sw=4: */
