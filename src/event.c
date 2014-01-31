#include "leda.h"
#include "event.h"
#include "instance.h"
#include "threading.h"
#include "marshal.h"

#include <string.h>
#include <event2/event.h>

static THREAD_T * event_thread;
static struct event_base *loop;

event_t leda_newevent(const char * ev, size_t len) {
   event_t e=malloc(sizeof(struct event_s));
   e->data=malloc(len);
   memcpy(e->data,ev,len);
   e->len=len;
   return e;
}

void leda_destroyevent(event_t e) {
   free(e->data);
   free(e);
}

static void dummy_event(evutil_socket_t fd, short events, void *arg) {}

static void io_ready(evutil_socket_t fd, short event, void *arg) {
	lua_State * L=(lua_State *)arg;
	//i->flags=WAITING_IO;
	stackDump(L,"asdasd");
	printf("SHITTTT\n");
}

static int event_wait_io(lua_State * L) {
   int fd=-1;

   fd=lua_tointeger(L,1);

   int mode=-1;
   mode=lua_tointeger(L,2);
   int m=0;
   if(mode==0) 
      m = EV_READ; // read
   else if(mode==1)
         m = EV_WRITE; //write
   else luaL_error(L,"Invalid io operation type (0=read and 1=write)");

   lua_pushliteral(L,"create_coroutine");
   stackDump(L,"yield1");
   lua_gettable(L,LUA_REGISTRYINDEX);
   stackDump(L,"yield2");
   lua_call(L,0,1);
   stackDump(L,"merd");
   event_base_once(loop, fd, m, io_ready, L, NULL);   
   return 0;
}

static int event_yield(lua_State * L){
   stackDump(L,"yield");
   lua_pushvalue(L,1);
   return lua_yield(L,1);
}

static THREAD_RETURN_T THREAD_CALLCONV event_main(void *t_val) {
	loop = event_base_new();
	if(!loop) {
	printf("PUTZ\n");
	return NULL;
	}
   struct event *listener_event = event_new(loop, -1, EV_READ|EV_PERSIST, dummy_event, NULL);
   event_add(listener_event, NULL);
   event_base_dispatch(loop);
   printf("AYE\n");
	return NULL;
}

LEDA_EXPORTAPI	int luaopen_leda_event(lua_State *L) {
	const struct luaL_Reg LuaExportFunctions[] = {
	{"encode",mar_encode},
	{"decode",mar_decode},
	{"waitfd",event_wait_io},
	{"yield",event_yield},
	{NULL,NULL}
	};
	if(!event_thread) {
		event_thread=malloc(sizeof(THREAD_T));
		THREAD_CREATE(event_thread, &event_main, NULL, 0);
	}
	
	lua_newtable(L);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'leda.event' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif        
	return 1;
};
