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
   EMMIT_REMOTE,
   WAIT_IO,
   FILE_IO,
   SLEEP,
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
           lua_newtable(L); \
           luaL_setfuncs (L,funcs,0); 
#else
   #define REGISTER_LEDA(L,libname,funcs) luaL_register(L,libname,funcs)
#endif

/* Defining debug functions */ 
#ifndef DEBUG
   #define _DEBUG(...)
   #define dump_stack(...)
#else
   extern MUTEX_T debug_lock;
   #define _DEBUG(...) fprintf(stdout,"%s: %d (%s):",__FILE__,__LINE__,__func__); fprintf(stdout,__VA_ARGS__); 
   void dump_stack( lua_State* L );
#endif

void thread_init(size_t ready_queue_capacity);

thread thread_get (lua_State *L, int i);
int thread_new (lua_State *L);
int thread_createmetatable (lua_State *L);
int thread_kill (lua_State *L);
int thread_rawkill (lua_State *L);

int wait_io(lua_State * L);
int do_file_aio(lua_State * L);
int emmit(lua_State * L);
int emmit_sync(lua_State * L);
int emmit_packed_event(stage_id dst_id,char * data,size_t len);
int cohort(lua_State * L);

int leda_getmetatable(lua_State *L);
int leda_setmetatable(lua_State *L);
int leda_quit(lua_State *L);

#endif //_THREAD_H_
