#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "extra/threading.h"
#include "event.h"

#define MAXN 1024

typedef struct _element {
   int type; //Type of element
   void * data; //Pointer to data
   size_t len; //Size of data
} * element;

//Creates a container for event with 'n' slots 'len' (size of 
event new_event(size_t n, size_t len) {
   event e;
   if(n<0 && n>MAXN) return NULL;
   e=malloc(sizeof(struct event_data));
   e->n=n;
   e->elements=malloc(len);
   return e;
} 

void destroy_element(element e) {
   if(e) {
      if(e->data && e->type!=LUA_TLIGHTUSERDATA) free(e->data);
      free(e);
   }
}

void destroy_event(data d) {
   size_t i;
   if(d) {
      for(i=0;i<d->n;i++) {
         destroy_element(d->elements[i]);
      }
      free(d->elements);
      free(d);
   }
}

void dump_event(lua_State *L, event d) {
   size_t i;
   for(i=0;i<d->n;i++) {
      _DEBUG("Event: DUMP element->type='%s' len='%u' ",lua_typename(L,d->elements[i]->type),(uint_t)d->elements[i]->len);
         switch ( d->elements[i]->type ) {
            case LUA_TBOOLEAN:
               _DEBUG("value='%s' ",(*(bool_t *)d->elements[i]->data?"TRUE":"FALSE"));
               break;
            case LUA_TNUMBER:
               _DEBUG("value='%f' ",*(lua_Number *)d->elements[i]->data);
               break;
            case LUA_TSTRING:
               _DEBUG("value='%s' ",(char *)d->elements[i]->data);
               break;
            case LUA_TLIGHTUSERDATA:
               _DEBUG("value='%p' ",d->elements[i]->data);
               break;

         }
        printf("\n");
      }
   }
}

int restore_event_to_lua_state(lua_State * L, event d) {
   size_t i;
   if(d) {
      for(i=0;i<d->n;i++) {
         switch ( d->elements[i]->type ) {
            case LUA_TNIL:
               lua_pushnil(L);
               break;
            case LUA_TBOOLEAN:
               lua_pushboolean(L,*(bool_t *)d->elements[i]->data);
               break;
            case LUA_TNUMBER:
               lua_pushnumber( L,*(lua_Number *)d->elements[i]->data);
               break;
            case LUA_TSTRING:
               lua_pushlstring( L,(char *)d->elements[i]->data,d->elements[i]->len);
               break;
            case LUA_TLIGHTUSERDATA:
               lua_pushlightuserdata( L, d->elements[i]->data );
               break;
         }
      }
      return d->n;
   }
   return 0;
}


element copy_event_element(lua_State *L, size_t i) {
   bool_t ret=TRUE;
   element e=NULL;

   switch ( lua_type(L,i) ) {
      case LUA_TNIL:
         e=(element)malloc(sizeof(struct _element));
         e->type=LUA_TNIL;
         e->data=NULL;
         e->len=0;
         break;
      case LUA_TBOOLEAN:
         e=(element)malloc(sizeof(struct _element));
         e->type=LUA_TBOOLEAN;
         e->data=(bool_t *)malloc(sizeof(bool_t));
         e->len=sizeof(bool_t);
         *((bool_t *)e->data)=lua_toboolean(L, i);
         break;
      case LUA_TNUMBER:
         e=(element)malloc(sizeof(struct _element));
         e->type=LUA_TNUMBER;
         e->data=(lua_Number *)malloc(sizeof(lua_Number));
         e->len=sizeof(lua_Number);
         *((lua_Number *)e->data)=lua_tonumber(L, i);
         break;
       case LUA_TSTRING: {
         size_t len; const char *s = lua_tolstring( L, i, &len );
         e=(element)malloc(sizeof(struct _element));
         e->type=LUA_TSTRING;
         e->data=(char *)malloc(len+1); //space for \0 just for sure
         e->len=len;
         memcpy(e->data,s,len);
         ((char *)e->data)[len]='\0';
         } break;
       case LUA_TLIGHTUSERDATA:
         e=(element)malloc(sizeof(struct _element));
         e->type=LUA_TLIGHTUSERDATA;
         e->data=lua_touserdata(L, i);
         e->len=sizeof(void *);
       break;
       
   }
   return e;
}
event extract_event_from_lua_state(lua_State *L, int from, int to);
//returns a negative integer on error and 0 if ok
data copy_data_from_state(lua_State *L,size_t n) {
   uint_t top = lua_gettop( L),i,j=0;
   bool_t ok=TRUE;
   event d;
   element e;

   if(n > top)	{
		//not enough parameters on stack
		return NULL;
	}
	
	d=new_data(n);

	for(i = top - n + 1; i <= top; ++ i) {
	   d->elements[j++]=e=copy_element(L,i);
	   if(e==NULL) { ok=FALSE; break; }
	}
	
	if(ok) {
	   //data was copied
	   return d;
   } else {
      //error occured at 'i' position
   	destroy_data(d);
      char sErr[128];
      sprintf(sErr,"Invalid parameter type: '%s'",lua_typename(L,lua_type(L,i)));
   	luaL_argerror (L, i, sErr);
   	return NULL;
  	}
}

