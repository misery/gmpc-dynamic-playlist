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

#include "fmSong.h"
#include <gmpc/gmpc_easy_download.h>

#define LASTFM_API_KEY "943e12ac88c773ce892a9f638be8cb84"
#define LASTFM_API_ROOT "http://ws.audioscrobbler.com/2.0/"

fmSong* new_fmSong(xmlChar* l_artist, xmlChar* l_title/*, xmlChar* l_match,*/)
{
	fmSong* ret = g_slice_new(fmSong);
	ret->artist = l_artist;
	ret->title = l_title;
//	ret->match = l_match;

	return ret;
}

void free_fmSong(fmSong* l_song)
{
	g_assert(l_song->artist != NULL);

	xmlFree(l_song->artist);
	if(l_song->title != NULL)
		xmlFree(l_song->title);
//	if(l_song->match != NULL)
//		xmlFree(l_song->match);

	g_slice_free(fmSong, l_song);
}

void free_fmList(fmList* l_list)
{
	clear_fmList(l_list, TRUE);
}

void clear_fmList(fmList* l_list, gboolean l_free_list)
{
	g_assert(l_list != NULL);

	fmList* iter;
	for(iter = l_list; iter != NULL; iter = g_slist_next(iter))
		free_fmSong((fmSong*) iter->data);

	if(l_free_list)
		g_slist_free(l_list);
}

static void lastfm_get_artist_callback(const GEADAsyncHandler* l_handle, const GEADStatus l_status, gpointer l_data)
{
	g_assert(l_data != NULL);

	if(l_status == GEAD_DONE)
	{
		goffset length = 0;
		const char* data = gmpc_easy_handler_get_data(l_handle, &length);
		fmList* result = lastfm_get_artist_parse(data, length);
		lastfm_callback cb = (lastfm_callback) l_data;
		cb(result);
	}
	else if(l_status == GEAD_CANCELLED || l_status == GEAD_FAILED)
	{
		lastfm_callback cb = (lastfm_callback) l_data;
		cb(NULL);
	}
}

void lastfm_get_artist_async(lastfm_callback l_callback, const gchar* l_artist, gint l_limit)
{
	g_assert(l_callback != NULL && l_artist != NULL);

	gchar* artist = gmpc_easy_download_uri_escape(l_artist);
	gchar* furl;
	if(l_limit > 0)
		furl = g_strdup_printf(LASTFM_API_ROOT"?method=artist.getsimilar&limit=%d&artist=%s&api_key=%s", l_limit, artist, LASTFM_API_KEY);
	else
		furl = g_strdup_printf(LASTFM_API_ROOT"?method=artist.getsimilar&artist=%s&api_key=%s", artist, LASTFM_API_KEY);

	gmpc_easy_async_downloader(furl, lastfm_get_artist_callback, l_callback);

	g_free(artist);
	g_free(furl);
}

static void lastfm_get_song_callback(const GEADAsyncHandler* l_handle, const GEADStatus l_status, gpointer l_data)
{
	g_assert(l_handle != NULL && l_data != NULL);

	if(l_status == GEAD_DONE)
	{
		goffset length = 0;
		const char* data = gmpc_easy_handler_get_data(l_handle, &length);
		fmList* result = lastfm_get_song_parse(data, length);
		lastfm_callback cb = (lastfm_callback) l_data;
		cb(result);
	}
	else if(l_status == GEAD_CANCELLED || l_status == GEAD_FAILED)
	{
		lastfm_callback cb = (lastfm_callback) l_data;
		cb(NULL);
	}
}

void lastfm_get_song_async(lastfm_callback l_callback, const gchar* l_artist, const gchar* l_title)
{
	g_assert(l_callback != NULL && l_artist != NULL && l_title != NULL);

	gchar* artist = gmpc_easy_download_uri_escape(l_artist);
	gchar* title =  gmpc_easy_download_uri_escape(l_title);
	gchar* furl = g_strdup_printf(LASTFM_API_ROOT"?method=track.getsimilar&artist=%s&track=%s&api_key=%s", artist, title, LASTFM_API_KEY);

	gmpc_easy_async_downloader(furl, lastfm_get_song_callback, l_callback);

	g_free(artist);
	g_free(title);
	g_free(furl);
}

