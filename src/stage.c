#include "stage.h"
#include "lf_hash.h"
#include "marshal.h"
#include "event.h"
#include "scheduler.h"
#include "instance.h"

#include <stdlib.h>
#include <string.h>

static qt_hash H;

#define DEFAULT_IDLE_CAPACITY 10
#define DEFAULT_QUEUE_CAPACITY -1

static void get_metatable(lua_State * L);

stage_t lstage_tostage(lua_State *L, int i) {
	stage_t * s = luaL_checkudata (L, i, LSTAGE_STAGE_METATABLE);
	luaL_argcheck (L, s != NULL, i, "Stage expected");
	return *s;
}

static int get_queue_capacity(lua_State * L) {
	stage_t s=lstage_tostage(L,1);
	lua_pushnumber(L,lstage_lfqueue_getcapacity(s->event_queue));
	return 1;
}

static int set_queue_capacity(lua_State * L) {
	stage_t s=lstage_tostage(L,1);
	luaL_checktype (L, 2, LUA_TNUMBER);
	int capacity=lua_tointeger(L,2);
	lstage_lfqueue_setcapacity(s->event_queue,capacity);
	return 0;
}

static int get_max_instances(lua_State * L) {
	stage_t s=lstage_tostage(L,1);
	lua_pushnumber(L,lstage_lfqueue_getcapacity(s->instances));
	return 1;
}

static int stage_getenv(lua_State * L) {
	stage_t s=lstage_tostage(L,1);
	lua_pushlstring(L,s->env,s->env_len);
	return 1;
}

static int stage_push(lua_State *L) {
   stage_t s=lstage_tostage(L,1);
   int top=lua_gettop(L);
   lua_pushcfunction(L,mar_encode);
   lua_newtable(L);
   int i;
   for(i=2;i<=top;i++) {
      lua_pushvalue(L,i);
      lua_rawseti(L,-2,i-1);
   }
   lua_call(L,1,1);
   size_t len;
   const char * str=lua_tolstring(L,-1,&len);
   lua_pop(L,1);
   event_t ev=lstage_newevent(str,len);
   instance_t ins=NULL;
   if(lstage_lfqueue_trypop(s->instances,ins)) {
   	ins->ev=ev;
		ins->flags=READY;
		lstage_pushinstance(ins);
		lua_pushboolean(L,1);
		return 1;
   } else if(lstage_lfqueue_trypush(s->event_queue,ev)) {
      lua_pushboolean(L,1);
      return 1;
   } 
   lstage_destroyevent(ev);
   lua_pushnil(L);
   lua_pushliteral(L,"Event queue is full");
   return 2;
}

/*static int set_max_instances(lua_State * L) {
	stage_t s=lstage_tostage(L,1);
	if(s->stateful) luaL_error(L,"Cannot alter the number of instances of a stateful stage");
	luaL_checktype (L, 2, LUA_TNUMBER);
	int capacity=lua_tointeger(L,2);
	lstage_lfqueue_setcapacity(s->instances,capacity);
	return 0;
}*/

/*tostring method*/
static int stage_tostring (lua_State *L) {
  stage_t * s = luaL_checkudata (L, 1, LSTAGE_STAGE_METATABLE);
  lua_pushfstring (L, "Stage (%p)", *s);
  return 1;
}

static int stage_getid (lua_State *L) {
	stage_t s = lstage_tostage(L, 1);
	lua_pushlstring(L,(const char *)&s,sizeof(void*));
	return 1;
}

static int stage_queue_size (lua_State *L) {
	stage_t s = lstage_tostage(L, 1);
	lua_pushnumber(L,lstage_lfqueue_size(s->event_queue));
	return 1;
}

