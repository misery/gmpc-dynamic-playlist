#include <glib.h>
#include "fixture_gmpc.h"
#include "../src/prefs.h"

gint nextIntValue = 0;
gint nextBoolValue = FALSE;

void assert_bool_func(gboolean l_value)
{
	g_assert_cmpint(nextBoolValue, ==, l_value);
}

void assert_int_func(gint l_value)
{
	g_assert_cmpint(nextIntValue, ==, l_value);
}

void assert_child(GtkWidget* l_container, gint l_size)
{
	GList* list = gtk_container_get_children(GTK_CONTAINER(l_container));
	g_assert_cmpint(l_size, ==, g_list_length(list));
	g_list_free(list);
}



void test_combo_value()
{
	GtkListStore* store = gtk_list_store_new(2, G_TYPE_INT, G_TYPE_STRING);
	gtk_list_store_insert_with_values(store, NULL, 99, 0, 123, 1, "first", -1);
	gtk_list_store_insert_with_values(store, NULL, 100, 0, 987, 1, "second", -1);
	GtkWidget* box = gtk_combo_box_new_with_model(GTK_TREE_MODEL(store));

	nextIntValue = 666;
	gtk_combo_box_set_active(GTK_COMBO_BOX(box), -1);
	pref_combo(GTK_COMBO_BOX(box), assert_int_func);

	nextIntValue = 123;
	gtk_combo_box_set_active(GTK_COMBO_BOX(box), 0);
	pref_combo(GTK_COMBO_BOX(box), assert_int_func);

	nextIntValue = 987;
	gtk_combo_box_set_active(GTK_COMBO_BOX(box), 1);
	pref_combo(GTK_COMBO_BOX(box), assert_int_func);

	gtk_widget_destroy(box);
}

void test_spin_value()
{
	GtkWidget* button = gtk_spin_button_new_with_range(0.0, 1000.0, 1.0);

	nextIntValue = 0;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), nextIntValue);
	pref_spin(GTK_SPIN_BUTTON(button), assert_int_func);

	nextIntValue = 666;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(button), nextIntValue);
	pref_spin(GTK_SPIN_BUTTON(button), assert_int_func);

	gtk_widget_destroy(button);
}

void test_toggle_value()
{
	GtkWidget* button = gtk_toggle_button_new();

	nextBoolValue = FALSE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), nextBoolValue);
	pref_toggle(GTK_TOGGLE_BUTTON(button), assert_bool_func);

	nextBoolValue = TRUE;
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), nextBoolValue);
	pref_toggle(GTK_TOGGLE_BUTTON(button), assert_bool_func);

	gtk_widget_destroy(button);
}

void test_toggle_menu_value()
{
	GtkWidget* item = gtk_check_menu_item_new();

	nextBoolValue = FALSE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), nextBoolValue);
	pref_toggle_menu(GTK_CHECK_MENU_ITEM(item), assert_bool_func);

	nextBoolValue = TRUE;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), nextBoolValue);
	pref_toggle_menu(GTK_CHECK_MENU_ITEM(item), assert_bool_func);

	gtk_widget_destroy(item);
}

void test_destroy_child()
{
	GtkWidget* parent = gtk_button_new();
	assert_child(parent, 0);
	pref_destroy(parent);
	assert_child(parent, 0);

	GtkWidget* child =  gtk_label_new("dummy");
	gtk_container_add(GTK_CONTAINER(parent), child);
	gtk_widget_destroyed(child, &child);

	assert_child(parent, 1);
	pref_destroy(parent);
	assert_child(parent, 0);
	g_assert(child == NULL);

	gtk_widget_destroy(parent);
}

void test_construct()
{
	GtkWidget* parent = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	assert_child(parent, 0);
	pref_construct(parent);
	assert_child(parent, 1);
	g_assert(GTK_IS_NOTEBOOK(gtk_bin_get_child(GTK_BIN(parent))));
	gtk_widget_destroy(parent);
}

int main(int argc, char** argv)
{
	gtk_test_init(&argc, &argv, NULL);

	g_test_add_func("/prefs/combi_value", test_combo_value);
	g_test_add_func("/prefs/spin_value", test_spin_value);
	g_test_add_func("/prefs/toggle_value", test_toggle_value);
	g_test_add_func("/prefs/toggle_menu_value", test_toggle_menu_value);
	g_test_add_func("/prefs/destroy", test_destroy_child);
	g_test_add_func("/prefs/construct", test_construct);

	/* mute standard debug output from plugin */
	g_log_set_handler("dynlist", G_LOG_LEVEL_DEBUG, redirect_log, NULL);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
