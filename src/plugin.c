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

#include <gmpc/playlist3-messages.h>
#include <gmpc/gmpc-easy-command.h>
#include <glib/gi18n-lib.h>
#include "plugin.h"
#include "database.h"
#include "blacklist.h"
#include "played.h"
#include "search.h"
#include "icon.h"
#include "prune.h"
#include "prefs.h"

extern GmpcEasyCommand* gmpc_easy_command;

static gboolean m_enabled = TRUE;
GRand* m_rand = NULL;


void dyn_changed_status(MpdObj* l_mi, ChangedStatusType l_what, void* l_userdata)
{
	if(!m_enabled)
		return;

	if(l_what & MPD_CST_PLAYLIST || l_what & MPD_CST_SONGPOS ||
			(l_what & MPD_CST_STATE && mpd_player_get_state(connection) == MPD_PLAYER_PLAY))
	{
		dyn_check_search(FALSE);
	}

	if(l_what & MPD_CST_STORED_PLAYLIST)
		reload_blacklists();

	if(l_what & MPD_CST_STATE && is_search_delayed() && mpd_player_get_state(connection) == MPD_PLAYER_STOP)
		reset_search_delay();
}

gboolean dyn_check_search(gboolean l_force_no_delay)
{
	mpd_Song* curSong = mpd_playlist_get_current_song(connection);
	if(curSong != NULL)
	{
		const gint curPos = curSong->pos;
		if(get_search_active())
		{
			const gint remains = mpd_playlist_get_playlist_length(connection) - curPos - 1;
			search(curSong, remains, l_force_no_delay);
		}

		prune_playlist(curPos);
	}

	return FALSE; // g_idle or g_timeout should not recall
}

void dyn_init()
{
#ifdef HG_REV
	g_debug("Revision: %s", HG_REV);
#endif

	g_assert(m_rand == NULL);

	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");

	m_rand = g_rand_new();
	m_enabled = cfg_get_single_value_as_int_with_default(config, "dynamic-playlist", "enable", TRUE);

	init_icon();
	init_prune();
	init_search();
	init_blacklists();
	init_played_list();

	/* GmpcEasyCommand */
	gmpc_easy_command_add_entry(
			gmpc_easy_command, _("prune"), "[0-9]*",  _("Prune playlist"),
			(GmpcEasyCommandCallback*) prune_playlist_easy, NULL);
	gmpc_easy_command_add_entry(
			gmpc_easy_command, _("dynamic"), "(on|off|)",  _("Dynamic search (on|off)"),
			(GmpcEasyCommandCallback*) set_search_active_easy, NULL);
	gmpc_easy_command_add_entry(
			gmpc_easy_command, _("similar"), "",  _("Search for similar song/artist/genre"),
			(GmpcEasyCommandCallback*) search_easy, NULL);
}

void dyn_destroy()
{
	free_played_list();
	free_blacklists();
	g_rand_free(m_rand);
	m_rand = NULL;
}

gboolean dyn_get_enabled()
{
	return m_enabled;
}

void dyn_set_enabled(gboolean l_enabled)
{
	if(m_enabled == l_enabled)
		return;

	if(!m_enabled && l_enabled)
		reload_blacklists();

	if(!l_enabled)
		reset_search_delay();

	m_enabled = l_enabled;
	cfg_set_single_value_as_int(config, "dynamic-playlist", "enable", m_enabled);
	reload_menu_list();
	reload_icon();
}

const gchar* dyn_get_translation_domain()
{
	return GETTEXT_PACKAGE;
}

gmpcPrefPlugin dyn_pref =
{
	.construct = pref_construct,
	.destroy   = pref_destroy
};

gint plugin_api_version = PLUGIN_API_VERSION;

gmpcPlugin plugin = {
	.name               = N_("Dynamic Playlist"),
	.version            = {VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH},
	.plugin_type        = GMPC_PLUGIN_NO_GUI,
	.init               = dyn_init,
	.destroy            = dyn_destroy,
	.mpd_status_changed = dyn_changed_status,
	.pref               = &dyn_pref,
	.get_enabled        = dyn_get_enabled,
	.set_enabled        = dyn_set_enabled,
	.tool_menu_integration = dyn_tool_menu_integration,
	.get_translation_domain = dyn_get_translation_domain
};

/* vim:set ts=4 sw=4: */
