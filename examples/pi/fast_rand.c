#include <stdio.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

typedef unsigned long long u64b;

static u64b rng64b(u64b *s)
{
	u64b x;

	asm volatile(
	"mov    $0x6595a395a1ec531b,%%rcx\n"
	"movl   0xc(%1),%k0\n"
	"add    %%rcx,(%1)\n"
	"adc    %%rcx,0x8(%1)\n"
	"xor	0x8(%1),%0\n"
	"xor    %1,%0\n" //Remove this line if you are single threaded
	"imul   %%rcx,%0\n"
	"mov    %0,%%rdx\n"
	"shr    $0x20,%0\n"
	"xor    %%rdx,%0\n"
	"imul   %%rcx,%0\n"
	"add    (%1),%0\n"
	:"=r" (x): "r"(s): "%rcx", "%rdx", "memory", "cc");
	return x;
}

static u64b seed[2];
#define RAND_MAX_F 10000000000000000LL

/* Sample usage */
int seed_f(lua_State * L)
{
	/* Initialize seed */
	seed[0] = lua_tointeger(L,1);
	seed[1] = lua_tointeger(L,2);
	return 0; 
}

int rand_f(lua_State * L) {
	u64b result;
	/* Get a random 64bit integer */
	result = rng64b(seed);
	lua_pushnumber(L,(double)(result%RAND_MAX_F)/RAND_MAX_F);
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
