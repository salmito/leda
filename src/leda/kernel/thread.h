#ifndef _THREAD_H_
#define _THREAD_H_

#include "extra/threading.h"
#include "instance.h"
#include "queue.h"
#include "graph.h"
#include "atomic.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>


#define THREAD_METATABLE   "leda thread"

/* leda's thread descriptor structure */ 
typedef struct thread_data {
	THREAD_T thread;
	volatile int status;
	volatile int destroy;
} * thread;

/* Thread main coroutine exit status*/
enum return_status{ 
   ENDED=0xF1F21AB,
   EMMIT_COHORT,
   PCALL_ERROR,
   YIELDED
};

extern atomic pool_size;

char const * get_return_status_name(int status);
/* lua 5.1 to 5.2 compatibility macros */
#if LUA_VERSION_NUM > 501
   #define lua_objlen lua_rawlen
   #define luaL_reg luaL_Reg
   #define REGISTER_LEDA(L,libname,funcs) \
           lua_getglobal(L,"leda");  \
           lua_pushliteral(L,"kernel");  \
           lua_newtable(L); \
           luaL_setfuncs (L,funcs,0); \
           lua_rawset(L,-3); \
           lua_newtable(L); 
#else
   #define REGISTER_LEDA(L,libname,funcs) luaL_register(L,libname,funcs)
#endif

/* Defining debug functions */ 
#ifndef DEBUG
   #define _DEBUG(...)
   #define dump_stack(...)
#else
   extern MUTEX_T debug_lock;
   #define _DEBUG(...) /*MUTEX_LOCK(&debug_lock);*/ fprintf(stdout,__VA_ARGS__); /*MUTEX_UNLOCK(&debug_lock);*/
   void dump_stack( lua_State* L );
#endif

void thread_init(size_t ready_queue_capacity);

thread thread_get (lua_State *L, int i);
int thread_new (lua_State *L);
int thread_createmetatable (lua_State *L);
int thread_kill (lua_State *L);

int emmit(lua_State * L);
int cohort(lua_State * L);

#endif //_THREAD_H_
