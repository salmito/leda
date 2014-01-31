#include "leda.h"
#include "scheduler.h"
#include "instance.h"
#include "threading.h"
#include "event.h"
#include "marshal.h"
#include "lf_queue.h"

#include <time.h>

static LFqueue_t ready_queue=NULL;

static int thread_tostring (lua_State *L) {
  THREAD_T ** t = luaL_checkudata (L, 1, LEDA_THREAD_METATABLE);
  lua_pushfstring (L, "Thread (%p)", *t);
  return 1;
}

static THREAD_T * thread_get(lua_State *L, int i) {
	THREAD_T ** t = luaL_checkudata (L, i, LEDA_THREAD_METATABLE);
	luaL_argcheck (L, *t != NULL, i, "not a Thread");
	return *t;
}

static int thread_join (lua_State *L) {
	THREAD_T * t=thread_get(L,1);
	int timeout=lua_tointeger(L,2);
	if(timeout>0) {
		struct timespec to;
		clock_gettime(CLOCK_REALTIME, &to);
		to.tv_sec += timeout;
	   pthread_timedjoin_np(*t,NULL,&to);
   } else {
	   pthread_join(*t,NULL);
   }
   return 0;
}

static int thread_rawkill (lua_State *L) {
   THREAD_T * t=thread_get(L,1);
   THREAD_KILL(t);
   return 0;
}

static int thread_ptr (lua_State *L) {
	THREAD_T * t=thread_get(L,1);
	lua_pushlightuserdata(L,t);
	return 1;
}

static void get_metatable(lua_State * L) {
	luaL_getmetatable(L,LEDA_THREAD_METATABLE);
   if(lua_isnil(L,-1)) {
   	lua_pop(L,1);
  		luaL_newmetatable(L,LEDA_THREAD_METATABLE);
  		lua_pushvalue(L,-1);
  		lua_setfield(L,-2,"__index");
		lua_pushcfunction (L, thread_tostring);
		lua_setfield (L, -2,"__tostring");
		lua_pushcfunction(L,thread_join);
		lua_setfield (L, -2,"join");
		lua_pushcfunction(L,thread_ptr);
		lua_setfield (L, -2,"ptr");
		lua_pushcfunction(L,thread_rawkill);
		lua_setfield (L, -2,"rawkill");
		luaL_loadstring(L,"local th=(...):ptr() return function() return require'leda.scheduler'.build(th) end");
		lua_setfield (L, -2,"__wrap");
  	}
}

static void thread_resume_instance(instance_t i) {
	lua_State * L=i->L;
	switch(i->flags) {
		case CREATED:
			leda_initinstance(i);
			break;
		case WAITING_IO:
			i->flags=READY;
			lua_pushliteral(L,STAGE_HANDLER_KEY);
			lua_gettable(L,LUA_REGISTRYINDEX);
			if(lua_pcall(i->L,0,0,0)) {
		      const char * err=lua_tostring(L,-1);
		      fprintf(stderr,"Error resuming instance: %s\n",err);
		   }
		   break;
		case IDLE:
			break;
		case READY:
			if(i->ev) {
				lua_pushliteral(L,STAGE_HANDLER_KEY);
				lua_gettable(L,LUA_REGISTRYINDEX);
		      lua_pushcfunction(L,mar_decode);
		      lua_pushlstring(L,i->ev->data,i->ev->len);
		      leda_destroyevent(i->ev);
  		      i->ev=NULL;
				if(lua_pcall(L,1,1,0)) {
					const char * err=lua_tostring(L,-1);
			      fprintf(stderr,"Error decoding event: %s\n",err);
			      break;
				}
				int n=
				#if LUA_VERSION_NUM < 502
					luaL_getn(L,2);
			   #else
					luaL_len(L,2);
				#endif
				int j;
				for(j=1;j<=n;j++) lua_rawgeti(L,2,j);
				lua_remove(L,2);
				i->args=n;
			}
			if(lua_pcall(i->L,i->args,0,0)) {
		      const char * err=lua_tostring(L,-1);
		      fprintf(stderr,"Error resuming instance: %s\n",err);
		   } 
			break;
	}
	if(i->flags!=WAITING_IO) leda_putinstance(i);
}

/*thread main loop*/
static THREAD_RETURN_T THREAD_CALLCONV thread_mainloop(void *t_val) {
   instance_t i=NULL;
   while(1) {
      leda_lfqueue_pop(ready_queue,(void **)&i);
      if(i==NULL) break;
      thread_resume_instance(i);
   }
   free(t_val);
   return t_val;
}

static int thread_new (lua_State *L) {
	THREAD_T ** thread=lua_newuserdata(L,sizeof(THREAD_T*));
   get_metatable(L);
   lua_setmetatable(L,-2);   
   THREAD_CREATE(*thread, thread_mainloop, *thread, 0 );
   return 1;
}

static int thread_from_ptr (lua_State *L) {
	THREAD_T * ptr=lua_touserdata(L,1);
	THREAD_T ** thread=lua_newuserdata(L,sizeof(THREAD_T*));
	*thread=ptr;
   get_metatable(L);
   lua_setmetatable(L,-2);   
   THREAD_CREATE(*thread, thread_mainloop, *thread, 0 );
   return 1;
}

static int thread_kill (lua_State *L) {
	void *a=NULL;
	leda_lfqueue_push(ready_queue,&a);
	return 0;
}

inline void leda_pushinstance(instance_t i) {
	return leda_lfqueue_push(ready_queue,(void **)&i);
}

LEDA_EXPORTAPI	int luaopen_leda_scheduler(lua_State *L) {
	const struct luaL_Reg LuaExportFunctions[] = {
	{"new_thread",thread_new},
	{"kill_thread",thread_kill},
	{"build",thread_from_ptr},
	{NULL,NULL}
	};
	if(!ready_queue) ready_queue=leda_lfqueue_new();
	lua_newtable(L);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'leda.scheduler' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif        
	return 1;
};