/* Modified from gmpc-last.fm-plugin (Qball Cow)
 * ***************************************************************************************/
xmlNodePtr get_first_node_by_name(xmlNodePtr l_xml, const gchar* l_name)
{
	if(l_name != NULL && l_xml != NULL)
	{
		xmlNodePtr child = l_xml->xmlChildrenNode;
		for(; child != NULL; child = child->next)
		{
			if(child->name != NULL && xmlStrEqual(child->name, (xmlChar*) l_name))
				return child;
		}
	}

	return NULL;
}

fmList* lastfm_get_artist_parse(const gchar* l_data, gint l_size)
{
	if(l_size <= 0 || l_data == NULL || l_data[0] != '<')
		return NULL;

	fmList* ret = NULL;
	xmlDocPtr doc = xmlParseMemory(l_data, l_size);
	if(doc != NULL)
	{
		xmlNodePtr root = xmlDocGetRootElement(doc);
		xmlNodePtr cur = get_first_node_by_name(root, "similarartists");
		if(cur != NULL)
		{
			xmlNodePtr cur2 = cur->xmlChildrenNode;
			for(; cur2 != NULL; cur2 = cur2->next)
			{
				if(xmlStrEqual(cur2->name, (xmlChar*) "artist"))
				{
					xmlChar* artist = NULL;
//					xmlChar* match = NULL;

					xmlNodePtr cur3 = cur2->xmlChildrenNode;
					for(; cur3 != NULL; cur3 = cur3->next)
					{
						if(xmlStrEqual(cur3->name, (xmlChar*) "name"))
							artist = xmlNodeGetContent(cur3);
//						else if(xmlStrEqual(cur3->name, (xmlChar*) "match"))
//							match = xmlNodeGetContent(cur3);
					}

					if(artist != NULL)
						ret = g_slist_prepend(ret, new_fmSong(artist, NULL/*, match*/));
//					else if(match != NULL) // free allocated match if artist is NULL
//						xmlFree(match);
				}
			}
			ret = g_slist_reverse(ret); // to have the match-order
		}
		xmlFreeDoc(doc);
	}

	return ret;
}

fmList* lastfm_get_song_parse(const gchar* l_data, gint l_size)
{
	if(l_size <= 0 || l_data == NULL || l_data[0] != '<')
		return NULL;

	fmList* ret = NULL;
	xmlDocPtr doc = xmlParseMemory(l_data, l_size);
	if(doc != NULL)
	{
		xmlNodePtr root = xmlDocGetRootElement(doc);
		xmlNodePtr cur = get_first_node_by_name(root, "similartracks");
		if(cur != NULL)
		{
			xmlNodePtr cur2 = cur->xmlChildrenNode;
			for(; cur2 != NULL; cur2 = cur2->next)
			{
				if(xmlStrEqual(cur2->name, (xmlChar*) "track"))
				{
					xmlChar* artist = NULL;
					xmlChar* title = NULL;
//					xmlChar* match = NULL;

					xmlNodePtr cur3 = cur2->xmlChildrenNode;
					for(; cur3 != NULL; cur3 = cur3->next)
					{
						if(xmlStrEqual(cur3->name, (xmlChar*) "name"))
							title = xmlNodeGetContent(cur3);
						else if(xmlStrEqual(cur3->name, (xmlChar*) "artist"))
						{
							xmlNodePtr cur4 = get_first_node_by_name(cur3, "name");
							if(cur4 != NULL)
								artist = xmlNodeGetContent(cur4);
						}
//						else if(xmlStrEqual(cur3->name, (xmlChar*) "match"))
//							match = xmlNodeGetContent(cur3);
					}

					if(artist != NULL || title != NULL)
						ret = g_slist_prepend(ret, new_fmSong(artist, title/*, match*/));
//					else if(match != NULL) // free allocated match if artist and title are NULL
//						xmlFree(match);
				}
			}
			ret = g_slist_reverse(ret); // to have the match-order
		}
		xmlFreeDoc(doc);
	}

	return ret;
}
/* ***************************************************************************************/

/* vim:set ts=4 sw=4: */
