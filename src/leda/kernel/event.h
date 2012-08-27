#ifndef _EVENT_H_
#define _EVENT_H_

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct _element* element;

typedef struct event_data {
	size_t n; //number of elements
	element payload;
} * event;

event extract_event_from_lua_state(lua_State *L, int from, int args);
void copy_values_directly (lua_State *dst, lua_State *src, int from, int args);
int restore_event_to_lua_state(lua_State * L, event *e);
void dump_event(lua_State *L, event e);
void destroy_event(event e);
void event_init(int daemon_fd);
int send_event(lua_State *L);

#endif //_EVENT_H_
