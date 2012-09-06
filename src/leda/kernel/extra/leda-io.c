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

int leda_wrap_sock(lua_State *L) {
   luaL_checktype(L,1,LUA_TUSERDATA);
   lua_getfield(L,1,"getfd");
   if(lua_type(L,-1)!=LUA_TFUNCTION) {
      luaL_error(L,"Parameter does not appears to be a socket");
   }
   lua_pushvalue(L,1);
   lua_call(L,1,1);

   int fd=luaL_checkint(L,-1);

   lua_getfield(L,1,"setfd");
   if(lua_type(L,-1)!=LUA_TFUNCTION) {
      luaL_error(L,"No setfd function found");
   }
   lua_pushvalue(L,1);
   lua_pushinteger(L,-1);
   if(lua_pcall(L,2,0,0)) {
      const char * err=lua_tostring(L,-1);
      luaL_error(L,"Setfd error: %s",err);
   }
   lua_getfield(L,1,"close");
   lua_pushvalue(L,1);
   if(lua_pcall(L,1,0,0)) {
      const char * err=lua_tostring(L,-1);
      luaL_error(L,"Close error: %s",err);
   }
   
   lua_pushinteger(L,fd);
   return 1;
}

int leda_unwrap_sock(lua_State *L) {  
   int fd = luaL_checkint(L, 1);
   char typestr[128];
   if(lua_type(L,2)==LUA_TSTRING) {
      size_t len;const char * t = lua_tolstring(L, 2,&len);
      sprintf(typestr,"%*s",(int)(len>128?128:len),t);
   } else {
     sprintf(typestr,"tcp{client}");
   }

   lua_getglobal(L,"socket");
   if(lua_type(L,-1)!=LUA_TTABLE) {
      luaL_error(L,"Module 'socket' must be loaded to use this function");
   }
   lua_getfield(L,-1,"tcp");
   if(lua_type(L,-1)!=LUA_TFUNCTION) {
      luaL_error(L,"Function 'socket.tcp' not found");
   }
   lua_call(L,0,1);
   lua_getfield(L,-1,"setfd");
   if(lua_type(L,-1)!=LUA_TFUNCTION) {
      luaL_error(L,"Function 'setfd' not found");
   }
   lua_pushvalue(L,-2);
   lua_pushinteger(L,fd);
   lua_call(L,2,0);
   lua_remove(L,-2); //remove the socket table from the stack
   luaL_getmetatable(L, typestr);
   if(lua_type(L,-1)!=LUA_TTABLE) {
      luaL_error(L,"Socket type '%s' not found",typestr);
   }
   lua_setmetatable(L, -2);
   return 1;
}

/*************** EPOOL API ***********************/

#ifndef _WIN32 //windows does not have epoll

#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

int epoll_lcreate(lua_State *L) {
   int size=lua_tointeger(L,1);
   int epfd = epoll_create (size);
   if(epfd==-1) {
      lua_pushnil(L);
      lua_pushstring(L,strerror(errno));
      return 2;
   }
   lua_pushinteger(L,epfd);
   return 1;
}

int epoll_add_read(lua_State *L) {
   struct epoll_event event; 
   int epfd=lua_tointeger(L,1);
   int fd=lua_tointeger(L,2);
   int err;

   event.data.fd = fd; /* return the fd to us later */
   event.events = EPOLLIN;

   if((err = epoll_ctl (epfd, EPOLL_CTL_ADD, fd, &event))) {
      lua_pushnil(L);
      lua_pushstring(L,strerror(errno));
      return 2;
   }
   lua_pushboolean(L,1);
   return 1;
}

int socket_flush(lua_State *L) {
   int fd=lua_tointeger(L,1);
   int nfd=dup(fd);
   shutdown(nfd,SHUT_RDWR);
   close(nfd);
   return 0;
}

int epoll_add_read_write(lua_State *L) {
   struct epoll_event event; 
   int epfd=lua_tointeger(L,1);
   int fd=lua_tointeger(L,2);
   int err;

   event.data.fd = fd; /* return the fd to us later */
   event.events = EPOLLIN|EPOLLOUT;

   if((err = epoll_ctl (epfd, EPOLL_CTL_ADD, fd, &event))) {
      lua_pushnil(L);
      lua_pushstring(L,strerror(errno));
      return 2;
   }
   lua_pushboolean(L,1);
   return 1;
}

int epoll_add_write(lua_State *L) {
   struct epoll_event event; 
   int epfd=lua_tointeger(L,1);
   int fd=lua_tointeger(L,2);
   int err;

   event.data.fd = fd; /* return the fd to us later */
   event.events = EPOLLOUT;

   if((err = epoll_ctl (epfd, EPOLL_CTL_ADD, fd, &event))) {
      lua_pushnil(L);
      lua_pushstring(L,strerror(errno));
      return 2;
   }
   lua_pushboolean(L,1);
   return 1;
}


int epoll_remove_descriptor(lua_State *L) {
   struct epoll_event event;
   int epfd=lua_tointeger(L,1);
   int fd=lua_tointeger(L,2);

   int err;

   if((err = epoll_ctl (epfd, EPOLL_CTL_DEL, fd, &event))) {
      lua_pushnil(L);
      lua_pushstring(L,strerror(errno));
      return 2;
   }
   
   lua_pushboolean(L,1);
   return 1;
}

#define MAX_EVENTS   64

int epoll_lwait(lua_State* L) {
   int epfd=lua_tointeger(L,1);
   lua_Number to=-1;
   if(lua_type(L,2)==LUA_TNUMBER)
      to=lua_tonumber(L,2);

   int timeout=-1;
   if(to>=0.0) {
      timeout=(int)to*1000.0;
   }
      
   struct epoll_event events[MAX_EVENTS];
   int nr_events, i;

   nr_events = epoll_wait (epfd, events, MAX_EVENTS, timeout);
   if (nr_events < 0) { //epoll error
         lua_pushnil(L);
         lua_pushstring(L,strerror(errno));
         return 2;
   }

   lua_newtable(L);
   
   lua_newtable(L);
   lua_pushvalue(L,-1);
   lua_setfield(L,-3,"write"); //-3
   
   lua_newtable(L);
   lua_pushvalue(L,-1);
   lua_setfield(L,-4,"read"); //-2
   int w=1,r=1;
   for (i = 0; i < nr_events; i++) {
/*    TODO verify error
      if(events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)) {
         lua_pushnil(L);
         lua_pushstring(L,strerror(errno));
         return 2;  
      }
*/
      if (events[i].events & EPOLLOUT) { //fd available for write
         
         lua_pushinteger(L,events[i].data.fd);
         lua_rawseti(L,-3,w++);
      }
      if(events[i].events & EPOLLIN) { //fd available for read
         lua_pushinteger(L,events[i].data.fd);
         lua_rawseti(L,-2,r++);
      }
   }
   lua_pop(L,2);
   return 1;
}

int epoll_close(lua_State* L) {
   int epfd=lua_tointeger(L,1);
   if(close(epfd)){
      lua_pushnil(L);
      lua_pushstring(L,strerror(errno));
      return 2;
   }
   lua_pushboolean(L,1);
   return 1;
}

#endif
