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
#include "stats.h"
#include "extra/leda-io.h"
#include "extra/lmarshal.h"

int leda_sleep(lua_State * L);

/*defining globals*/
/*
* Recycle queue vector, initialized in the instace_init() function
* It holds one instance queue for each different stage on the graph.
*/
queue * recycle_queues;
queue * event_queues;
atomic * number_of_instances;
int * recycle_queue_limits,* event_queue_limits;
queue ready_queue;

/* defining external signal */
extern SIGNAL_T queue_used_cond;

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

static void registerlib(lua_State * L,char const * name, lua_CFunction f) {
   lua_getglobal(L,"package");
   lua_getfield(L,-1,"preload");
   lua_pushcfunction(L,f);
   lua_setfield(L,-2,name);
   lua_pop(L,2);
}

MUTEX_T require_cs;

/* isolate require function from other threads */
static int new_require(lua_State *L) {
	int rc, i;
	int args = lua_gettop( L);

	lua_pushvalue( L, lua_upvalueindex(1));
	for( i = 1; i <= args; ++ i)
		lua_pushvalue( L, i);

	MUTEX_LOCK( &require_cs);
	rc = lua_pcall( L, args, 1, 0);
	MUTEX_UNLOCK( &require_cs);

	if (rc)
		lua_error(L);   // error message already at [-1]

	return 1;
}

int serialize_require(lua_State * L) {
	lua_getglobal( L, "require" );
	if (lua_isfunction( L, -1 )) {
		lua_pushcclosure( L, new_require, 1);
		lua_setglobal( L, "require" );
	}
	else lua_pop(L,1);
	return 0;
}

static void openlibs(lua_State * L) {
   lua_cpcall(L,luaopen_base,NULL);
   lua_cpcall(L,luaopen_package,NULL);
   registerlib(L,"io", luaopen_io);
   registerlib(L,"os", luaopen_os);
   registerlib(L,"table", luaopen_table);
   registerlib(L,"string", luaopen_string);
   registerlib(L,"math", luaopen_math);
   registerlib(L,"debug", luaopen_debug);   
}

/*Create an empty new lua_state and returns it */
lua_State * new_lua_state(bool_t libs) {
   lua_State * L = luaL_newstate();

   lua_gc( L, LUA_GCSTOP, 0);
   
   if(libs) {
      openlibs(L);
      serialize_require(L);
   }
   else {
      lua_cpcall(L,luaopen_base,NULL);
   }
   lua_gc( L, LUA_GCRESTART, 0);   
	return L;
}

/* Destroy an instance, i.e. destroy its lua_state. */
void instance_destroy(instance i) {
   //Lock protected subtract
   SUB(number_of_instances[i->stage],1);
   time_d t=now_secs()-i->init_time;
   STATS_UPDATE_TIME(i->stage,(int)(t*1000000));
   lua_close(i->L);
   free(i);
}

/* Initialize instance subsystem
 * Params:    'limit'  sets maximum size of all queues created
 *                      -1    unbounded queue size
 *                      0     no instances will be stored
 *                      >0    sets size to 'limit'
 
 * Note: If the serial flag of a stage is raised, only allow one instance
 * to be recycled
 */
void instance_init(size_t recycle_limit_t,size_t pending_limit_t) {
   int i;

   //allocating queue vector (pointers)
   recycle_queues=calloc(main_graph->n_s,sizeof(queue));
   recycle_queue_limits=calloc(main_graph->n_s,sizeof(int));

   event_queues=calloc(main_graph->n_s,sizeof(queue));
   event_queue_limits=calloc(main_graph->n_s,sizeof(int));
   
   number_of_instances = calloc(main_graph->n_s,sizeof(atomic));;
   
   MUTEX_RECURSIVE_INIT( &require_cs );
   
   //for each stage of the graph, initiate queues and set limits
   for(i=0;i<main_graph->n_s;i++) {
      number_of_instances[i]=atomic_new(0);
      
      recycle_queues[i]=queue_new();
      event_queues[i]=queue_new();

      if(STAGE(i)->serial) {
         queue_set_capacity(recycle_queues[i],1);
         recycle_queue_limits[i]=1;
      } else {
         queue_set_capacity(recycle_queues[i],recycle_limit_t);
         recycle_queue_limits[i]=recycle_limit_t;
      }
      queue_set_capacity(event_queues[i],pending_limit_t);
      event_queue_limits[i]=pending_limit_t;
   }
}

