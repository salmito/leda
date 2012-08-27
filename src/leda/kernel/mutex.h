#ifndef _MUTEX_H_
#define _MUTEX_H_

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#define MUTEX_METATABLE   "leda mutex"

int mutex_new (lua_State *L);
int mutex_destroy (lua_State *L);
int mutex_lock (lua_State *L);
int mutex_unlock (lua_State *L);
int mutex_createmetatable (lua_State *L);

#endif //_MUTEX_H_
