#ifndef _DYN_LIST_PLUGIN_PREF
#define _DYN_LIST_PLUGIN_PREF

#include <gtk/gtk.h>
#include <glib/gi18n-lib.h>

typedef void (*Combo)(gint l_value);
typedef void (*Toggle)(gboolean l_value);
typedef void (*Spin)(gint l_value);

void pref_destroy(GtkWidget* l_con);
void pref_construct(GtkWidget* l_con);
void pref_construct_signals_and_values(GtkBuilder* l_builder);
void pref_combo(GtkComboBox* l_combo, Combo l_func);
void pref_toggle(GtkToggleButton* l_button, Toggle l_func);
void pref_toggle_menu(GtkCheckMenuItem* l_item, Toggle l_func);
void pref_spin(GtkSpinButton* l_button, Spin l_func);

int dyn_tool_menu_integration(GtkMenu* l_menu);
void reload_menu_list();

#endif

/* vim:set ts=4 sw=4: */