bool_t instance_try_push_pending_event(stage_id dst, event e) {
   if(TRY_PUSH(event_queues[dst],e)) {
      SIGNAL_ALL(&queue_used_cond);
      return TRUE;
   }
   //If we're here, there is no space left on the pending event queue

   return FALSE; //Could not send an event (the pending event queue is full)
}

/* Push instance to the ready queue */
void push_ready_queue(instance i) {
   if(i) _DEBUG("Instance: Pushing instance '%d' of stage '%s' onto the ready queue\n",
   i->instance_number,STAGE(i->stage)->name);
   bool_t ret=TRY_PUSH(ready_queue,i);
   if(!ret) {
       /*   The ready queue is FULL.
       *       Block the sender stage until someone frees a
       *       slot on the ready queue.      */
       PUSH(ready_queue,i);
   }
//   SIGNAL_ALL(&queue_used_cond);
}

int instance_wait_for_event(lua_State *L) {
   lua_getfield( L, LUA_REGISTRYINDEX, "__SELF" );
   instance i=lua_touserdata(L,-1);
   lua_pop(L,1);
   
   event e;

   POP(event_queues[i->stage],e);
   int args=restore_event_to_lua_state(L,&e);

   _DEBUG("Instance: Stage '%s' fetched an event with '%d' args directly\n",STAGE(i->stage)->name,args);
//   dump_stack(L);
   return args;
}

int instance_peek_for_event(lua_State *L) {
   lua_getfield( L, LUA_REGISTRYINDEX, "__SELF" );
   instance i=lua_touserdata(L,-1);
   lua_pop(L,1);
   
   event e;
   int args=0;
   if(TRY_POP(event_queues[i->stage],e)){ 
      args=restore_event_to_lua_state(L,&e);
   }
   else {
      lua_pushnil(L);
      lua_pushstring(L,"No pending events available");
      return 2;
   }

   _DEBUG("Instance: Stage '%s' fetched an event with '%d' args directly\n",STAGE(i->stage)->name,args);
//   dump_stack(L);
   return args;
}


void register_mutex_api(lua_State * L) {
   lua_newtable(L);
   lua_pushliteral(L,"new");
   lua_pushcfunction(L,mutex_new);
   lua_rawset(L,-3);
   lua_pushliteral(L,"destroy");
   lua_pushcfunction(L,mutex_destroy);
   lua_rawset(L,-3);
   lua_pushliteral(L,"lock");
   lua_pushcfunction(L,mutex_lock);
   lua_rawset(L,-3);
   lua_pushliteral(L,"unlock");
   lua_pushcfunction(L,mutex_unlock);
   lua_rawset(L,-3);
   lua_setglobal(L,"__mutex");
}

void register_io_api(lua_State * L) {
   lua_newtable(L);
   lua_pushliteral(L,"wrap");
   lua_pushcfunction(L,leda_wrap_io);
   lua_rawset(L,-3);
   lua_pushliteral(L,"unwrap");
   lua_pushcfunction(L,leda_unwrap_io);
   lua_rawset(L,-3);
   lua_setglobal(L,"__io");
   /* AIO epoll */
   lua_newtable(L);
   lua_pushliteral(L,"close");
   lua_pushcfunction(L,epoll_close);
   lua_rawset(L,-3);
   lua_pushliteral(L,"wait");
   lua_pushcfunction(L,epoll_lwait);
   lua_rawset(L,-3);
   lua_pushliteral(L,"remove");
   lua_pushcfunction(L,epoll_remove_descriptor);
   lua_rawset(L,-3);
   lua_pushliteral(L,"add_read");
   lua_pushcfunction(L,epoll_add_read);
   lua_rawset(L,-3);
   lua_pushliteral(L,"add_write");
   lua_pushcfunction(L,epoll_add_write);
   lua_rawset(L,-3);
   lua_pushliteral(L,"add_read_write");
   lua_pushcfunction(L,epoll_add_read_write);
   lua_rawset(L,-3);
   lua_pushliteral(L,"create");
   lua_pushcfunction(L,epoll_lcreate);
   lua_rawset(L,-3);
   lua_setglobal(L,"__epoll");
}

