#include "stage.h"
#include "lf_queue.h"
#include "lf_hash.h"

#include <stdlib.h>
#include <string.h>

#define DEFAULT_IDLE_CAPACITY 4096
#define DEFAULT_QUEUE_CAPACITY -1

struct leda_Stage {
	LFqueue_t instances;
	LFqueue_t event_queue;
	char * env;
	size_t env_len;
	volatile unsigned int flags;
};

stage_t leda_tostage(lua_State *L, int i) {
	stage_t * s = luaL_checkudata (L, i, LEDA_STAGE_META);
	luaL_argcheck (L, s != NULL, i, "Stage expected");
	return *s;
}

static int get_queue_capacity(lua_State * L) {
	stage_t s=leda_tostage(L,1);
	lua_pushnumber(L,leda_lfqueue_getcapacity(s->event_queue));
	return 1;
}

static int set_queue_capacity(lua_State * L) {
	stage_t s=leda_tostage(L,1);
	luaL_checktype (L, 2, LUA_TNUMBER);
	int capacity=lua_tointeger(L,2);
	leda_lfqueue_setcapacity(s->event_queue,capacity);
	return 0;
}

static int get_max_instances(lua_State * L) {
	stage_t s=leda_tostage(L,1);
	lua_pushnumber(L,leda_lfqueue_getcapacity(s->instances));
	return 1;
}

static int set_max_instances(lua_State * L) {
	stage_t s=leda_tostage(L,1);
	luaL_checktype (L, 2, LUA_TNUMBER);
	int capacity=lua_tointeger(L,2);
	leda_lfqueue_setcapacity(s->instances,capacity);
	return 0;
}

/*tostring method*/
static int stage_tostring (lua_State *L) {
  stage_t * s = luaL_checkudata (L, 1, LEDA_STAGE_META);
  lua_pushfstring (L, "Stage (%p)", *s);
  return 1;
}

static int stage_getid (lua_State *L) {
	stage_t s = leda_tostage(L, 1);
	lua_pushlstring(L,(const char *)&s,sizeof(void*));
	return 1;
}

static void get_metatable(lua_State * L) {
	luaL_getmetatable(L,LEDA_STAGE_META);
   if(lua_isnil(L,-1)) {
   	lua_pop(L,1);
  		luaL_newmetatable(L,LEDA_STAGE_META);
  		lua_pushvalue(L,-1);
  		lua_setfield(L,-2,"__index");
		lua_pushcfunction (L, stage_tostring);
		lua_setfield (L, -2,"__tostring");
//		lua_pushcfunction (L, leda_destroystage);
//		lua_setfield (L, -2,"__gc");
  		lua_pushcfunction(L,get_max_instances);
  		lua_setfield(L,-2,"max_instances");
		lua_pushcfunction(L,set_max_instances);
  		lua_setfield(L,-2,"set_max_instances");
  		lua_pushcfunction(L,get_queue_capacity);
  		lua_setfield(L,-2,"capacity");
  		lua_pushcfunction(L,set_queue_capacity);
  		lua_setfield(L,-2,"set_capacity");
  		lua_pushcfunction(L,stage_getid);
  		lua_setfield(L,-2,"id");
  	}
}

static void leda_buildstage(lua_State * L,stage_t t) {
	stage_t *s=lua_newuserdata(L,sizeof(stage_t *));
	*s=t;
	get_metatable(L);
   lua_setmetatable(L,-2);
}

static int leda_newstage(lua_State * L) {
   luaL_checktype (L, 1, LUA_TSTRING);
   size_t len=0; const char *env=lua_tolstring(L,1,&len);
   stage_t * stage=lua_newuserdata(L,sizeof(stage_t *));
   (*stage)=malloc(sizeof(struct leda_Stage));   
   (*stage)->instances=leda_lfqueue_new();
   leda_lfqueue_setcapacity((*stage)->instances,DEFAULT_IDLE_CAPACITY);
   (*stage)->event_queue=leda_lfqueue_new();
   leda_lfqueue_setcapacity((*stage)->event_queue,DEFAULT_QUEUE_CAPACITY);
   char *envcp=malloc(len+1);
   envcp[len]='\0';
   memcpy(envcp,env,len+1);
   (*stage)->env=envcp;
   (*stage)->env_len=len;  
  	get_metatable(L);
   lua_setmetatable(L,-2);
   return 1;
}

static int leda_destroystage(lua_State * L) {
	stage_t * s_ptr = luaL_checkudata (L, 1, LEDA_STAGE_META);
	if(!s_ptr) return 0;
	if(!(*s_ptr)) return 0;
	stage_t s=*s_ptr;
	free(s->env);
	leda_lfqueue_free(s->instances);
	leda_lfqueue_free(s->event_queue);
	*s_ptr=0;
	return 0;
}


static qt_hash H=NULL;

static int leda_getstage(lua_State * L) {
	size_t len=0; 
	const void ** key=(const void **)lua_tolstring(L,1,&len);
	if(len!=sizeof(void *)) {
		lua_pushnil(L);
		lua_pushliteral(L,"Key length error");
		return 2;
	}
	stage_t s=qt_hash_get(H,*key);
	if(s) {
		leda_buildstage(L,s);
		return 1;
	}
	lua_pushnil(L);
	lua_pushliteral(L,"Stage not found");
	return 2;
}

static int leda_addstage(lua_State * L) {
   stage_t s=leda_tostage(L,1);
	qt_hash_put(H,s,s);
	return 0;
}

static const struct luaL_Reg LuaExportFunctions[] = {
	{"new",leda_newstage},
	{"get",leda_getstage},
	{"add",leda_addstage},
	{"destroy",leda_destroystage},
	{NULL,NULL}
};

LEDA_EXPORTAPI	int luaopen_leda_stage(lua_State *L) {
	if(!H) H=qt_hash_create();
	lua_newtable(L);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs (L, LuaExportFunctions, 0);
#endif        
	return 1;
};
