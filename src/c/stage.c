#include "leda.h"
#include "stage.h"
//#include "lf_queue.h"

struct leda_Stage {
//	LFqueue_t L;
//	LFqueue_t queue;
	const char * env;
	const size_t env_len;
	unsigned int flags;
};

static void stage_newmetatable(lua_State *L) {
	luaL_newmetatable(L,LEDA_STAGE_META);
}

int leda_newstage(lua_State * L) {
   size_t len=0; const char *env=lua_tolstring(L,1,&len);
   stage_t stage=lua_newuserdata(L,sizeof(struct leda_Stage));
   luaL_getmetatable(L,LEDA_STAGE_META);
   if(lua_isnil(L,-1)) {
   	lua_pop(L,1);
   	stage_newmetatable(L);
   }
   lua_setmetatable(L,-2);
   stackDump(L,"teste");
   return 1;
}
