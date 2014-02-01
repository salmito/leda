#include "leda.h"
#include "marshal.h"
#include "threading.h"

#ifdef DEBUG
//can be found here  http://www.lua.org/pil/24.2.3.html
void stackDump (lua_State *L, const char *text) {
      int i;
      int top = lua_gettop(L);
	  if (text == NULL)
		printf("--------Start Dump------------\n");
	  else
	    printf("--------Start %s------------\n", text);
      for (i = 1; i <= top; i++) {  /* repeat for each level */
        int t = lua_type(L, i);
        switch (t) {
    
          case LUA_TSTRING:  /* strings */
            printf("`%s'", lua_tostring(L, i));
            break;
    
          case LUA_TBOOLEAN:  /* booleans */
            printf(lua_toboolean(L, i) ? "true" : "false");
            break;
    
          case LUA_TNUMBER:  /* numbers */
            printf("%g", lua_tonumber(L, i));
            break;
    
          default:  /* other values */
            printf("%s", lua_typename(L, t));
            break;
    
        }
        printf("  ");  /* put a separator */
      }
      printf("\n");  /* end the listing */
	  printf("--------End Dump------------\n");
    }

void tableDump(lua_State *L, int idx, const char* text)
{
	lua_pushvalue(L, idx);		// copy target table
	lua_pushnil(L);
	  if (text == NULL)
		printf("--------Table Dump------------\n");
	  else
	    printf("--------Table dump: %s------------\n", text);
	while (lua_next(L,-2) != 0) {
		printf("%s - %s\n",
			lua_typename(L, lua_type(L, -2)),
			lua_typename(L, lua_type(L, -1)));
		lua_pop(L, 1);
	}
	lua_pop(L, 1);	// remove table copy
    printf("--------End Table dump------------\n");
}
#endif

static int leda_version(lua_State * L) {
	lua_pushliteral(L,LEDA_VERSION);
	return 1;
}

static int leda_gettime(lua_State * L) {
   lua_pushnumber(L,now_secs());
   return 1;
}

static int leda_cpus(lua_State *L) {
   #ifdef _WIN32
      #ifndef _SC_NPROCESSORS_ONLN
         SYSTEM_INFO info;
         GetSystemInfo(&info);
         #define sysconf(a) info.dwNumberOfProcessors
         #define _SC_NPROCESSORS_ONLN
      #endif
   #endif
   #ifdef _SC_NPROCESSORS_ONLN
	   long nprocs = -1;
	   long nprocs_max = -1;
      nprocs = sysconf(_SC_NPROCESSORS_ONLN);
      if (nprocs < 1) {
         luaL_error(L,"Could not determine number of CPUs online:\n%s\n", 
            strerror (errno));
      }
      nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
      if (nprocs_max < 1) {
         luaL_error(L, "Could not determine number of CPUs configured:\n%s\n", 
            strerror (errno));
      }
      lua_pushnumber(L,nprocs);
      lua_pushnumber(L,nprocs_max);
      return 2;
   #else
      luaL_error(L, "Could not determine number of CPUs");
   #endif
   return 0;
}


LEDA_EXPORTAPI	int luaopen_leda_event(lua_State *L);
LEDA_EXPORTAPI	int luaopen_leda_scheduler(lua_State *L);
LEDA_EXPORTAPI	int luaopen_leda_stage(lua_State *L);

LEDA_EXPORTAPI	int luaopen_leda_new(lua_State *L) {
	const struct luaL_Reg LuaExportFunctions[] = {
	{"_VERSION",leda_version},
	{"now",leda_gettime},
	{"cpus",leda_cpus},
	{NULL,NULL}
	};
	lua_newtable(L);
	lua_getglobal(L,"require");
	lua_pushliteral(L,"leda.event");
	lua_call(L,1,1);
	lua_getfield(L,-1,"encode");
	lua_setfield(L,-3,"encode");
	lua_getfield(L,-1,"decode");
	lua_setfield(L,-3,"decode");
	lua_pop(L,1);
	lua_getglobal(L,"require");
	lua_pushliteral(L,"leda.scheduler");
	lua_call(L,1,1);
	lua_setfield(L,-2,"scheduler");
	lua_getglobal(L,"require");
	lua_pushliteral(L,"leda.stage");
	lua_call(L,1,1);
	lua_getfield(L,-1,"new");
	lua_setfield(L,-3,"stage");
	lua_pop(L,1);
	lua_newtable(L);
	luaL_loadstring(L,"return function() return require'leda.new' end");
	lua_setfield (L, -2,"__persist");
	lua_setmetatable(L,-2);
#if LUA_VERSION_NUM < 502
	luaL_register(L, NULL, LuaExportFunctions);
#else
	luaL_setfuncs(L, LuaExportFunctions, 0);
#endif        
	return 1;
};


