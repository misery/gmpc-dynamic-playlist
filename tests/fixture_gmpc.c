#include <gmpc/gmpc-extras.h>
#include <gmpc/playlist3-messages.h>
#include <gmpc/plugin.h>
#include "fixture_gmpc.h"
#include "../src/plugin.h"
#include "../src/prefs.h"
#include "../src/icon.h"

static GQueue m_queue_msg = G_QUEUE_INIT;
static GQueue m_queue_log = G_QUEUE_INIT;
static GtkMenu* m_menu = NULL;

/* init */
static void clear_queue(GQueue* l_queue)
{
	GList* list;
	for(list = l_queue->head; list != NULL; list = list->next)
		g_free(list->data);
	g_queue_clear(l_queue);
}

static void clear_all_queues()
{
	clear_queue(&m_queue_msg);
	clear_queue(&m_queue_log);
}

static void add_to_queue(GQueue* l_queue, const char* l_msg)
{
	if(l_msg != NULL)
		g_queue_push_tail(l_queue, g_strdup(l_msg));
}

gboolean quit_main()
{
	gtk_main_quit();
	return FALSE;
}

void fake_gmpc_init()
{
	gtk_init_add(quit_main, NULL);
	dyn_init();
	m_menu = GTK_MENU(gtk_menu_new());
	dyn_tool_menu_integration(m_menu);

	gtk_main();
	while(gtk_events_pending())
	{
		gtk_main_iteration();
	}
}

void fake_gmpc_free()
{
	gtk_widget_destroy(GTK_WIDGET(m_menu));
	dyn_destroy();
	clear_all_queues();
	if(is_icon_added())
		remove_icon();
}


/* Config settings */
config_obj* config = NULL;
void cfg_set_single_value_as_int(G_GNUC_UNUSED config_obj* l_cfg, const char* l_class, const char* l_key, int l_value)
{
	g_test_message("set int value: %s | %s | %d", l_class, l_key, l_value);
}

int cfg_get_single_value_as_int_with_default(G_GNUC_UNUSED config_obj* l_cfg, const char* l_class, const char* l_key, int l_def)
{
	g_test_message("get int with default: %s | %s | %d", l_class, l_key, l_def);
	return l_def;
}


/* Metadata */
GmpcMetaWatcher* gmw = NULL;
void gmpc_meta_watcher_get_meta_path_callback(
					G_GNUC_UNUSED GmpcMetaWatcher* l_self,
					G_GNUC_UNUSED mpd_Song* l_song,
					G_GNUC_UNUSED MetaDataType l_type,
					G_GNUC_UNUSED MetaDataCallback l_callback,
					G_GNUC_UNUSED gpointer l_data)
{

}

const GList* meta_data_get_text_list(G_GNUC_UNUSED const MetaData* l_data)
{
	return NULL;
}


/* Messages */
void playlist3_show_error_message(const gchar* l_message, ErrorLevel l_level)
{
	g_test_message("show message: %s | %d", l_message, l_level);
	add_to_queue(&m_queue_msg, l_message);
}

void g_assert_queue(GQueue* l_queue, const gchar* l_msg, int l_count)
{
	g_assert(l_msg != NULL);

	int i = 0;
	GList* list;
	for(list = l_queue->tail; i < l_count && list != NULL; list = list->prev)
	{
		if(strcasecmp(l_msg, (gchar*)list->data) == 0)
			return;
		++i;
	}

	g_message("Message expected but not found: '%s'", l_msg);
	g_assert_not_reached();
}

void g_assert_message(const gchar* l_msg)
{
	g_assert_queue(&m_queue_msg, l_msg, 1);
}

void g_assert_log(const gchar* l_msg)
{
	g_assert_queue(&m_queue_log, l_msg, 1);
}

void redirect_log(const gchar* l_domain, GLogLevelFlags l_flags, const gchar* l_message, G_GNUC_UNUSED gpointer l_data)
{
	g_test_message("redirected: %s | domain: %s | flag: %d", l_message, l_domain, l_flags);
	add_to_queue(&m_queue_log, l_message);
}


/* Gmpc Easy Command */
GmpcEasyCommand* gmpc_easy_command = 0;
guint gmpc_easy_command_add_entry(
					G_GNUC_UNUSED GmpcEasyCommand* l_self,
					const char* l_name,
					const char* l_pattern,
					const char* l_hint,
					G_GNUC_UNUSED GmpcEasyCommandCallback* l_callback,
					G_GNUC_UNUSED gpointer l_data)
{
	g_test_message("Add easy command '%s' with pattern '%s' and hint '%s'", l_name, l_pattern, l_hint);
	return 0;
}

/* Icon */
void main_window_add_status_icon(GtkWidget* l_icon)
{
	g_assert(l_icon != NULL);
}

/* vim:set ts=4 sw=4: */
