#include <glib.h>
#include <gmpc/playlist3-messages.h>
#include <gmpc/gmpc-easy-command.h>
#include <gmpc/plugin.h>


/* Config settings */
config_obj* config = NULL;
void cfg_set_single_value_as_int(config_obj *cfg, const char* l_class, const char* l_key, int l_value)
{
	g_debug("set int value: %s | %s | %d", l_class, l_key, l_value);
}

int cfg_get_single_value_as_int_with_default(config_obj* l_cfg, const char* l_class, const char* l_key, int l_def)
{
	g_debug("get int with default: %s | %s | %d", l_class, l_key, l_def);
	return l_def;
}


/* Metadata */
GmpcMetaWatcher* gmw = NULL;
void gmpc_meta_watcher_get_meta_path_callback(GmpcMetaWatcher* l_self,
					mpd_Song* l_song,
					MetaDataType l_type,
					MetaDataCallback l_callback,
					gpointer l_data)
{

}

const GList* meta_data_get_text_list(const MetaData* l_data)
{
	return NULL;
}


/* Messages */
void playlist3_show_error_message(const gchar* l_message, ErrorLevel l_level)
{
	g_debug("show message: %s | %d", l_message, l_level);
}


/* Gmpc Easy Command */
GmpcEasyCommand* gmpc_easy_command = 0;
guint gmpc_easy_command_add_entry(GmpcEasyCommand* l_self,
					const char* l_name,
					const char* l_pattern,
					const char* l_hint,
					GmpcEasyCommandCallback* l_callback,
					gpointer l_data)
{
	return 0;
}

/* Icon */
void main_window_add_status_icon(GtkWidget* l_icon)
{

}

/* vim:set ts=4 sw=4: */
