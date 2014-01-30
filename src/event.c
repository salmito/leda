#include "leda.h"
#include "event.h"
#include "threading.h"
#include "marshal.h"

#include <string.h>

static THREAD_T event_thread;

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

static THREAD_RETURN_T THREAD_CALLCONV event_main(void *t_val) {
	printf("started\n");
	return NULL;
}

LEDA_EXPORTAPI	int luaopen_leda_event(lua_State *L) {
	const struct luaL_Reg LuaExportFunctions[] = {
	{"encode",mar_encode},
	{"decode",mar_decode},
	{NULL,NULL}
	};
	if(!event_thread) THREAD_CREATE(&event_thread, event_main, NULL, 0);
	
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
