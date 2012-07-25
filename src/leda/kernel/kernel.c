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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "thread.h"
#include "graph.h"
#include "queue.h"
#include "instance.h"

#define LEDA_NAME          "leda.kernel"
#define LEDA_ENV           "leda kernel environment"

/* initialized flag, 'true' if already initialized */
static bool_t initialized=FALSE;

/*
* Run a graph defined by the lua declaration
*
* Returns: 'true' in case of sucess
*          'nil' in case of error, with an error message
*/
int leda_run(lua_State * L) {
   //check if sirst argument is a table (with a graph representation)
   luaL_checktype(L,1, LUA_TTABLE);
   //build graph representation
   graph g=build_graph_representation(L,1);
   //graph_dump(g);
   
   //initiate instances for the graph
   //recycle queue is initially unbounded
   instance_init(g,-1);

   //first, iterate through the connectors field
   //to push pending sends to the each connector
   //each send pushes a correspondent instance to 
   //the ready_queue
   lua_pushstring(L,"connectors");
   lua_rawget(L, 1);
   luaL_checktype(L,-1, LUA_TTABLE);
   int n=lua_objlen(L,-1); //get size of 'connectors' field
   int i;
   for(i=1;i<=n;i++) {
      lua_rawgeti(L,-1,i); //push g.connectors[i]
      int con_index=lua_gettop(L);
      lua_pushliteral(L,"pending");
      lua_rawget(L, -2); //push g.connectors[i].pending
      luaL_checktype(L,-1, LUA_TTABLE);      
      int m=lua_objlen(L,-1);
      int j;
      for(j=1;j<=m;j++) {
         lua_getglobal(L,"unpack"); //Push unpack function
         int t=lua_gettop(L);
         lua_rawgeti(L,-2,j);
         lua_call(L,1,LUA_MULTRET); //Unpack the pending arguments
         lua_pushliteral(L,"consumers");
         lua_rawget(L, con_index); 
         luaL_checktype(L,-1, LUA_TTABLE);      
         int o=lua_objlen(L,-1);
         int k;
         for(k=1;k<=o;k++) { //For each consumer of the pending connector
            lua_rawgeti(L,-1,k);
            stage_id id=get_stage_id_from_ptr(g, (void *)lua_topointer(L,-1));
            lua_pop(L,1);
            dump_stack(L);
            _DEBUG("Kernel: Passing pending data to stage %d\n",(int)id);
            //Aquire new instance for pending data
            instance inst=instance_aquire(id); 
            //Push the handler main coroutine of the instance
            lua_getglobal(inst->L, "handler");
            //Copy pending arguments to the aquired instance
            copy_values (inst->L, L, t, lua_gettop(L)-1);
            //Set the number arguments copyed
            inst->args=lua_gettop(L)-t;
            //Push instance to the ready queue
            thread_try_push_instance(inst);
            //Restore stack
            lua_settop(L,t-1);
         }         
      }
      //Pop the connector and its pending data table from the stack
      lua_pop(L,2);
   }
   //Pop connectors table
   lua_pop(L,1);

 
   int controllers=lua_gettop(L);

  //then, iterate through the provided controllers
  //and call its init function
   for(i=2;i<=controllers;i++) {
      //push the init function of a controller
      lua_pushstring(L,"init");
      lua_rawget(L, i);
      luaL_checktype(L,-1, LUA_TFUNCTION);
      //Call the init function
      lua_call(L,0,0);
  }
  /* TODO: Waiting for controllers for a wait condition FIXME change this
   */
  for(i=2;i<=controllers;i++) {
      lua_pushstring(L,"wait_condition");
      lua_rawget(L, i);
      luaL_checktype(L,-1, LUA_TFUNCTION);
      lua_call(L,0,0);
  }
  
   //Finished running the graph, cleanup kernel runtime
   //Cleanup instance subsystem
   instance_end();
   //Destroy graph internal representation
   graph_destroy(g);
   //Return TRUE
   lua_pushboolean(L,TRUE);
   return 1;
}

/* Kernel Lua function to get the size of the ready queue*/
int leda_ready_queue_size(lua_State * L) {
   size_t size=thread_ready_queue_size();
   _DEBUG("QUEUE SIZE=%d\n",(int)size);
   lua_pushinteger(L,size);
   return 1;
}

/* Kernel Lua function to sleep for a time in miliseconds*/
int leda_sleep(lua_State * L) {
   lua_Number n=lua_tonumber(L,1);
   usleep((useconds_t)(n*1000000.0));
   return 0;
}

/* Kernel Lua function to get the pointer of a lua value 
 * (used as a unique name for objects)
 */
int leda_to_pointer(lua_State * L) {
   lua_pushfstring(L,"%p",lua_topointer(L,1));
   return 1;
}

/* Leda's kernel info  */
static void set_leda_info (lua_State *L) {
   lua_getglobal(L,"leda");
	lua_pushliteral (L, "_DESCRIPTION");
	lua_pushliteral (L, "Leda");
	lua_settable (L, -3);
	lua_pushliteral (L, "_VERSION");
	lua_pushliteral (L, "Leda 0.2.0");
	lua_settable (L, -3);
	lua_pop(L,1);
}

/* Load the Leda's kernel C API into a lua_State
 */
int luaopen_leda_kernel (lua_State *L) {
	/* Leda's kernel library functions */
	struct luaL_reg leda_funcs[] = {
	   {"run", leda_run},
  	   {"to_pointer", leda_to_pointer},
  	   {"new_thread", thread_new},
 	   {"ready_queue_size", leda_ready_queue_size},
  	   {"sleep", leda_sleep},
		{NULL, NULL},
	};
	
   /* Initialize some debug related variables and the thread subsystem */
   if(!initialized) {
      initialized=TRUE;
      #ifdef DEBUG
      MUTEX_INIT(&debug_lock);
      #endif
      thread_init(-1);
   }
	
	/* Load main library functions */
   _DEBUG("Kernel: Loading leda main API\n");
	luaL_register(L, LEDA_NAME, leda_funcs);
	lua_pop(L,1);
   lua_pushliteral(L, LEDA_ENV);
	lua_newtable (L);
	lua_settable (L, LUA_REGISTRYINDEX);
	set_leda_info (L);

 	/* Create a unique thread metatable */
   thread_createmetatable(L);

   _DEBUG("Kernel: Leda's kernel loaded successfully.\n");
 	return 1;
}

