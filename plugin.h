#ifndef _DYN_LIST_PLUGIN
#define _DYN_LIST_PLUGIN

#include "dbSong.h"
#include <gmpc/plugin.h>
#include <libmpd/libmpd.h>

void dyn_changed_status(MpdObj* l_mi, ChangedStatusType l_what, void* l_userdata);
void dyn_init();
void dyn_destroy();
void dyn_set_enabled(gboolean l_enabled);
gboolean dyn_get_enabled();
const gchar* dyn_get_translation_domain();

#endif

/* vim:set ts=4 sw=4: */
