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
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include <stdlib.h>

#include "instance.h"
#include "thread.h"
#include "queue.h"
#include "mutex.h"

int leda_sleep(lua_State * L);

/*
* Recycle queue vector, initialized in the instace_init() function
* It holds one instance queue for each different stage on the graph.
*/
queue * recycle_queues;
queue * pending_queues;

/* Graph read-only gobal representation of the running graph */
//g=NULL;

/* Lua code for the instance states (baked in).
 * Contanins a byte array for automatic
 * loading with 'lua_dobuffer'.
 *
 * see file: extra/bin2c.lua.
 */
static char const instance_chunk[]= 
#include "instance.lo"

/*Create an empty new lua_state and returns it */
lua_State * new_lua_state(bool_t libs) {
   lua_State * L = luaL_newstate();

   /*TODO 'lua.c' stop the collector during initialization, as pointed out
    *           by the author the lualanes code. This is done in the following files:
    *            @lua.c pmain line: 332 (lua 5.1.4)
    *            @lua.c pmain line: 456 (lua 5.2.1)
    *
    *           so i'm doing in here too
    */

   lua_gc( L, LUA_GCSTOP, 0);
   
   if(libs) luaL_openlibs(L);
   else {
      //TODO pehaps it's good to load only the base library in this case?
   }
   
   lua_gc( L, LUA_GCRESTART, 0);   
	return L;
}

/* Destroy a lua_state, GCing all of its members. */
void destroy_lua_state(lua_State * L) {
   lua_close(L); 
}

/* Destroy an instance, i.e. destroy its lua_state and free its memory. */
void instance_destroy(instance i) {
   destroy_lua_state(i->L);
   free(i);
}

#define STAGE(i) main_graph->s[i]
#define CONNECTOR(i) main_graph->c[i]

/* Initialize instance subsystem
 * Params:     'g_par'  read-only graph representation 
 *
 *             'limit'  sets maximum size of all recycle queues created
 *                      -1    unbounded queue size
 *                      0     no instances will be stored
 *                      >0    sets size to 'limit'
 */
void instance_init(size_t limit){
   int i;
   recycle_queues=calloc(main_graph->n_s,sizeof(queue));
   pending_queues=calloc(main_graph->n_s,sizeof(queue));

   for(i=0;i<main_graph->n_s;i++) {
      recycle_queues[i]=queue_new();
      //if serial flag is detected, set the capacity of its recycle queue to 1
      //and prealocate an exclusive instance for it
      if(STAGE(i)->serial) {
         pending_queues[i]=queue_new();
         queue_set_capacity(pending_queues[i],limit);
         queue_set_capacity(recycle_queues[i],1);
         STAGE(i)->serial=FALSE;
         instance serial=instance_aquire(i);
         STAGE(i)->serial=TRUE;
         serial->serial=TRUE;
         instance_release(serial);
      } else {
         queue_set_capacity(recycle_queues[i],limit);
      }
   }
}

void instance_try_push_pending_queue(stage_id serial_stage, instance i) {
   TRY_PUSH(pending_queues[serial_stage],i);
}

/* Try to aquire an instance from its correspondent recycle queue
 * if there is no instance available in the recycle queue, create a 
 * a new instance, initialize-it with the stage method and returns it
 *
 * Warning: can return 'NULL' in case of error
 */
