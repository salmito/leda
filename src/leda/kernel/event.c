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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "event.h"
#include "thread.h"
#include "extra/lmarshal.h"

#define MAXN 1024

struct _element {
   int type; //Type of element
   union  {
      lua_Number n;
      char * str;
    	bool_t b;
   	void * ptr;
//   	element table; TODO
   } data;
   size_t len; //Size of data (if type is 'string' it holds the size of string)
};

/* Free the payload vector of an event.
 * 
 * If an element is a string, it will be freed as well
 */
void destroy_event(event e) {
   size_t i;
   for(i=0;i<e->n;i++) {
      switch(e->payload[i].type) {
         case LUA_TSTRING:
         case LUA_TFUNCTION:
         case LUA_TTABLE:
         case LUA_TUSERDATA:
         if(e->payload[i].data.str) 
            free(e->payload[i].data.str);
         }
    }
   free(e->payload);
   free(e);
}

/* Dump an event for debug purposes */
void dump_event(lua_State *L, event e) {
   size_t i;
   for(i=0;i<e->n;i++) {
      _DEBUG("Event: DUMP element->type='%s' len='%u' ",
      lua_typename(L,e->payload[i].type),
      (uint_t)e->payload[i].len);
      
      switch (e->payload[i].type) {
      case LUA_TBOOLEAN:
         _DEBUG("value='%s' ",(e->payload[i].data.b?"TRUE":"FALSE"));
         break;
      case LUA_TNUMBER:
         _DEBUG("value='%f' ",e->payload[i].data.n);
         break;
      case LUA_TSTRING:
         _DEBUG("value='%s' length='%d'",e->payload[i].data.str,(int)e->payload[i].len);
         break;
      case LUA_TLIGHTUSERDATA:
         _DEBUG("value='%p' ",e->payload[i].data.ptr);
         break;
      }
     _DEBUG("\n");
   }
}

/* restore an event to a Lua stack.
 * The event payload is pushed back to the top of the stack
 */
int restore_event_to_lua_state(lua_State * L, event *e_t) {
   size_t i;
   event e=*e_t;
   
   for(i=0;i<e->n;i++) {
      switch ( e->payload[i].type ) {
         case LUA_TNIL:
            lua_pushnil(L);
            break;
         case LUA_TBOOLEAN:
            lua_pushboolean(L, e->payload[i].data.b);
            break;
         case LUA_TNUMBER:
            lua_pushnumber(L, e->payload[i].data.n);
            break;
         case LUA_TSTRING:
            lua_pushlstring(L, e->payload[i].data.str,e->payload[i].len);
            break;
         case LUA_TLIGHTUSERDATA:
            lua_pushlightuserdata(L, e->payload[i].data.ptr);
            break;
         case LUA_TFUNCTION:
         case LUA_TTABLE:
         case LUA_TUSERDATA:
            lua_pushcfunction(L,mar_decode);
            lua_pushlstring(L, e->payload[i].data.str,e->payload[i].len);
            lua_call(L,1,1);
            break;
      }
   }
   int ret=e->n;
   destroy_event(e);
   *e_t=NULL;
   return ret;
}


/*
 * Copies values directly from lua_State src to lua_State dst. 
 * (code taken from luarings, with small adjustments)
 */
void copy_values_directly 
     (lua_State *dst, lua_State *src, int from, int args) {
  int i;
  if(args==0) return; //nothing to copy
 lua_checkstack(dst, from+args);
  
  for (i=from; i < from+args; i++) {
    switch (lua_type (src, i)) {
      case LUA_TNUMBER:
        lua_pushnumber (dst, lua_tonumber (src, i));
        break;
      case LUA_TBOOLEAN:
        lua_pushboolean (dst, lua_toboolean (src, i));
        break;
      case LUA_TSTRING: {
        const char *string = lua_tostring (src, i);
        size_t length = lua_objlen (src, i);
        lua_pushlstring (dst, string, length);
        break;
      }
      case LUA_TLIGHTUSERDATA: {
        lua_pushlightuserdata (dst, lua_touserdata (src, i));
        break;
      }
      case LUA_TNIL:
        lua_pushnil (dst);
        break;
      case LUA_TFUNCTION:
      case LUA_TTABLE:
      case LUA_TUSERDATA: {
          lua_pushcfunction(src,mar_encode);
          lua_pushvalue(src,i);
          lua_call(src,1,1);
          size_t len; const char *s = lua_tolstring( src, -1, &len );
          lua_pushcfunction(dst,mar_decode);
          lua_pushlstring (dst, s, len);
          lua_call(dst,1,1);
          lua_pop(src,1);
         } break;              
      default:
         lua_pushfstring(src,"Value type '%s' not supported",
         lua_typename(src,lua_type(src,i)));
         lua_error(src);
        break;
    }
  }
}

/*
** Copies an event element from the Lua stack
*/
bool_t copy_event_element(lua_State *L, size_t i, element e) {
   switch ( lua_type(L,i) ) {
      case LUA_TNIL:
         e->type=LUA_TNIL;
         e->len=0;
         break;
      case LUA_TBOOLEAN:
         e->type=LUA_TBOOLEAN;
         e->len=sizeof(bool_t);
         e->data.b=lua_toboolean(L, i);
         break;
      case LUA_TNUMBER:
         e->type=LUA_TNUMBER;
         e->data.n=lua_tonumber(L, i);
         e->len=sizeof(lua_Number);
         break;
       case LUA_TSTRING: {
         size_t len; const char *s = lua_tolstring( L, i, &len );
         e->type=LUA_TSTRING;
         /* We have to allocate a new memory chunk for the string
         because the garbage collector may destroy the string 's'
         before the event consumption.
         It will be deallocated in the event_destroy() function */
         e->data.str=malloc(len+1); //space for \0
         e->len=len;
         memcpy(e->data.str,s,len);
         e->data.str[len]='\0';
         } break;
       case LUA_TLIGHTUSERDATA:
         e->type=LUA_TLIGHTUSERDATA;
         e->data.ptr=lua_touserdata(L, i);
         e->len=sizeof(void *);
         break;
       case LUA_TFUNCTION:
       case LUA_TTABLE:
       case LUA_TUSERDATA: {
          e->type=lua_type(L,i);
          lua_pushcfunction(L,mar_encode);
          lua_pushvalue(L,i);
          lua_call(L,1,1);
          size_t len; const char *s = lua_tolstring( L, -1, &len );
          e->data.str=malloc(len);
          e->len=len;
          memcpy(e->data.str,s,len);
          lua_pop(L,1);
          } break;
       default:
         return FALSE;
   }
   return TRUE;
}

#define ABS(x) (x<0?-x:x)

/* extract a event from a lua stack of length 'args' from the stack 
 * index 'from' (inclusive).
 */
event extract_event_from_lua_state(lua_State *L, int from, int args) {
   int i,j=0;
   bool_t ok=TRUE;

	event e=malloc(sizeof(struct event_data));
   e->n=args; //number of elements of the payload to be copyed
	
	e->payload=calloc(e->n,sizeof(struct _element));
	
   for(i = from; i < from+args; ++ i) {
      if(!(ok=copy_event_element(L, i, &e->payload[j++])))
         break;
	}
	
	if(!ok) {
      //error occured at i-th position
   	destroy_event(e);
      lua_pushfstring(L,"Value type '%s' not supported",
      lua_typename(L,lua_type(L,i)));
      lua_error(L);
  	}
  	return e; //copyed ok
}

