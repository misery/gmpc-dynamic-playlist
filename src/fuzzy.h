#ifndef _DYN_LIST_PLUGIN_FUZZY
#define _DYN_LIST_PLUGIN_FUZZY

#include <glib.h>

void init_fuzzy();
gboolean fuzzy_match_artist(const gchar* l_search, const gchar* l_text);
gboolean fuzzy_match_title(const gchar* l_search, const gchar* l_text);
gboolean fuzzy_match_value(const gchar* l_search, const gchar* l_text, gint8 l_value);

void set_fuzzy_artist(gint8 l_value);
gint8 get_fuzzy_artist();
void set_fuzzy_title(gint8 l_value);
gint8 get_fuzzy_title();

#endif

/* vim:set ts=4 sw=4: */
