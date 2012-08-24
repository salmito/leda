#ifndef _STATS_H_
#define _STATS_H_

#include "lua.h"

#ifdef STATS_OFF
   #define STATS_UPDATE_TIME(id,t)
   #define STATS_UPDATE_EVENTS(id,e)
   #define STATS_ACTIVE(id)
   #define STATS_PUSH(L) lua_pushnil(L)
#else
   #define STATS_ACTIVE(id) stats_active_instance(id)
   #define STATS_UPDATE_TIME(id,t) stats_update_time(id,t)
   #define STATS_UPDATE_EVENTS(id,e) stats_update_events(id,e)
   #define STATS_PUSH(L) stats_push(L)
#endif

void stats_active_instance(int id);
void stats_reset(int id);
void stats_init(int n);
void stats_update_time(int id, int t);
void stats_update_events(int id, int e);
void stats_push(lua_State * L);
void stats_free();

#endif //_STATS_H_
