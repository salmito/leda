#ifndef _LEDA_IO_H_
#define _LEDA_IO_H_

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

int leda_wrap_io(lua_State *L);
int leda_unwrap_io(lua_State *L);

int leda_wrap_sock(lua_State *L);
int leda_unwrap_sock(lua_State *L);

#ifndef _WIN32
int epoll_close(lua_State* L);
int epoll_lwait(lua_State* L);
int epoll_wait_read(lua_State* L);
int epoll_add_write(lua_State *L);
int epoll_add_read(lua_State *L);
int epoll_add_read_write(lua_State *L);
int epoll_remove_descriptor(lua_State *L);
int epoll_lcreate(lua_State *L);
int socket_flush(lua_State *L);
#endif //_WIN32

#endif// _LEDA_IO_H_
