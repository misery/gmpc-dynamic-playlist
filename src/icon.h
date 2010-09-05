#ifndef _DYN_LIST_PLUGIN_ICON
#define _DYN_LIST_PLUGIN_ICON

#include <gtk/gtk.h>

gboolean icon_clicked(GtkWidget* l_widget, GdkEventButton* l_event, gpointer l_data);
gboolean is_icon_added();
gboolean is_grayed_out();
void init_icon();
void add_icon();
void remove_icon();
void reload_icon();

#endif

/* vim:set ts=4 sw=4: */
