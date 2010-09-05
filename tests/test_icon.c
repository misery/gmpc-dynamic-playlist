#include <gtk/gtk.h>
#include "fixture_gmpc.h"
#include "../src/icon.h"
#include "../src/plugin.h"
#include "../src/search.h"

void test_icon_init_and_reload()
{
	fake_gmpc_init();

	g_assert(dyn_get_enabled());
	g_assert(is_icon_added());

	dyn_set_enabled(FALSE);
	g_assert(!is_icon_added());

	dyn_set_enabled(TRUE);
	g_assert(is_icon_added());

	fake_gmpc_free();
}

void test_icon_clicked()
{
	fake_gmpc_init();
	GdkEventButton* event = (GdkEventButton*)gdk_event_new(GDK_BUTTON_RELEASE);
	g_assert(!icon_clicked(NULL, event, NULL));

	event->button = 1;
	set_search_active(FALSE);
	g_assert(icon_clicked(NULL, event, NULL));
	g_assert(get_search_active());
	g_assert(icon_clicked(NULL, event, NULL));
	g_assert(!get_search_active());

	event->button = 2;
	dyn_set_enabled(FALSE);
	g_assert(icon_clicked(NULL, event, NULL));
	g_assert_message("Dynamic playlist is disabled");

	event->button = 3;
	g_assert(icon_clicked(NULL, event, NULL));
	g_assert_log("todo: open popup menu");

	gdk_event_free((GdkEvent*)event);
	fake_gmpc_free();
}

void test_icon_integration()
{
	g_assert(!is_icon_added());
	add_icon();
	g_assert(is_icon_added());
	remove_icon();
	g_assert(!is_icon_added());
}

void test_icon_grayed_out()
{
	fake_gmpc_init();

	set_search_active(FALSE);
	g_assert(!is_grayed_out());

	set_search_active(TRUE);
	g_assert(is_grayed_out());

	fake_gmpc_free();
}

int main(int argc, char** argv)
{
	gtk_test_init(&argc, &argv, NULL);

	g_test_add_func("/icon/init_and_reload", test_icon_init_and_reload);
	g_test_add_func("/icon/clicked", test_icon_clicked);
	g_test_add_func("/icon/integration", test_icon_integration);
	g_test_add_func("/icon/grayed_out", test_icon_grayed_out);

	/* mute standard debug output from plugin */
	g_log_set_handler("dynlist", G_LOG_LEVEL_DEBUG, redirect_log, NULL);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
