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

#include "icon.h"
#include "search.h"
#include "plugin.h"
#include <glib/gi18n-lib.h>
#include <gmpc/status_icon.h>
#include "defaults.h"

static GtkWidget* m_box = NULL;
static GtkWidget* m_image = NULL;

gboolean icon_clicked(G_GNUC_UNUSED GtkWidget* l_widget, GdkEventButton* l_event, G_GNUC_UNUSED gpointer l_data)
{
	if(l_event->button == 1) // left mouse button
		set_search_active(!get_search_active());
	else if(l_event->button == 2) // middle mouse button
		search_easy();
	else if(l_event->button == 3) // right mouse button
		g_debug("todo: open popup menu");
	else
		return FALSE;

	return TRUE;
}

static void refresh_icon_state()
{
	g_assert(m_image != NULL);
	gtk_widget_set_sensitive(m_image, get_search_active());
}

gboolean icon_integration(G_GNUC_UNUSED gpointer l_data)
{
	g_assert(m_box == NULL);
	g_assert(m_image == NULL);

	m_box = gtk_event_box_new();
	m_image = gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU);
	gtk_container_add(GTK_CONTAINER(m_box), m_image);
	gtk_widget_set_tooltip_text(m_box, _("Dynamic Playlist"));
	g_signal_connect(G_OBJECT(m_box), "button-release-event", G_CALLBACK(icon_clicked), NULL);

	refresh_icon_state();
	gtk_widget_show_all(m_box);
	main_window_add_status_icon(m_box);

	return TRUE;
}

gboolean is_icon_added()
{
	return m_box != NULL;
}

gboolean is_grayed_out()
{
	g_assert(m_image != NULL);
	return gtk_widget_get_sensitive(m_image);
}

void add_icon()
{
	icon_integration(NULL);
}

void remove_icon()
{
	g_assert(m_box != NULL);
	g_assert(m_image != NULL);

	gtk_widget_destroy(m_box);
	m_box = NULL;
	m_image = NULL;
}

void reload_icon()
{
	if(dyn_get_enabled())
	{
		if(is_icon_added())
			refresh_icon_state();
		else
			add_icon();
	}
	else if(is_icon_added())
		remove_icon();
}

void init_icon()
{
	icon_integration(NULL);
}

/* vim:set ts=4 sw=4: */
