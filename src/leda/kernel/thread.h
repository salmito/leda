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
#ifndef _THREAD_H_
#define _THREAD_H_

#include "extra/threading.h"
#include "instance.h"
#include "queue.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


#define THREAD_METATABLE   "leda thread"

/* leda's thread descriptor structure */ 
typedef struct thread_data {
	THREAD_T thread;
	volatile int status;
} * thread;

typedef thread * thread_ptr;

/* lua 5.1 to 5.2 compatibility macros */
#if LUA_VERSION_NUM > 501
   #define lua_objlen lua_rawlen
   #define luaL_reg luaL_Reg
   #define luaL_register(L,libname,funcs) \
           lua_getglobal(L,"leda");  \
           lua_pushliteral(L,"kernel");  \
           lua_newtable(L); \
           luaL_setfuncs (L,funcs,0); \
           lua_rawset(L,-3); \
           lua_newtable(L); \
            
#endif

/* Defining debug functions */ 
#ifndef DEBUG
   #define _DEBUG(...)
   #define dump_stack(...)
#else
   static MUTEX_T debug_lock;
   #define _DEBUG(...) MUTEX_LOCK(&debug_lock); fprintf(stderr,__VA_ARGS__); MUTEX_UNLOCK(&debug_lock);
   void dump_stack( lua_State* L );
#endif

void copy_values (lua_State *dst, lua_State *src, int i, int top);
size_t thread_ready_queue_size();
bool_t thread_try_push_instance(instance i);
void thread_init(size_t ready_queue_capacity);

thread thread_get (lua_State *L, int i);
int thread_new (lua_State *L);
int thread_createmetatable (lua_State *L);
int thread_kill (lua_State *L);
size_t thread_ready_queue_size();

int call(lua_State * L);
int emmit(lua_State * L);
int emmit_self_call(lua_State * L);

#endif //_THREAD_H_
