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
#include "extra/lmarshal.h"

#define __VERSION "0.2.0-beta5"

#define CONNECTOR_TIMEOUT 2.0

#define LEDA_NAME          "leda.kernel"
#define LEDA_ENV           "leda kernel environment"

/* initialized flag, 'true' if already initialized */
bool_t initialized=FALSE;

SIGNAL_T queue_used_cond;
MUTEX_T queue_used_lock;
extern queue ready_queue;
extern queue * event_queues;

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
   main_graph=build_graph_representation(L,1);
   #ifdef DEBUG
   graph_dump(main_graph);
   #endif


   //initiate instances for the graph
   //queues are initially unbounded (-1 limit)
   instance_init(-1,-1);

   SIGNAL_INIT(&queue_used_cond);
   MUTEX_INIT(&queue_used_lock);
   
   //second parameter must be a table for controller
   luaL_checktype(L,2, LUA_TTABLE);

   //first, iterate through the connectors field
   //to push pending sends to the each connector.
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
      for(j=1;j<=m;j++) { //For each connector
         lua_getglobal(L,"unpack"); //Push unpack function
         int begin=lua_gettop(L);
         lua_rawgeti(L,-2,j);
         lua_call(L,1,LUA_MULTRET); //Unpack the pending arguments
         int end=lua_gettop(L);
         lua_pushliteral(L,"consumers");
         lua_rawget(L, con_index); 
         luaL_checktype(L,-1, LUA_TTABLE);      
         int o=lua_objlen(L,-1);
         int k;

         for(k=1;k<=o;k++) { //For each consumer of the pending connector
            lua_rawgeti(L,-1,k);
            stage_id dst_id=get_stage_id_from_ptr(main_graph, (void *)lua_topointer(L,-1));
            lua_pop(L,1);
            _DEBUG("Kernel: Passing pending data to stage %d\n",(int)dst_id);

            instance dst=instance_aquire(dst_id);
            
            if(!dst) {   //error getting an instance from the 
                        //recycle queue, try to emmit an event insted
               event e=extract_event_from_lua_state(L, begin, end-begin+1);
               if(!instance_try_push_pending_event(NULL,dst_id,e)) {
                  //error, event queue is full, push FALSE to sender
                  luaL_error(L,"Event queue for the stage '%s' is full.",
                  main_graph->s[dst_id]->name);
               }
            } else {
               //got an idle instance from recycle queue  
               //Get the  main coroutine of the instance's handler
               lua_getglobal(dst->L, "handler");
               //push arguments to instance
               copy_values_directly(dst->L, L, begin, end-begin+1);
               dst->args=end-begin+1;
               push_ready_queue(dst);
            }
            lua_settop(L,begin-1);
         }
      }
      //Pop the connector pending data table from the stack
      lua_pop(L,1);
      //clear pending data for connector
      lua_pushliteral(L,"pending");
      lua_newtable(L);
      lua_rawset(L,-3);
      //Pop the connector table from the stack
      lua_pop(L,1);
   }
   //Pop connectors table
   lua_pop(L,1);

   //call the init function of a controller, if defined
   lua_pushstring(L,"init");
   lua_rawget(L, 2);
   if(lua_type(L,-1)==LUA_TFUNCTION)
      lua_call(L,0,0);
   else 
      lua_pop(L,1);

   lua_pushstring(L,"event_pushed");
   lua_rawget(L, 2);
   if(lua_type(L,-1)==LUA_TFUNCTION) {
      /*if the controller defined a push_event function, 
      * wait for a ready_queue_used signal and call it
      */
      while(1) {
         time_d timeout=SIGNAL_TIMEOUT_PREPARE(CONNECTOR_TIMEOUT);
         bool_t timedout=SIGNAL_WAIT(&queue_used_cond,&queue_used_lock,timeout);
//	      printf("AQUI\n");
//         bool_t timedout=FALSE;
         lua_pushvalue(L,-1);
         lua_pushboolean(L,!timedout);
         lua_call(L,1,0);
         //comment the line below to disable end condition
         if(READ(pool_size)==-queue_size(ready_queue)) break;
         //while(1) usleep(1000000000);
      }
   } else { /*if the controller did not define a push_event function, 
             * sleep forever
             */
      while(1) usleep(1000000000);
   }

   lua_pop(L,1); //pop event_pushed function   
   
   //call the collect function of a controller, if defined
   lua_pushstring(L,"finish");
   lua_rawget(L, 2);
   if(lua_type(L,-1)==LUA_TFUNCTION) 
      lua_call(L,0,0);
   else 
      lua_pop(L,1);
      
   usleep(200000); //give some time for threads to kill themselves
  
   SIGNAL_FREE(&queue_used_cond);
   MUTEX_FREE(&queue_used_lock);
  
  
   //Finished running the graph, cleanup kernel runtime
   //Cleanup instance subsystem
   instance_end();
   //Destroy graph internal representation
   graph_destroy(main_graph);
   main_graph=NULL;
   //Return TRUE
   lua_pushboolean(L,TRUE);
   return 1;
}

/* Kernel Lua function to get the size of the ready queue*/
int leda_ready_queue_size(lua_State * L) {
   lua_pushinteger(L,queue_size(ready_queue));
   return 1;
}

int leda_ready_queue_isempty(lua_State * L) {
   lua_pushboolean(L,queue_isempty(ready_queue));
   return 1;
}


/* Kernel Lua function to sleep for a time in miliseconds*/
int leda_sleep(lua_State * L) {
   lua_Number n=lua_tonumber(L,1);
   usleep((int)(n*1000000.0));
   return 0;
}

/* Kernel Lua function to get the pointer of a lua value 
 * (used as a unique name for objects)
 */
int leda_to_pointer(lua_State * L) {
   lua_pushfstring(L,"%p",lua_topointer(L,1));
   return 1;
}

/* Get the size of the thread pool */
int leda_get_thread_pool_size(lua_State * L) {
   lua_pushinteger(L,READ(pool_size));
   return 1;
}

/* Leda's kernel info  */
static void set_leda_info (lua_State *L) {
   lua_getglobal(L,"leda");
	lua_pushliteral (L, "_DESCRIPTION");
	lua_pushliteral (L, "Leda");
	lua_settable (L, -3);
	lua_pushliteral (L, "_VERSION");
	lua_pushliteral (L, __VERSION);
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
  	   {"encode", mar_encode},
  	   {"decode", mar_decode},
  	   {"clone", mar_clone},
  	   {"sleep", leda_sleep},

  	   //functions for controllers
 	   {"ready_queue_size", leda_ready_queue_size},
  	   {"thread_pool_size", leda_get_thread_pool_size},
// 	   {"event_queue_size", leda_event_queue_size},
	   

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
	REGISTER_LEDA(L, LEDA_NAME, leda_funcs);
	lua_pop(L,1);
   lua_pushliteral(L, LEDA_ENV);
	lua_newtable (L);
	lua_settable (L, LUA_REGISTRYINDEX);
	set_leda_info (L);

 	/* Create a unique thread metatable */
   thread_createmetatable(L);

//   luaopen_leda_io (L);

   _DEBUG("Kernel: Leda's kernel loaded successfully.\n");
 	return 1;
}

