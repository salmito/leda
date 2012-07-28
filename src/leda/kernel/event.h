#ifndef _EVENT_H_
#define _EVENT_H_

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef struct event_data {
	size_t n;
	element * elements;
} * event;

int restore_event_to_lua_state(lua_State * L, event e);
event extract_event_from_lua_state(lua_State *L, int from, int to);

event new_event(size_t n);
void destroy_event(event e);
#endif //_EVENT_H_
