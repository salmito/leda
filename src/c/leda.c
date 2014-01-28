#include "leda.h"
#include "stage.h"
#include "lf_hash.h"

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

static qt_hash H;

static int leda_getlstage(lua_State * L) {
	luaL_checktype (L, 1, LUA_TNUMBER);
	long long int key=lua_tonumber(L,1);
	stage_t s=qt_hash_get(H,(void *)key);
	if(s) {
		leda_pushstage(L,s);
		return 1;
	}
	lua_pushnil(L);
	lua_pushliteral(L,"Stage not found");
	return 2;
}


static int leda_addstage(lua_State * L) {
	luaL_checktype (L, 1, LUA_TNUMBER);
	long long int key=lua_tonumber(L,1);
   stage_t s=leda_getstage(L,2);
	qt_hash_put(H,(void *)key,s);
	return 0;
}

static const struct luaL_Reg LuaExportFunctions[] = {
	{"stage_new",leda_newstage},
	{"stage_get",leda_getlstage},
	{"stage_add",leda_addstage},
	{NULL,NULL}
};

LEDA_EXPORTAPI	int luaopen_leda_scheduler(lua_State *L){	
	// Export Lua API
	H=qt_hash_create(0);
	lua_newtable(L);
#if LUA_VERSION_NUM < 502
	luaL_register(L, "leda", LuaExportFunctions);
#else
	luaL_setfuncs (L, LuaExportFunctions, 0);
#endif        
	return 1;
};


