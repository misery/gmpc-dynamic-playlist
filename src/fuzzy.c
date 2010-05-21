/* gmpc-dynamic-playlist (GMPC plugin)
 * Copyright (C) 2010 Andre Klitzing <andre@incubo.de>
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

#include <gmpc/plugin.h>
#include <string.h>
#include "fuzzy.h"

static gint8 m_artist = 0;
static gint8 m_title = 0;

void init_fuzzy()
{
	m_artist = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "fuzzy_artist", 80);
	m_title = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "fuzzy_title", 80);
}

static inline gboolean fuzzy_exact_compare(const gchar* l_first, const gchar* l_second)
{
	return strcasecmp(l_first, l_second) == 0;
}

static gint8 fuzzy_match(const gchar* l_search, const gchar* l_text)
{
	/* TODO: Add a fuzzy string matching algorithm */

	return 0;
}

gboolean fuzzy_match_value(const gchar* l_search, const gchar* l_text, gint8 l_value)
{
	g_assert(l_value <= 100 && l_value >= 0);
	g_assert(l_search != NULL);
	g_assert(l_text != NULL);

	return fuzzy_exact_compare(l_search, l_text) || fuzzy_match(l_search, l_text) >= l_value;
}

gboolean fuzzy_match_artist(const gchar* l_search, const gchar* l_text)
{
	return fuzzy_match_value(l_search, l_text, m_artist);
}

gboolean fuzzy_match_title(const gchar* l_search, const gchar* l_text)
{
	return fuzzy_match_value(l_search, l_text, m_title);
}

void set_fuzzy_artist(gint8 l_value)
{
	m_artist = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "fuzzy_artist", m_artist);
}

gint8 get_fuzzy_artist()
{
	return m_artist;
}

void set_fuzzy_title(gint8 l_value)
{
	m_title = l_value;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "fuzzy_title", m_title);
}

gint8 get_fuzzy_title()
{
	return m_title;
}

/* vim:set ts=4 sw=4: */
