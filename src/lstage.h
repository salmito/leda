/*Adapted from https://github.com/Tieske/Lua_library_template/*/

/*
** ===============================================================
** Leda is a parallel and concurrent framework for Lua.
** Copyright 2014: Tiago Salmito
** License MIT
** ===============================================================
*/

#ifndef _LSTAGE_H
#define _LSTAGE_H

#define LSTAGE_VERSION "1.0.0-beta"

#include <lua.h>
#include <lauxlib.h>

#ifdef DEBUG
void stackDump (lua_State *L, const char *text);
void tableDump(lua_State *L, int idx, const char* text);
#define _DEBUG() 
#else
#define _DEBUG() 
#define stackDump(...) 
#define tableDump(...) 
#endif

#ifndef LSTAGE_EXPORTAPI
        #ifdef _WIN32
                #define LSTAGE_EXPORTAPI __declspec(dllexport)
        #else
                #define LSTAGE_EXPORTAPI extern
        #endif
#endif  

#define LSTAGE_STAGE_METATABLE "ltsage-Stage *"
#define LSTAGE_THREAD_METATABLE "lstage-Thread *"

#endif
