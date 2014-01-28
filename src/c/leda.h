/*Adapted from https://github.com/Tieske/Lua_library_template/*/

/*
** ===============================================================
** Leda is a parallel and concurrent framework for Lua.
** Copyright 2014: Tiago Salmito
** License MIT
** ===============================================================
*/

#ifndef leda_h
#define leda_h

#include <lua.h>
#include <lauxlib.h>

#define LEDA_LIBRARYNAME leda_library

#ifdef DEBUG
void stackDump (lua_State *L, const char *text);
void tableDump(lua_State *L, int idx, const char* text);
#else
#define stackDump() 
#define tableDump() 
#endif

#ifndef LEDA_EXPORTAPI
        #ifdef _WIN32
                #define LEDA_EXPORTAPI __declspec(dllexport)
        #else
                #define LEDA_EXPORTAPI extern
        #endif
#endif  

#ifdef DEBUG
	#define _DEBUG() 
#else
	#define _DEBUG() 
#endif

#define LEDA_STAGE_META "leda-Stage *"

#endif