void register_marshal_api(lua_State * L) {
   lua_pushcfunction(L,mar_encode);
   lua_setglobal(L,"__encode");
   lua_pushcfunction(L,mar_decode);
   lua_setglobal(L,"__decode");
   lua_pushcfunction(L,mar_clone);
   lua_setglobal(L,"__clone");
}

void register_sock_api(lua_State * L) {
   lua_newtable(L);
   lua_pushliteral(L,"wrap");
   lua_pushcfunction(L,leda_wrap_sock);
   lua_rawset(L,-3);
   lua_pushliteral(L,"unwrap");
   lua_pushcfunction(L,leda_unwrap_sock);
   lua_rawset(L,-3);
   lua_setglobal(L,"__socket");
 
}


void register_debug_api(lua_State * L) {
   lua_pushcfunction(L,leda_sleep);
   lua_setglobal(L,"__sleep");
   
   lua_pushcfunction(L,instance_wait_for_event);
   lua_setglobal(L,"__wait_event");
   lua_pushcfunction(L,instance_peek_for_event);
   lua_setglobal(L,"__peek_event");
   
}


void register_connector_api(lua_State * L) {
   lua_pushcfunction(L,emmit);
   lua_setglobal(L,"__emmit");

   lua_pushcfunction(L,cohort);
   lua_setglobal(L,"__cohort");
}

/* Try to aquire an instance. 
 * if there is no instances available then wait for a  
 * instance to be released
 *
 */
instance instance_wait(stage_id s) {
   instance ret=instance_aquire(s);
   if(ret==NULL) {
      POP(recycle_queues[s],ret);
      ret->init_time=now_secs();
      STATS_ACTIVE(ret->stage);
   }
   return ret; //released instance
}

/* Try to aquire an instance from its correspondent recycle queue
 * if there is no instance available in the recycle queue, create a 
 * a new instance, initialize-it with the stage method and returns it
 *
 */
