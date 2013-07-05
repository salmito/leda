#ifndef _MUTEX_H_
#define _MUTEX_H_

#include <lua.h>

#define MUTEX_METATABLE   "leda mutex"

int luaopen_leda_mutex(lua_State *L);

#endif //_MUTEX_H_
