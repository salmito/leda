#include <stdlib.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#include <math.h>

unsigned int s=0;

unsigned long long rdtsc(){
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((unsigned long long)hi << 32) | lo;
}

int seed_f(lua_State * L)
{
	/* Initialize seed */
	//s = lua_tointeger(L,1);
	s = rdtsc();
	return 0; 
}

int rand_f(lua_State * L) {
lua_Number r = (lua_Number)(rand_r(&s)%RAND_MAX) / (lua_Number)RAND_MAX;
  switch (lua_gettop(L)) {  /* check number of arguments */
    case 0: {  /* no arguments */
      lua_pushnumber(L, r);  /* Number between 0 and 1 */
      break;
    }
    case 1: {  /* only upper limit */
      int u = luaL_checkint(L, 1);
      luaL_argcheck(L, 1<=u, 1, "interval is empty");
      lua_pushnumber(L, floor(r*u)+1);  /* int between 1 and `u' */
      break;
    }
    case 2: {  /* lower and upper limits */
      int l = luaL_checkint(L, 1);
      int u = luaL_checkint(L, 2);
      luaL_argcheck(L, l<=u, 2, "interval is empty");
      lua_pushnumber(L, floor(r*(u-l+1))+l);  /* int between `l' and `u' */
      break;
    }
    default: return luaL_error(L, "wrong number of arguments");
  }
  return 1;
}

static const luaL_reg R[] =
{
    {"rand",      rand_f},
    {"seed",      seed_f},
    {NULL,	    NULL}
};

int luaopen_fast_rand(lua_State *L)
{
    lua_newtable(L);
    luaL_register(L, NULL, R);
    return 1;
}
