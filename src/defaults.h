/* gmpc-dynamic-playlist (GMPC plugin)
 * Copyright (C) 2009-2013 Andre Klitzing <andre@incubo.de>
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

#ifndef _DYN_LIST_PLUGIN_DEFAULTS
#define _DYN_LIST_PLUGIN_DEFAULTS

/*
 * This default values should be assignment by the build process.
 * This file exists for IDEs only and should not by used for releases!
 */

#ifndef VERSION_MAJOR
#define VERSION_MAJOR 0
#endif

#ifndef VERSION_MINOR
#define VERSION_MINOR 0
#endif

#ifndef VERSION_PATCH
#define VERSION_PATCH 0
#endif

#ifndef GETTEXT_PACKAGE
#define GETTEXT_PACKAGE "gmpc-dynamic-playlist"
#endif

#ifndef PACKAGE_LOCALE_DIR
#define PACKAGE_LOCALE_DIR "/usr/local/share/locale"
#endif

#ifndef UI_OPTIONS
#define UI_OPTIONS "/usr/local/lib/gmpc/plugins/dynlist-ui-options.xml"
#endif

#ifndef MPD_BINARY
#define MPD_BINARY "/usr/bin/mpd"
#endif

#ifndef CONFIG
#define CONFIG "played/config"
#endif

#ifndef CONFIG_EMPTY
#define CONFIG_EMPTY "blacklist/empty/config"
#endif

#ifndef CONFIG_BL_ALL
#define CONFIG_BL_ALL "database/blacklist/all/config"
#endif

#endif

/* vim:set ts=4 sw=4: */
