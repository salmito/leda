/*
===============================================================================

Copyright (C) 2012 Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

===============================================================================
*/
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "leda-io.h"

#define tofile(L,i)	((FILE **)luaL_checkudata(L, i, LUA_FILEHANDLE))

int leda_wrap_io(lua_State *L) {  
   FILE ** f=tofile(L,1);
   int fd=fileno(*f);
   int newfd=dup(fd);
   lua_pushinteger(L,newfd);
   return 1;
}

int leda_unwrap_io(lua_State *L) {
   int fd = luaL_checkint(L, 1);
   const char * mode="rw";
   if(lua_type(L,2)==LUA_TSTRING) {
      mode=lua_tostring(L, 2);
   }
   FILE **f = (FILE **)lua_newuserdata(L, sizeof(FILE *));
   *f = NULL;
   luaL_getmetatable(L, LUA_FILEHANDLE);
   lua_setmetatable(L, -2);
   *f = fdopen(fd, mode);
   return (*f != NULL);   
}

int socket_flush(lua_State *L) {
   int fd=lua_tointeger(L,1);
   int nfd=dup(fd);
   shutdown(nfd,SHUT_RDWR);
   close(nfd);
   return 0;
}
