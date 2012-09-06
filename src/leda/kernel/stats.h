#ifndef _STATS_H_
#define _STATS_H_

#include "lua.h"

#ifdef STATS_OFF
   #define STATS_UPDATE_TIME(id,time)
   #define STATS_UPDATE_ERROR(id,e)
   #define STATS_UPDATE_EVENTS(id,e)
   #define STATS_UPDATE_CONNECTOR(id,time)
   #define STATS_ACTIVE(id)
   #define STATS_INACTIVE(id)
   #define STATS_PUSH(L) lua_pushnil(L); lua_pushnil(L)
#else
   #define STATS_ACTIVE(id) stats_active_instance(id)
   #define STATS_INACTIVE(id) stats_inactive_instance(id)
   #define STATS_UPDATE_TIME(id,time) stats_update_time(id,time)
   #define STATS_UPDATE_CONNECTOR(id,time) stats_update_connector(id, time)
   #define STATS_UPDATE_EVENTS(id,e) stats_update_events(id,e)
   #define STATS_UPDATE_ERROR(id,e) stats_update_error(id,e)
   #define STATS_PUSH(L) stats_push(L)
#endif

void stats_active_instance(int id);
void stats_inactive_instance(int id);
int stats_reset(lua_State * L);
void stats_init(int ns, int nc);
void stats_update_time(int id, long int t);
void stats_update_events(int id, int e);
void stats_update_error(int id, int e);
void stats_update_connector(int id, long int t);
void stats_push(lua_State * L);
void stats_free(void);

#endif //_STATS_H_
