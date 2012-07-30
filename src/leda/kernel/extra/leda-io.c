#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "leda-io.h"

#define tofile(L,i)	((FILE **)luaL_checkudata(L, i, LUA_FILEHANDLE))

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

int leda_wrap_io(lua_State *L) {  
   FILE ** f=tofile(L,1);
   int fd=fileno(*f);
   int newfd=dup(fd);
   lua_pushinteger(L,newfd);
   return 1;
}

int leda_unwrap_sock(lua_State *L) {  
   int fd = luaL_checkint(L, 1);
   char typestr[128];
   if(lua_type(L,2)==LUA_TSTRING) {
      size_t len;const char * t = lua_tolstring(L, 2,&len);
      sprintf(typestr,"%*s",(int)len,t);
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
