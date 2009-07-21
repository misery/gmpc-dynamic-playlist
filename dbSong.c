/* gmpc-dynamic-playlist (GMPC plugin)
 * Copyright (C) 2009 Andre Klitzing <andre@incubo.de>
 * Homepage: http://www.incubo.de

 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "dbSong.h"
#include <string.h>

dbSong* new_dbSong(const gchar* l_artist, const gchar* l_title, const gchar* l_path)
{
	g_assert(l_artist != NULL && l_title != NULL && l_path != NULL);

	dbSong* ret = g_slice_new(dbSong);
	ret->artist = g_strdup(l_artist);
	ret->title = g_strdup(l_title);
	ret->path = g_strdup(l_path);

	return ret;
}

void free_dbSong(dbSong* l_song)
{
	g_assert(l_song->artist != NULL && l_song->title != NULL && l_song->path != NULL);

	g_free(l_song->artist);
	g_free(l_song->title);
	g_free(l_song->path);

	g_slice_free(dbSong, l_song);
}

void free_dbList(dbList* l_list)
{
	clear_dbList(l_list, TRUE);
}

void clear_dbList(dbList* l_list, gboolean l_free_list)
{
	g_assert(l_list != NULL);

	dbList* iter;
	for(iter = l_list; iter != NULL; iter = g_list_next(iter))
		free_dbSong((dbSong*) iter->data);

	if(l_free_list)
		g_list_free(l_list);
}

gboolean exists_dbList(const dbList* l_list, const gchar* l_artist, const gchar* l_title)
{
	g_assert(l_artist != NULL);
	g_assert(l_title != NULL);

	for(; l_list != NULL; l_list = g_list_next(l_list))
	{
		const dbSong* song = (const dbSong*) l_list->data;
		if(strcasecmp(song->artist, l_artist) == 0 && strcasecmp(song->title, l_title) == 0)
			return TRUE;
	}

	return FALSE;
}

void clear_dbQueue(dbQueue* l_queue)
{
	g_assert(l_queue != NULL && !g_queue_is_empty(l_queue));

	dbList* list = g_queue_peek_head_link(l_queue);
	clear_dbList(list, FALSE);
	g_queue_clear(l_queue);
}

strList* new_strListItem(strList* l_list, const gchar* l_str)
{
	g_assert(l_str != NULL);
	return g_slist_prepend(l_list, g_strdup(l_str));
}

void free_next_strListItem(strList* l_list)
{
	g_assert(l_list != NULL && l_list->next != NULL);

	strList* tmp = l_list->next;
	l_list->next = tmp->next;
	g_free((gchar*) tmp->data);
	g_slist_free_1(tmp);
}

void clear_strListItem(strList* l_list)
{
	g_assert(l_list != NULL && l_list->data != NULL);

	g_free((gchar*) l_list->data);
	l_list->data = NULL;
}

void free_strList(strList* l_list)
{
	clear_strList(l_list, TRUE);
}

void clear_strList(strList* l_list, gboolean l_free_list)
{
	g_assert(l_list != NULL);

	strList* iter;
	for(iter = l_list; iter != NULL; iter = g_slist_next(iter))
		g_free((gchar*) iter->data);

	if(l_free_list)
		g_slist_free(l_list);
}

gboolean exists_strList(const strList* l_list, const gchar* l_value)
{
	g_assert(l_value != NULL);

	for(; l_list != NULL; l_list = g_slist_next(l_list))
	{
		if(strcasecmp((gchar*) (l_list->data), l_value) == 0)
			return TRUE;
	}

	return FALSE;
}

/* vim:set ts=4 sw=4: */