instance instance_aquire(stage_id s) {
   if(s<0 && s>main_graph->n_s) return NULL; //error: invalid stage

   instance ret=NULL;

   //Try to recycle an instance from its correspondent queue   
   if(TRY_POP(recycle_queues[s],ret)) {
      ret->init_time=now_secs();
      STATS_ACTIVE(ret->stage);
      return ret; //instance recycled
   }
   
   //If not (beacuse the recycle queue was empty) verify if there
   //is space on the recycle queue for one more instance
   if(READ(number_of_instances[s])==recycle_queue_limits[s]) {
      _DEBUG("Instance: Error: No more instances allowed for stage '%s', (%ld == %d)\n",
      STAGE(s)->name,READ(number_of_instances[s]),recycle_queue_limits[s]);
      return NULL;//No more instances allowed for this stage
   }
   
   //Lock protected add
   ADD(number_of_instances[s],1);
   //create a new instance and returns it
   ret=calloc(1,sizeof(struct instance_data));
   ret->L=new_lua_state(TRUE);
   ret->stage=s;
   
   ret->instance_number=READ(number_of_instances[s]);
   
   lua_pushlightuserdata( ret->L, ret);
	lua_setfield( ret->L, LUA_REGISTRYINDEX, "__SELF" );
   if(luaL_loadbuffer( ret->L, instance_chunk, sizeof(instance_chunk), "@instance.lua"))
			return NULL; //"luaL_loadbuffer() failed";   // LUA_ERRMEM
   
   //Create a stage representation
   lua_newtable(ret->L); //Table holds all information

   //Push the init and handler chunks into the table
   lua_pushliteral(ret->L,"name"); //key
   lua_pushstring (ret->L, STAGE(s)->name); //value
   lua_settable(ret->L,-3); //Set __name

   lua_pushliteral(ret->L,"serial"); //key
   lua_pushboolean (ret->L, STAGE(s)->serial); //value
   lua_settable(ret->L,-3); //Set __init

   /*lua_pushliteral(ret->L,"__backpressure"); //key
   lua_pushboolean (ret->L, STAGE(s)->backpressure); //value
   lua_settable(ret->L,-3); //Set __init
   */
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
      lua_pushliteral(ret->L,"__consumer"); //key
      lua_pushinteger(ret->L,(int)CONNECTOR(c)->c);
     // printf("CONSUMER %d %d\n",c,(int)CONNECTOR(c)->c);
//      lua_newtable(ret->L); //value __output[key].__consumers
//      int k;
      /*for(k=1;k<=CONNECTOR(c)->n_c;k++) {
         lua_pushinteger(ret->L,(int)CONNECTOR(c)->c[k-1]); //consumer[k-1]
         lua_rawseti(ret->L,-2,k);
      }*/
      lua_settable(ret->L,-3); //Set __output[key].__consumer=value
      
      lua_settable(ret->L,-3); //Set __output[key]=value
   }
   lua_settable(ret->L,-3); //Set __output
   
   /* Push the kernel C API functions to the newly created lua_State */
   lua_setglobal(ret->L,"__stage");

   /* C methods for passing control to instances */
   register_connector_api(ret->L);
   
   /* Push the endcode for this instance, for now is only an integer*/
   lua_pushinteger(ret->L,ENDED);   
   lua_setglobal(ret->L,"__end_code");
   /* Push the endcode for this instance, for now is only an integer*/
   lua_pushinteger(ret->L,YIELDED);   
   lua_setglobal(ret->L,"__yield_code");

   /* load api with assorted functions useful for concurrency*/
   register_debug_api(ret->L);
   register_io_api(ret->L);
   register_sock_api(ret->L);
   register_mutex_api(ret->L);
   register_marshal_api(ret->L);
     
   /* Call the lua_chunk loaded in the luaL_loadbuffer */
//   dump_stack(ret->L);
   lua_call( ret->L, 0 , 0 );
   _DEBUG("Instance created for stage '%d' name='%s'\n",(int)s,STAGE(s)->name);

   ret->init_time=now_secs();
   STATS_ACTIVE(ret->stage);
   return ret; //created instance
}

int event_queue_size(stage_id s) {
   return queue_size(event_queues[s]);
}

int event_queue_capacity(stage_id s) {
   return queue_capacity(event_queues[s]);
}

/* Release a instance, if the queue is full (TRY_PUSH will fail),
 * destroy it.
 */
int instance_release(instance i) {
   lua_settop(i->L,0); //empty the instance's stack
   
   time_d t=now_secs()-i->init_time;

   STATS_UPDATE_TIME(i->stage,(int)(t*1000000));

   event e;
   if(TRY_POP(event_queues[i->stage],e)) { 
      //There are pending events waiting for this instance to finish
      
      lua_settop(i->L,0);
      
      //Get the  main coroutine of the instance's handler
      lua_getglobal(i->L, "handler");
      //push arguments to instance
      i->args=restore_event_to_lua_state(i->L,&e);
      
      i->init_time=now_secs();
      STATS_ACTIVE(i->stage);
      push_ready_queue(i);
      _DEBUG("Instance: Instance %d of stage '%s' popped a pending event.\n",
         i->instance_number,STAGE(i->stage)->name);
      return 0;
   }

   if(!TRY_PUSH(recycle_queues[i->stage],i)) {
      instance_destroy(i);
      return -1; //cannot push instance (it shouldn't get here)
   }
//   SIGNAL_ALL(&queue_used_cond);
   _DEBUG("Instance: Instance %d of stage '%s' released\n",i->instance_number,STAGE(i->stage)->name);
   return 0;// ok
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
         queue_free(event_queues[i]);
         atomic_free(number_of_instances[i]);
    }
   free(number_of_instances);
   free(recycle_queue_limits);
   free(event_queue_limits);
   free(recycle_queues);
   free(event_queues);
}