static int stage_destroyinstances(lua_State * L) {
	stage_t s = lstage_tostage(L, 1);
	int n=lua_tointeger(L,2);
	int i;
	if(n<=0) luaL_error(L,"Argument must be grater than zero");
	//TODO warning thread_unsafe, mutex needed (or use it in only one thread)
	if(lstage_lfqueue_getcapacity(s->instances)-n<0)
		luaL_error(L,"Cannot destroy this number of instances");
	for(i=0;i<n;i++) {
		instance_t i;
		if(!lstage_lfqueue_trypop(s->instances,i)) break;
		lstage_destroyinstance(i); //should not longjmp
	}
	lstage_lfqueue_setcapacity(s->instances,lstage_lfqueue_getcapacity(s->instances)-i);
	//unlock mutex
	lua_pushnumber(L,i);
	return 1;
}

static int stage_instantiate(lua_State * L) {
	stage_t s = lstage_tostage(L, 1);
	int n=lua_tointeger(L,2);
	int i;
	if(n<=0) luaL_error(L,"Argument must be grater than zero");
	//TODO warning thread_unsafe, mutex needed (or use it in only one thread)
	lstage_lfqueue_setcapacity(s->instances,lstage_lfqueue_getcapacity(s->instances)+n);
	for(i=0;i<n;i++) {
		(void)lstage_newinstance(s);
	}
	//unlock mutex
	return 0;
}

static int stage_ptr(lua_State * L) {
	stage_t * s = luaL_checkudata (L, 1, LSTAGE_STAGE_METATABLE);
	lua_pushlightuserdata(L,*s);
	return 1;
}

void lstage_buildstage(lua_State * L,stage_t t) {
	stage_t *s=lua_newuserdata(L,sizeof(stage_t *));
	*s=t;
	get_metatable(L);
   lua_setmetatable(L,-2);
}

static int stage_getparent(lua_State * L) {
	stage_t s = lstage_tostage(L, 1);
	if(s->parent)
		lstage_buildstage(L,s->parent);
	else
		lua_pushnil(L);
	return 1;
}

static void get_metatable(lua_State * L) {
	luaL_getmetatable(L,LSTAGE_STAGE_METATABLE);
   if(lua_isnil(L,-1)) {
   	lua_pop(L,1);
  		luaL_newmetatable(L,LSTAGE_STAGE_METATABLE);
  		lua_pushvalue(L,-1);
  		lua_setfield(L,-2,"__index");
		lua_pushcfunction (L, stage_tostring);
		lua_setfield (L, -2,"__tostring");
		luaL_loadstring(L,"local ptr=(...):ptr() return function() return require'lstage.stage'.get(ptr) end");
		lua_setfield (L, -2,"__wrap");
//		lua_pushcfunction (L, lstage_destroystage); //TODO implement refcount?
//		lua_setfield (L, -2,"__gc");
  		lua_pushcfunction(L,get_max_instances);
  		lua_setfield(L,-2,"instances");
//		lua_pushcfunction(L,set_max_instances);
//  		lua_setfield(L,-2,"set_instances");
  		lua_pushcfunction(L,get_queue_capacity);
  		lua_setfield(L,-2,"capacity");
  		lua_pushcfunction(L,set_queue_capacity);
  		lua_setfield(L,-2,"set_capacity");
  		lua_pushcfunction(L,stage_getid);
  		lua_setfield(L,-2,"id");
  		lua_pushcfunction(L,stage_getenv);
  		lua_setfield(L,-2,"env");
  		lua_pushcfunction(L,stage_push);
  		lua_setfield(L,-2,"push");
  		lua_pushcfunction(L,stage_queue_size);
  		lua_setfield(L,-2,"queue_size");
  		lua_pushcfunction(L,stage_instantiate);
  		lua_setfield(L,-2,"instantiate");
  		lua_pushcfunction(L,stage_destroyinstances);
  		lua_setfield(L,-2,"free");
  		lua_pushcfunction(L,stage_ptr);
  		lua_setfield(L,-2,"ptr");
  		lua_pushcfunction(L,stage_getparent);
  		lua_setfield(L,-2,"parent");
  	}
}


