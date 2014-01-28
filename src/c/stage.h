/*Adapted from https://github.com/Tieske/Lua_library_template/*/

/*
** ===============================================================
** Leda is a parallel and concurrent framework for Lua.
** Copyright 2014: Tiago Salmito
** License MIT
** ===============================================================
*/

#ifndef stage_h
#define stage_h

typedef struct leda_Stage * stage_t;

#include "leda.h"

enum stage_flag_t {
	DESTROYED=0x01
};

stage_t leda_tostage(lua_State *L, int i);

#endif
