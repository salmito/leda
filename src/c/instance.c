#include "instance.h"
#include "marshal.h"

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

void leda_initinstance(instance_t i) {
	lua_State *L=i->L;
	lua_pushcfunction(L,luaopen_base);
   lua_pcall(L,0,0,0);
   lua_pushcfunction(L,luaopen_package);
   lua_pcall(L,0,1,0);
   #if LUA_VERSION_NUM > 501
   lua_pushcfunction(L,luaopen_coroutine);
   lua_pcall(L,0,1,0);
	#endif
	lua_pushliteral(L,"stage-env");
	lua_pushcfunction(L,mar_decode);
	lua_pushlstring(L,i->stage->env,i->stage->env_len);
	lua_call(L,1,1);
	lua_settable(L, LUA_REGISTRYINDEX);
	i->flags=INITIATED;
}

instance_t leda_newinstance(stage_t s) {
   lua_State * L = luaL_newstate();
	instance_t i=malloc(sizeof(struct instance_s));
	i->L=L;
	i->stage=s;
	i->flags=CREATED;
	i->ev=NULL;
	return i;
}

void leda_destroyinstance(instance_t i) {
   lua_close(i->L);
   if(i->ev) leda_destroyevent(i->ev);
   free(i);

}
