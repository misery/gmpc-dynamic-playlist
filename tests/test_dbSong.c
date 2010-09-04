#include <gtk/gtk.h>
#include <glib.h>
#include <stdlib.h>
#include <string.h>
#include "fixture_gmpc.h"
#include "../src/dbSong.h"
#include "../src/fuzzy.h"

strList* createDummyList()
{
	strList* list = new_strListItem(NULL, "dummy1");
	list = new_strListItem(list, "dummy2");
	return new_strListItem(list, "dummy3");
}

void test_free_next_strListItem()
{
	strList* list = createDummyList();
	g_assert_cmpint(g_slist_length(list), ==, 3);

	free_next_strListItem(list);
	g_assert_cmpint(g_slist_length(list), ==, 2);
	g_assert(strcasecmp("dummy3", g_slist_nth_data(list, 0)) == 0);
	g_assert(strcasecmp("dummy1", g_slist_nth_data(list, 1)) == 0);

	free_next_strListItem(list);
	g_assert_cmpint(g_slist_length(list), ==, 1);
	g_assert(strcasecmp("dummy3", g_slist_nth_data(list, 0)) == 0);

	if(g_test_trap_fork(0, G_TEST_TRAP_SILENCE_STDERR))
	{
		free_next_strListItem(list);
		exit(EXIT_SUCCESS);
	}
	g_test_trap_assert_failed();

	free_strList(list);
}

void test_clear_strListItem()
{
	strList* list = createDummyList();
	g_assert_cmpint(g_slist_length(list), ==, 3);

	clear_strListItem(list);
	g_assert_cmpint(g_slist_length(list), ==, 3);
	g_assert(g_slist_nth_data(list, 0) == NULL);
	g_assert(strcasecmp("dummy2", g_slist_nth_data(list, 1)) == 0);
	g_assert(strcasecmp("dummy1", g_slist_nth_data(list, 2)) == 0);

	free_strList(list);
}

void test_exists_strList()
{
	strList* list = createDummyList();

	g_assert(exists_strList(list, "dummy1"));
	g_assert(exists_strList(list, "dummy2"));
	g_assert(exists_strList(list, "dummy3"));
	g_assert(!exists_strList(list, "notInList"));

	free_strList(list);
}

void test_exists_dbList()
{
	fake_gmpc_init();
	dbList* list = g_list_prepend(NULL, new_dbSong("Metallica", "The Unforgiven", "path"));
	list = g_list_prepend(list, new_dbSong("The Offspring", "Self Esteem", "path"));

	g_assert(exists_dbList(list, "Metallica", "The Unforgiven"));
	g_assert(exists_dbList(list, "The Offspring", "Self Esteem"));

	g_assert(!exists_dbList(list, "notInList", "notInList"));
	g_assert(!exists_dbList(list, "Metallica", "TheUnforgiven"));
	g_assert(!exists_dbList(list, "TheOffspring", "Self Esteem"));
	g_assert(!exists_dbList(list, "Metallica", "Self Esteem"));

	free_dbList(list);
	fake_gmpc_free();
}

int main(int argc, char** argv)
{
	gtk_test_init(&argc, &argv, NULL);

	g_test_add_func("/dbSong/free_next_strListItem", test_free_next_strListItem);
	g_test_add_func("/dbSong/clear_strListItem", test_clear_strListItem);
	g_test_add_func("/dbSong/exists_strList", test_exists_strList);
	g_test_add_func("/dbSong/exists_dbList", test_exists_dbList);

	/* mute standard debug output from plugin */
	g_log_set_handler("dynlist", G_LOG_LEVEL_DEBUG, redirect_log, NULL);

	return g_test_run();
}

/* vim:set ts=4 sw=4: */