instance instance_aquire(stage_id s) {
   instance ret;
   if(s<0 && s>main_graph->n_s) return NULL; //error: invalid stage
   
   // if serial flag is detected for stage and the recycle queue is not empty, 
   // block on the recycle queue
   if(STAGE(s)->serial) {
      _DEBUG("Instance: Waiting to aquire a instance of a serial stage id='%d'\n",(int)s);
      if(!TRY_POP(recycle_queues[s],ret)) {
         _DEBUG("Instance: tried to aquire an serial stage but it was not available id='%d'\n",(int)s);
         return NULL;
      }
      _DEBUG("Instance: Serial instance aquired stage id='%d'\n",(int)s);
      return ret;
   } else {
      //Try to recycle an instance from its correspondent queue
      if(TRY_POP(recycle_queues[s],ret))
         return ret; //instance recycled
   }
   
   //If not (beacuse the recycle queue was empty) create
   //a new instance and returns it
   ret=calloc(1,sizeof(struct instance_data));
   
   ret->L=new_lua_state(TRUE);
   ret->stage=s;
   
   lua_pushlightuserdata( ret->L, ret);
	lua_setfield( ret->L, LUA_REGISTRYINDEX, "__SELF" );
   
   if(luaL_loadbuffer( ret->L, instance_chunk, sizeof(instance_chunk), "@instance.lua"))
			return NULL; //"luaL_loadbuffer() failed";   // LUA_ERRMEM
   
   //Create a stage representation
   lua_newtable(ret->L); //Table holds all information

   //Push the init and handler chunks into the table
   lua_pushliteral(ret->L,"__name"); //key
   lua_pushstring (ret->L, STAGE(s)->name); //value
   lua_settable(ret->L,-3); //Set __name


   //Push the init and handler chunks into the table
   lua_pushliteral(ret->L,"__init"); //key
   lua_pushlstring (ret->L, STAGE(s)->init, STAGE(s)->init_len); //value
   lua_settable(ret->L,-3); //Set __init
   
   lua_pushliteral(ret->L,"__handler"); //key
   lua_pushlstring (ret->L, STAGE(s)->handler, STAGE(s)->handler_len); //value
   lua_settable(ret->L,-3); //Set __handler
   
   //push output table __output[id]={sendf,{ids}}
   lua_pushliteral(ret->L,"__output"); //key
   lua_newtable(ret->L);  //value
   int j;
   for(j=0;j<STAGE(s)->n_out;j++) {
      switch(STAGE(s)->output[j].type) {
      case _STRING:
         //string key of __output[key]=value
         lua_pushstring(ret->L,STAGE(s)->output[j].key.c); 
         break;
      case _NUMBER:
         //numeric key of __output[key]=value
         lua_pushnumber(ret->L,STAGE(s)->output[j].key.n); 
         break;
      }
      lua_newtable(ret->L); //value of __output[key]=value

      connector_id c=STAGE(s)->output[j].value;
            
      lua_pushliteral(ret->L,"__sendf"); //key
      lua_pushlstring (ret->L, CONNECTOR(c)->send, CONNECTOR(c)->send_len); //value
      lua_settable(ret->L,-3); //Set __output[key].__sendf

      //put consumers ids
      lua_pushliteral(ret->L,"__consumers"); //key
      lua_newtable(ret->L); //value __output[key].__consumers
      int k;
      for(k=1;k<=CONNECTOR(c)->n_c;k++) {
         lua_pushinteger(ret->L,(int)CONNECTOR(c)->c[k-1]); //consumer[k-1]
         lua_rawseti(ret->L,-2,k);
      }
      lua_settable(ret->L,-3); //Set __output[key].__consumers=value
      
      lua_settable(ret->L,-3); //Set __output[key]=value
   }
   lua_settable(ret->L,-3); //Set __output
   
   /* Push the kernel C API functions to the newly created lua_State */
   lua_setglobal(ret->L,"stage");
   /* C methods for passing control to instances */
   lua_pushcfunction(ret->L,call);
   lua_setglobal(ret->L,"__call");

   lua_pushcfunction(ret->L,emmit);
   lua_setglobal(ret->L,"__emmit");

   lua_pushcfunction(ret->L,emmit_self_call);
   lua_setglobal(ret->L,"__emmit_self_call");
   
   /* C method for new mutexes*/
   lua_newtable(ret->L);
   lua_pushliteral(ret->L,"new");
   lua_pushcfunction(ret->L,mutex_new);
   lua_rawset(ret->L,-3);
   lua_pushliteral(ret->L,"destroy");
   lua_pushcfunction(ret->L,mutex_destroy);
   lua_rawset(ret->L,-3);
   lua_pushliteral(ret->L,"lock");
   lua_pushcfunction(ret->L,mutex_lock);
   lua_rawset(ret->L,-3);
   lua_pushliteral(ret->L,"unlock");
   lua_pushcfunction(ret->L,mutex_unlock);
   lua_rawset(ret->L,-3);
   lua_setglobal(ret->L,"__mutex");

   lua_pushnumber(ret->L,ENDED);   
   lua_setglobal(ret->L,"__end_code");
   
   lua_pushcfunction(ret->L,call);
   lua_setglobal(ret->L,"__call");
   
   lua_pushcfunction(ret->L,leda_sleep);
   lua_setglobal(ret->L,"__sleep");

   lua_pushcfunction(ret->L,leda_sleep);
   lua_setglobal(ret->L,"__sleep");
   
   /*lua_pushcfunction(ret->L,wrap_test);
   lua_setglobal(ret->L,"wrap");
   
   lua_pushcfunction(ret->L,unwrap_test);
   lua_setglobal(ret->L,"unwrap");*/

   
   /* Call the lua_chunk loaded in the luaL_loadbuffer */
   lua_call( ret->L, 0 , 0 );
  
   _DEBUG("Instance created for stage '%d' name='%s'\n",(int)s,STAGE(s)->name);

   return ret;
}

/* Release a instance, if the queue is full (TRY_PUSH will fail),
 * destroy it.
 */
void instance_release(instance i) {
   if(STAGE(i->stage)->serial) {
      TRY_PUSH(recycle_queues[i->stage],i); 
      _DEBUG("Instance: Serial instance released. id='%d'\n",(int)i->stage);
      instance pending;
      if(TRY_POP(pending_queues[i->stage],pending)) { 
         //There are pending instances waiting for this serial stage
         //Try to execute it again now that the serial stage has been released
         _DEBUG("Instance: Trying to call pending instance. id='%d'\n",(int)pending->stage);
         emmit_and_continue(pending);
      }
      return;
   }
   if(!TRY_PUSH(recycle_queues[i->stage],i)) {
      _DEBUG("Instance: Destroying instance of the stage id='%d'.\n",(int)i->stage);
      instance_destroy(i);
      return; 
   }
   _DEBUG("Instance: Instance released. id='%d'\n",(int)i->stage);
}

/* Cleanup the instance subsystem and destroy all recycle queues.
 *
 * Warning: Thread unsafe. Every other threads must be terminated
 * before calling this function.
 */
void instance_end() {
   int i;
   for(i=0;i<main_graph->n_s;i++) { 
         queue_free(recycle_queues[i]);
   }
}
