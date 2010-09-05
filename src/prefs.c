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

#include "prefs.h"
#include "plugin.h"
#include "search.h"
#include "played.h"
#include "prune.h"
#include "blacklist.h"

/* Menu */
static GtkWidget* m_menu_item = NULL;
static GtkWidget* m_menu = NULL;
static GtkWidget* m_menu_search = NULL;
static GtkWidget* m_menu_blacklist = NULL;

void pref_destroy(GtkWidget* l_con)
{
	GtkWidget* child = gtk_bin_get_child(GTK_BIN(l_con));
	if(child)
		gtk_widget_destroy(child);
}

void pref_construct(GtkWidget* l_con)
{
	GtkBuilder* builder = gtk_builder_new();
	gtk_builder_set_translation_domain(builder, dyn_get_translation_domain());
	GError* err = NULL;

	if(gtk_builder_add_from_file(builder, UI_OPTIONS, &err))
	{
		GtkWidget* notebook = GTK_WIDGET(gtk_builder_get_object(builder, "ui-options"));
		pref_construct_signals_and_values(builder);
		gtk_container_add(GTK_CONTAINER(l_con), notebook);
	}
	else
	{
		g_warning("Cannot construct option page: %s", err->message);
		g_error_free(err);
	}
}

void pref_construct_signals_and_values(GtkBuilder* l_builder)
{
	GtkToggleButton* check;
	GtkSpinButton* spin;
	GtkComboBox* combo;

	/* Local ----------------------------------------------------------------------------------- */
	check = GTK_TOGGLE_BUTTON(gtk_builder_get_object(l_builder, "dynamic_search"));
	gtk_toggle_button_set_active(check, get_search_active());
	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(pref_toggle), set_search_active);

	check = GTK_TOGGLE_BUTTON(gtk_builder_get_object(l_builder, "use_blacklists"));
	gtk_toggle_button_set_active(check, get_active_blacklist());
	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(pref_toggle), set_active_blacklist);

	check = GTK_TOGGLE_BUTTON(gtk_builder_get_object(l_builder, "search_genre"));
	gtk_toggle_button_set_active(check, get_local_search_genre());
	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(pref_toggle), set_local_search_genre);

	combo = GTK_COMBO_BOX(gtk_builder_get_object(l_builder, "search_genre_style"));
	gtk_combo_box_set_active(combo, get_local_search_genre_style());
	g_signal_connect(G_OBJECT(combo), "changed", G_CALLBACK(pref_combo), set_local_search_genre_style);

	check = GTK_TOGGLE_BUTTON(gtk_builder_get_object(l_builder, "search_comment"));
	gtk_toggle_button_set_active(check, get_local_search_comment());
	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(pref_toggle), set_local_search_comment);

	spin = GTK_SPIN_BUTTON(gtk_builder_get_object(l_builder, "spin_queue"));
	gtk_spin_button_set_value(spin, get_queue_songs());
	g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(pref_spin), set_queue_songs);

	spin = GTK_SPIN_BUTTON(gtk_builder_get_object(l_builder, "spin_delay"));
	gtk_spin_button_set_value(spin, get_delay_time());
	g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(pref_spin), set_delay_time);

	spin = GTK_SPIN_BUTTON(gtk_builder_get_object(l_builder, "spin_keep"));
	gtk_spin_button_set_value(spin, get_prune_value());
	g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(pref_spin), set_prune_value);

	spin = GTK_SPIN_BUTTON(gtk_builder_get_object(l_builder, "spin_block_song"));
	gtk_spin_button_set_value(spin, get_played_limit_song());
	g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(pref_spin), set_played_limit_song);

	spin = GTK_SPIN_BUTTON(gtk_builder_get_object(l_builder, "spin_block_artist"));
	gtk_spin_button_set_value(spin, get_played_limit_artist());
	g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(pref_spin), set_played_limit_artist);


	/* Metadata -------------------------------------------------------------------------------- */
	check = GTK_TOGGLE_BUTTON(gtk_builder_get_object(l_builder, "song_toggle"));
	gtk_toggle_button_set_active(check, get_search_song());
	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(pref_toggle), set_search_song);

	check = GTK_TOGGLE_BUTTON(gtk_builder_get_object(l_builder, "artist_toggle"));
	gtk_toggle_button_set_active(check, get_search_artist());
	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(pref_toggle), set_search_artist);

	check = GTK_TOGGLE_BUTTON(gtk_builder_get_object(l_builder, "artist_same_toggle"));
	gtk_toggle_button_set_active(check, get_search_artist_same());
	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(pref_toggle), set_search_artist_same);

	check = GTK_TOGGLE_BUTTON(gtk_builder_get_object(l_builder, "genre_toggle"));
	gtk_toggle_button_set_active(check, get_search_genre());
	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(pref_toggle), set_search_genre);

	check = GTK_TOGGLE_BUTTON(gtk_builder_get_object(l_builder, "genre_same_toggle"));
	gtk_toggle_button_set_active(check, get_search_genre_same());
	g_signal_connect(G_OBJECT(check), "toggled", G_CALLBACK(pref_toggle), set_search_genre_same);

	spin = GTK_SPIN_BUTTON(gtk_builder_get_object(l_builder, "spin_song"));
	gtk_spin_button_set_value(spin, get_search_song_max());
	g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(pref_spin), set_search_song_max);

	spin = GTK_SPIN_BUTTON(gtk_builder_get_object(l_builder, "spin_artist"));
	gtk_spin_button_set_value(spin, get_search_artist_max());
	g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(pref_spin), set_search_artist_max);

	spin = GTK_SPIN_BUTTON(gtk_builder_get_object(l_builder, "spin_genre"));
	gtk_spin_button_set_value(spin, get_search_genre_max());
	g_signal_connect(G_OBJECT(spin), "value-changed", G_CALLBACK(pref_spin), set_search_genre_max);
}

