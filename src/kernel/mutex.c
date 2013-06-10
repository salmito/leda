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

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "mutex.h"
#include "scheduler.h"

/*Get a thread descriptor from the lua stack*/
MUTEX_T * mutex_get (lua_State *L, int i) {
  if(!lua_islightuserdata(L,i)) {
      lua_pushstring(L,"expected a lightuserdata as argument");
      lua_error(L);
  }
  MUTEX_T * m = lua_touserdata (L, i);
  luaL_argcheck (L, m != NULL, i, "mutex is null");
  return m;
}

/* Lock a mutex from Lua*/
int mutex_lock (lua_State *L) {
   MUTEX_T * m=mutex_get(L,1);
   _DEBUG("Mutex: Locking mutex %p\n",m);
   MUTEX_LOCK(&*m);
   return 0;
}

/* Unlock a mutex from Lua*/
int mutex_unlock (lua_State *L) {
   MUTEX_T * m=mutex_get(L,1);
   _DEBUG("Mutex: Unlocking mutex %p\n",m);
   MUTEX_UNLOCK(m);
   return 0;
}

/* Destroy a mutex from Lua*/
int mutex_destroy (lua_State *L) {
   MUTEX_T * m=mutex_get(L,1);
   MUTEX_FREE(m);
   free(m);
   return 0;
}

/* create a new mutex and returns its descriptor */
int mutex_new (lua_State *L) {
   //Allocate space for the mutex descriptor
   MUTEX_T * m=malloc(sizeof(MUTEX_T));
   MUTEX_INIT(m);
   _DEBUG("Mutex: Created new mutex %p\n",m);
   //Put a reference to the pointer of the mutex descriptor
   //on the stack as a light userdata
   lua_pushlightuserdata(L, m);

   //Return the reference pointer of the mutex descriptor
   return 1;
}
