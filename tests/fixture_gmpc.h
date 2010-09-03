#ifndef _DYN_LIST_FIXTURE_GMPC
#define _DYN_LIST_FIXTURE_GMPC

void fake_gmpc_init();
void fake_gmpc_free();

void g_assert_message(const gchar* l_msg);
void g_assert_message_do(const gchar* l_msg, int l_count);

void redirect_log(const gchar* l_domain, GLogLevelFlags l_flags, const gchar* l_message, gpointer l_data);

#endif

/* vim:set ts=4 sw=4: */
