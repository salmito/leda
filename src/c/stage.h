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

#define LEDA_STAGE_META "leda-Stage *"
int leda_newstage(lua_State * L);

#endif
