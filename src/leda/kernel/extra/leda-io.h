#ifndef _LEDA_IO_H_
#define _LEDA_IO_H_

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

int leda_wrap_io(lua_State *L);
int leda_unwrap_io(lua_State *L);
int socket_flush(lua_State *L);

#endif// _LEDA_IO_H_