static int stage_isstage(lua_State * L) {
	lua_getmetatable(L,1);
	get_metatable(L);
	int has=0;
   #if LUA_VERSION_NUM > 501
	if(lua_compare(L,-1,-2,LUA_OPEQ)) has=1;
	#else
	if(lua_equal(L,-1,-2)) has=1;
	#endif
	lua_pop(L,2);
   lua_pushboolean(L,has);
	return 1;
}

static int lstage_newstage(lua_State * L) {
   luaL_checktype (L, 1, LUA_TFUNCTION);
   int idle=luaL_optint(L, 2, 1);
   int capacity=luaL_optint(L, 3, DEFAULT_QUEUE_CAPACITY);
   lua_pushcfunction(L,mar_encode);
   lua_pushvalue(L,1);
   lua_call(L,1,1);
   size_t len=0; const char *env=lua_tolstring(L,-1,&len);
   lua_pop(L,1);
   stage_t * stage=lua_newuserdata(L,sizeof(stage_t *));
   (*stage)=malloc(sizeof(struct lstage_Stage));   
   (*stage)->instances=lstage_lfqueue_new();
   lstage_lfqueue_setcapacity((*stage)->instances,0);
   (*stage)->event_queue=lstage_lfqueue_new();
   lstage_lfqueue_setcapacity((*stage)->event_queue,capacity);
   char *envcp=malloc(len+1);
   envcp[len]='\0';
   memcpy(envcp,env,len+1);
   (*stage)->env=envcp;
   (*stage)->env_len=len;
  	get_metatable(L);
   lua_setmetatable(L,-2);
   if(idle>0) {
	   lua_pushcfunction(L,stage_instantiate);
	   lua_pushvalue(L,-2);
	   lua_pushnumber(L,idle);
	   lua_call(L,2,0);
   }
   (*stage)->parent=NULL;
   lua_pushliteral(L,LSTAGE_INSTANCE_KEY);
   lua_gettable(L, LUA_REGISTRYINDEX);	
	if(lua_type(L,-1)==LUA_TLIGHTUSERDATA) {
		instance_t i=lua_touserdata(L,-1);
	   (*stage)->parent=i->stage;
	}
	lua_pop(L,1);
   qt_hash_put(H,(*stage),(*stage));
   return 1;
}

static int lstage_destroystage(lua_State * L) {
	stage_t * s_ptr = luaL_checkudata (L, 1, LSTAGE_STAGE_METATABLE);
	if(!s_ptr) return 0;
	if(!(*s_ptr)) return 0;
	stage_t s=*s_ptr;
	qt_hash_remove(H,s);
	free(s->env);
	lstage_lfqueue_free(s->instances);
	lstage_lfqueue_free(s->event_queue);
	*s_ptr=0;
	return 0;
}

static int lstage_getstage(lua_State * L) {
	stage_t s=lua_touserdata(L,1);
	if(s) {
		lstage_buildstage(L,s);
		return 1;
	}
	lua_pushnil(L);
	lua_pushliteral(L,"Stage not found");
	return 2;
}

static void dump_hashtable(const qt_key_t k, void *v, void *l) {
	lua_State *L=l;
	int n=lua_tonumber(L,-1)+1;
	lua_pop(L,1);
	lua_pushnumber(L,n);
	lua_pushnumber(L,n);
	lstage_buildstage(L,v);
	lua_settable(L,-4);
}

static int stage_getall(lua_State * L) {
	lua_newtable(L);
	lua_pushnumber(L,0);
	qt_hash_callback(H, dump_hashtable, L);
	lua_pop(L,1);
	return 1;
}

LSTAGE_EXPORTAPI	int luaopen_lstage_stage(lua_State *L) {
	const struct luaL_Reg LuaExportFunctions[] = {
		{"new",lstage_newstage},
		{"get",lstage_getstage},
		{"destroy",lstage_destroystage},
		{"is_stage",stage_isstage},
		{"all",stage_getall},
		{NULL,NULL}
	};
	if(!H) H=qt_hash_create();
	lua_newtable(L);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'lstage.stage' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif        
	return 1;
};
