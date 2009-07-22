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
#include "plugin.h"

GtkWidget* m_box = NULL;

gboolean icon_clicked(GtkWidget* l_widget, GdkEventButton* l_event, gpointer l_data)
{
	if(l_event->button == 1) // left mouse button
		pref_similar_set(similar_search, !enabled_search());
	else if(l_event->button == 2) // middle mouse button
		findSimilar_easy();
	else if(l_event->button == 3) // right mouse button
		g_debug("todo: open popup menu");
	else
		return FALSE;

	return TRUE;
}

gboolean icon_integration(gpointer l_data)
{
	g_assert(m_box == NULL);

	m_box = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(m_box), gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU));
	g_signal_connect(G_OBJECT(m_box), "button-release-event", G_CALLBACK(icon_clicked), NULL);
	gtk_widget_show_all(m_box);
	main_window_add_status_icon(m_box);

	return TRUE;
}

void init_icon()
{
	gtk_init_add(icon_integration, NULL);
}

/* vim:set ts=4 sw=4: */
