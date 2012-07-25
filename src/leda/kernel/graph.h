/*
===============================================================================

Copyright (C) 2012 Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

===============================================================================
*/
#ifndef _GRAPH_H_
#define _GRAPH_H_

#define GRAPH_METATABLE   "leda graph"
#include <lua.h>

#include "extra/threading.h"

typedef size_t stage_id;
typedef size_t connector_id;

typedef struct {
   union {
      char const * c;
      lua_Number n;
   } key;
   enum {_STRING,_NUMBER} type;
   connector_id value;
} key;

/* leda stage structure */ 
typedef struct stage_data {
   char const * name;
   char const * handler;
   char const * init;
   size_t init_len;
   size_t handler_len;
   key * output;
   size_t n_out;
   connector_id input;
   void * unique_id;
   bool_t serial;
} * stage;


typedef struct connector_data {
   char const * name;
   stage_id * p;
   size_t n_p;
   stage_id * c;
   size_t n_c;
   char const * send;
   size_t send_len;
   void * unique_id;
} * connector;


//graph read only representation (thread-safe)
typedef struct graph_data {
   stage * s;
   size_t n_s;
   connector * c;
   size_t n_c;
   char const * name;
} * graph;

extern graph main_graph;

stage_id get_stage_id_from_ptr(graph g, void * id);
connector_id get_connector_id_from_ptr(graph g, void * id);

connector graph_get_connector(graph g,connector_id id);
stage graph_get_stage(graph g, stage_id id);
graph build_graph_representation(lua_State *L,int i);
void graph_destroy(graph g);
void graph_dump(graph g);

#endif //_GRAPH_H_
