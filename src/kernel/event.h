#ifndef _EVENT_H_
#define _EVENT_H_

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "instance.h"

typedef struct _element* element;

typedef struct event_data {
	size_t n; //number of elements
	element payload;
	char packed;
	char * data;
	size_t data_len;
} * event;

event event_new_packed_event(char * data,size_t len);
event extract_event_from_lua_state(lua_State *L, int from, int args);
void copy_values_directly (lua_State *dst, lua_State *src, int from, int args);
int restore_event_to_lua_state(lua_State * L, event *e);
void dump_event(lua_State *L, event e);
void destroy_event(event e);
void event_init_t(int process_fd);
//void leda_event_end();
void leda_event_end_t();
int send_async_event(instance i, stage_id s_id, int con_id, time_d comunication_time, size_t len ,const char * payload);
int send_sync_event(lua_State *L);
int leda_gettime(lua_State *L);

#endif //_EVENT_H_