void pref_combo(GtkComboBox* l_combo, Combo l_func)
{
	GtkTreeIter iter;
	GtkListStore* store = GTK_LIST_STORE(gtk_combo_box_get_model(l_combo));
	if(gtk_combo_box_get_active_iter(l_combo, &iter))
	{
		gint value;
		gtk_tree_model_get(GTK_TREE_MODEL(store), &iter, 0, &value, -1);
		l_func(value);
	}
}

void pref_toggle(GtkToggleButton* l_button, Toggle l_func)
{
	l_func( gtk_toggle_button_get_active(l_button) );
}

void pref_toggle_menu(GtkCheckMenuItem* l_item, Toggle l_func)
{
	l_func( gtk_check_menu_item_get_active(l_item) );
}

void pref_spin(GtkSpinButton* l_button, Spin l_func)
{
	l_func( gtk_spin_button_get_value_as_int(l_button) );
}

int dyn_tool_menu_integration(GtkMenu* l_menu)
{
	m_menu_item = gtk_image_menu_item_new_with_label(_("Dynamic Playlist"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(m_menu_item), gtk_image_new_from_stock(GTK_STOCK_INDEX, GTK_ICON_SIZE_MENU));
	m_menu = gtk_menu_new();
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(m_menu_item), m_menu);
	gtk_menu_shell_append(GTK_MENU_SHELL(l_menu), m_menu_item);

	m_menu_search = gtk_check_menu_item_new_with_label(_("Dynamic search"));
	g_signal_connect(G_OBJECT(m_menu_search), "activate", G_CALLBACK(pref_toggle_menu), set_search_active);
	gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), m_menu_search);

	m_menu_blacklist = gtk_check_menu_item_new_with_label(_("Use blacklists"));
	g_signal_connect(G_OBJECT(m_menu_blacklist), "activate", G_CALLBACK(pref_toggle_menu), set_active_blacklist);
	gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), m_menu_blacklist);

	GtkWidget* menu_add_song = gtk_image_menu_item_new_with_label(_("Add similar song"));
	gtk_image_menu_item_set_image(GTK_IMAGE_MENU_ITEM(menu_add_song), gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU));
	g_signal_connect(G_OBJECT(menu_add_song), "activate", G_CALLBACK(search_easy), NULL);
	gtk_menu_shell_append(GTK_MENU_SHELL(m_menu), menu_add_song);

	reload_menu_list();
	return 1;
}

void reload_menu_list()
{
	gtk_widget_set_sensitive(m_menu_item, dyn_get_enabled());
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_menu_search), get_search_active());
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_menu_blacklist), get_active_blacklist());
}

/* vim:set ts=4 sw=4: */
