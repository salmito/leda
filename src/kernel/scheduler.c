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

#include <stdlib.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "scheduler.h"
#include "extra/lmarshal.h"
#include "queue.h"
#include "instance.h"
#include "event.h"
#include "stats.h"

MUTEX_T debug_lock;
atomic pool_size;
extern queue ready_queue;

/* Thread subsystem internal functions */
void emmit_cohort(instance caller);
void emmit_self(instance i);
void emmit_remote(instance i);

/* Returns the current size of the ready queue */
bool_t thread_ready_queue_isempty() {
   return queue_isempty(ready_queue);
}

/* Initialize thread subsystem */
void thread_init(size_t ready_queue_capacity) {
   ready_queue=queue_new();
   queue_set_capacity(ready_queue,ready_queue_capacity);
   pool_size=atomic_new(0);
}

#ifdef DEBUG
/* Dump a lua stack for debug purposes */
void dump_stack( lua_State* L ) {
   int top= lua_gettop(L);
      MUTEX_LOCK(&debug_lock);
    
   int i;
	fprintf( stderr, "\n\tDEBUG STACK:\n" );
	if (top==0)
		fprintf( stderr, "\t(none)\n" );
	for( i=1; i<=top; i++ ) {
		int type= lua_type( L, i );
		fprintf( stderr, "\t[%d]= (%s) ", i, lua_typename(L,type) );
      lua_getglobal( L, "tostring" );
      // [-1]: tostring function, or nil
      if (!lua_isfunction(L,-1)) {
         fprintf( stderr, "('tostring' not available)" );
      } else {
         lua_pushvalue( L, i );
         lua_call( L, 1, 1);
         fprintf( stderr, "%s", lua_tostring(L,-1) );
      }
      lua_pop(L,1);
		fprintf( stderr, "\n" );
   }
	fprintf( stderr, "\n" );
   MUTEX_UNLOCK(&debug_lock);
}
#endif

char const * get_return_status_name(int status) {
   switch(status) {
      case ENDED:
         return "ENDED";
      case EMMIT_COHORT:
         return "EMMIT_COHORT";
      case PCALL_ERROR:
         return "PCALL_ERROR";
      case EMMIT_REMOTE:
         return "EMMIT_REMOTE";
      case NICE:
         return "NICE";
      case WAIT_IO:
			return "WAIT_IO";
		case FILE_IO:
         return "FILE_IO";
		case SLEEP:
	      return "SLEEP";
      case DESTROY:
         return "DESTROY";
      default:
         return "UNKNOWN";
   }
   return "UNKNOWN";
}

   

/* Call an instance loaded with 'args' values at the top of its stack */
void thread_resume_instance(instance i) {
   _DEBUG("Thread: CALLING STAGE '%s' instance=%d args=%d\n",
         main_graph->s[i->stage]->name,i->instance_number,(int)i->args);
   
   if(i->packed) {
      _DEBUG("Thread: Unpacking event for stage '%s'\n",main_graph->s[i->stage]->name);
      GET_HANDLER(i->L);
      i->packed=0;
      i->args=restore_event_to_lua_state(i->L,&i->packed_event);
   }
//   dump_stack(i->L);
   //resume main instance coroutine
   int status=0;
   if(lua_pcall(i->L,i->args,LUA_MULTRET,0)) {
      const char * err=lua_tostring(i->L,-1);
      fprintf(stderr,"Error resuming instance: %s\n",err);
      status=PCALL_ERROR;
   } else {   
      if(lua_isnumber(i->L, 1)) status=lua_tointeger(i->L,1);
   }
   
   _DEBUG("Thread: Stage '%s' returned status code '%s'\n",main_graph->s[i->stage]->name,get_return_status_name(status));

   switch(status) {
      case ENDED: //stage finished, release instance
         _DEBUG("Thread: Stage '%s' finished top=%d agrs=%d stage=%d\n",
         main_graph->s[i->stage]->name,lua_gettop(i->L),(int)i->args,(int)i->stage);
         instance_release(i); //release the instance
         break;
         
      case PCALL_ERROR:
         STATS_UPDATE_ERROR(i->stage,1);
         STATS_INACTIVE(i->stage);
         instance_destroy(i);
         break;

      case DESTROY:
         STATS_INACTIVE(i->stage);
         instance_destroy(i);
         break;
         
      case EMMIT_COHORT:
         lua_remove(i->L,1);
         emmit_cohort(i);
         break;

      case EMMIT_REMOTE:
         lua_remove(i->L,1);
         emmit_remote(i);
         break;

      case WAIT_IO:
         lua_remove(i->L,1);
         event_wait_io(i);
         break;

	 case FILE_IO:
         lua_remove(i->L,1);
#ifndef SYNC_IO	 
         event_do_file_aio(i);
#endif
         break;

      case SLEEP:
         lua_remove(i->L,1);
         event_sleep(i);
         break;
         
      case NICE: 
         //handler coroutine yielded, put it again in the ready queue
         //It will be resumed with the value 'true' as return value
         emmit_self(i);
         break;
      default:
         fprintf(stderr,"Error, cannot resume main coroutine of stage '%s'\n",STAGE(i->stage)->name);
         instance_destroy(i);
   }
}

int leda_get_self_process(lua_State *L) {
   lua_getfield(L, LUA_REGISTRYINDEX, "__SELF" );
   instance self=lua_touserdata(L,-1);
   lua_pop(L,1);
   cluster c=CLUSTER(STAGE(self->stage)->cluster);
   process_id p=c->processes[0];
   lua_pushfstring(L,"%s:%d",PROCESS(p)->host,PROCESS(p)->port);
   return 1;
}

/* Put the current executing instance in the tail of the ready queue */
void emmit_self(instance i) {
   _DEBUG("Thread: Stage '%d' yielded itself top=%d\n",(int)i->stage,lua_gettop(i->L));
   //clear the stack (throw out yield arguments)
   lua_settop(i->L,0);
   //Get the  main coroutine of the instance's handler
   GET_HANDLER(i->L);
   //Put it on the bottom of the instance's stack
   lua_pushboolean(i->L,TRUE);
   //Set the previous number of arguments
   i->args=1;
   
   //Push it to the ready queue
   push_ready_queue(i);
}

int wait_io(lua_State * L) {
   //Push status code WAIT_IO to the bottom of the stack
   lua_pushinteger(L,WAIT_IO);
   lua_insert(L,1);
   int args=lua_gettop(L);
   //Yield current instance handler
   _DEBUG("Thread: Yielding to wait for a socket IO\n");
   return lua_yield(L,args);
}

int do_file_aio(lua_State * L) {
   lua_pushinteger(L,FILE_IO);
   lua_insert(L,1);
   int args=lua_gettop(L);
   //Yield current instance handler
   _DEBUG("Thread: Yielding to wait for a file IO\n");
   return lua_yield(L,args);
}

int leda_sleep(lua_State * L) {
   //Push status code SLEEP to the bottom of the stack
   lua_pushinteger(L,SLEEP);
   lua_insert(L,1);
   int args=lua_gettop(L);
   //Yield current instance handler
   _DEBUG("Thread: Yielding to sleep\n");
   return lua_yield(L,args);
}

int leda_destroy(lua_State * L) {
   //Push status code SLEEP to the bottom of the stack
   lua_pushinteger(L,DESTROY);
   lua_insert(L,1);
   int args=lua_gettop(L);
   //Yield current instance handler
   _DEBUG("Thread: Yielding to selfdestroy\n");
   return lua_yield(L,args);
}

int leda_quit(lua_State * L) {
   int sig=0;
   if(lua_type(L,1)==LUA_TNUMBER)
      sig=lua_tointeger(L,1);
   _DEBUG("Ending process");
   exit(sig);
}


/* Caller thread has yielded with code EMMIT_COHORT, therefore
 * pass direcly to the aquired instance and put the continuaiton
 * of the current instance on the ready queue 
 * (passind 'true' as resume value).
 *
 * Arguments:  instance 'caller'
 *                Instance of the continuation of the current stage (caller)
 *             Note: any other argument is poped and copyied to 
 *             the aquired instance 
 */
void emmit_cohort(instance caller) {
//   dump_stack(caller->L);
	_DEBUG("Thread: COHORTING through stages\n");
   time_d comunication_time=now_secs();
   stage_id dst_id=lua_tointeger(caller->L,1);
   connector_id con_id=lua_tointeger(caller->L,2);
   lua_remove(caller->L,2);
   int const args=lua_gettop(caller->L)-1;

   _DEBUG("Thread: Stage '%d' emmited itself and called stage '%d' top=%d\n",
      (int)caller->stage,(int)dst_id,lua_gettop(caller->L));

   //Get instance of the stage 'dst_id' (callee)
   instance callee=instance_aquire(dst_id);

   if(!callee) { //error getting an instance from the 
                 //recycle queue no more instances allowed for callee
         _DEBUG("Thread: ERROR: Cannot get an instance for the stage '%s'.\n",
         main_graph->s[dst_id]->name);
         lua_settop(caller->L,0);
         GET_HANDLER(caller->L);
         lua_pushnil(caller->L);
         lua_pushfstring(caller->L,"Cannot get a parallel instance of the stage '%s'.",
            STAGE(dst_id)->name);
         caller->args=2;
         return push_ready_queue(caller);
   }
   //if got the instance, pass the thread to it and emmit an event for self

   //Get the  main coroutine of the callee's handler
   GET_HANDLER(callee->L);
   //push arguments from caller to callee instance
   copy_values_directly(callee->L, caller->L, 2, args);
   callee->args=args;
   
   lua_settop(caller->L,0);
   //Get the  main coroutine of the caller's handler
   GET_HANDLER(caller->L);
   //push the resume value (TRUE) to the caller instance
   lua_pushboolean(caller->L,TRUE);
   //results
   caller->args=1;
   //Put the caller instance on the ready queue
   //dump_stack(caller->L);
   push_ready_queue(caller);
   //Pass the thread to the callee (call directly its instance)
   _DEBUG("Thread: Emmited self, passing the thread to the stage '%s'.\n",
      main_graph->s[dst_id]->name);
   if(con_id>=0) {
      time_d ct=now_secs()-comunication_time;
      STATS_UPDATE_EVENTS(caller->stage,1,con_id,ct*1000000);
   }
   thread_resume_instance(callee);
}

/* Emmit an event to a remote stage and continue the execution of the caller instance
 * Note: This will block the caller instance but not the thread.
 *
 * Arguments:  instance 'caller'
 *             Instance of the continuation of the current stage (caller)
 *             Note: any other argument is poped and copyied to 
 *             the aquired instance 
 */
void emmit_remote(instance caller) {
	_DEBUG("Thread: Emmit async remote event\n");
   time_d communication_time=now_secs(); //TODO
   stage_id dst_id=lua_tointeger(caller->L,1);
   int con_id=lua_tointeger(caller->L,2);
   lua_remove(caller->L,2);
   int const args=lua_gettop(caller->L)-1;
   int i;
   lua_pushcfunction(caller->L,mar_encode);
   lua_newtable(caller->L);
   for(i=1;i<=args;i++) {
      lua_pushvalue(caller->L,i+1);
      lua_rawseti(caller->L,-2,i);
   }
   lua_pushnil(caller->L);
   lua_pushboolean(caller->L,TRUE);
   lua_call(caller->L,3,1); //propagate error
   if(lua_type(caller->L,-1)!=LUA_TSTRING) {     
      lua_settop(caller->L,0);
      GET_HANDLER(caller->L);
      lua_pushboolean(caller->L,FALSE);
      lua_pushliteral(caller->L,"Error serializing event");
      caller->args=2;
      return push_ready_queue(caller);
   }
   size_t len; const char *payload=lua_tolstring(caller->L,-1,&len); 
   lua_pop(caller->L,1);    
   send_async_event(caller,dst_id,con_id,communication_time,len,payload);
}

/* Emmit an event to a stage and continue the execution of the caller instance 
 * without yelding control
 * Note: This will block the caller thread.
 *
 */
int emmit_remote_sync(lua_State * L) { //TODO send event synchronously
	_DEBUG("Thread: Emmit sync remote event\n");
   time_d comunication_time=now_secs();
   //stage_id dst_id=lua_tointeger(L,1);
   int con_id=lua_tointeger(L,2);
   lua_remove(L,2);
   int const args=lua_gettop(L)-1;
   int i;
   lua_pushcfunction(L,mar_encode);
   lua_newtable(L);
   for(i=1;i<=args;i++) {
      lua_pushvalue(L,i+1);
      lua_rawseti(L,-2,i);
   }
   lua_pushnil(L);
   lua_pushboolean(L,TRUE);
   lua_call(L,3,1); //propagate error
   
   if(lua_type(L,-1)!=LUA_TSTRING) {     
      luaL_error(L,"Error serializing event");
   }

   int resp=send_sync_event(L);

   if(con_id>=0) {
      time_d ct=now_secs()-comunication_time;
      STATS_UPDATE_EVENTS(CONNECTOR(con_id)->p,1,con_id,ct*1000000);
   }

   return resp;
}

/* Emmit an packed event to a local stage.
 * Note: This will not block the thread of caller instance.
 */
int emmit_packed_event(stage_id dst_id,char * data,size_t len) {
	_DEBUG("Thread: Emmit packed event\n");
   event e=event_new_packed_event(data,len);
   instance dst=instance_aquire(dst_id);

   if(!dst) { //Put on event queue
      if(!instance_try_push_pending_event(dst_id,e)) {
         return -1;
      }
      return 0;
   }
   dst->packed=1;
   dst->packed_event=e;
   push_ready_queue(dst);
   return 0;
}

/* Emmit an event to a stage and continue the execution of the caller instance
 * Note: This will not block the caller instance.
 *
 * Arguments:  int stage_id
 *                The internal id of the stage to be called
 *
 *             Note: any other argument is poped and copyied to 
 *             the aquired instance 
 */
int emmit(lua_State * L) {
   time_d comunication_time=now_secs();
   stage_id dst_id=lua_tointeger(L,1);
   int con_id=lua_tointeger(L,2);
//   lua_remove(L,2);
   _DEBUG("Thread: Event Emmit to local cluster? %d\n",CLUSTER(STAGE(dst_id)->cluster)->local);
   if(!CLUSTER(STAGE(dst_id)->cluster)->local) {
      return emmit_remote_sync(L);
   }
   lua_remove(L,2);
   int const args=lua_gettop(L)-1;
   
   _DEBUG("Thread: Emmiting event to stage '%s' top=%d\n",STAGE(dst_id)->name,lua_gettop(L));
   instance dst=instance_aquire(dst_id);
   _DEBUG("Thread: Result stage=%s instance='%p'\n",STAGE(dst_id)->name,dst);
   
   if(!dst) {   //error getting an instance from the 
               //recycle queue, emmit an event insted
      event e=extract_event_from_lua_state(L, 2, args);
      if(!instance_try_push_pending_event(dst_id,e)) {
         //error, event queue is full, push FALSE to sender
         lua_pushnil(L);
         lua_pushfstring(L,"Event queue for the stage '%s' is full.",
         STAGE(dst_id)->name);
         _DEBUG("Thread: Event queue for the stage '%s' is full\n",STAGE(dst_id)->name);
         return 2;
      }
      _DEBUG("Thread: Event emmited for stage '%s'\n",STAGE(dst_id)->name);
      lua_pushboolean(L,TRUE);
     if(con_id>=0) {
            time_d ct=now_secs()-comunication_time;
            STATS_UPDATE_EVENTS(CONNECTOR(con_id)->p,1,con_id,ct*1000000);
      }
      return 1;
   }

   _DEBUG("Thread: Emmit: got an idle instance (%d) of stage '%s'\n",dst->instance_number,STAGE(dst_id)->name);   
   //got an idle instance from recycle queue  
   //Get the  main coroutine of the instance's handler
   GET_HANDLER(dst->L);
   //push arguments to instance
   copy_values_directly(dst->L, L, 2, args);
   dst->args=args;
   push_ready_queue(dst);
   lua_pushboolean(L,TRUE);
   if(con_id>=0) {
      time_d ct=now_secs()-comunication_time;
      STATS_UPDATE_EVENTS(CONNECTOR(con_id)->p,1,con_id,ct*1000000);
   }
   return 1;
}


int cohort(lua_State * L) {
   //Push status code EMMIT_COHORT to the bottom of the stack
   lua_pushinteger(L,EMMIT_COHORT);
   lua_insert(L,1);
   int args=lua_gettop(L);
   //Yield current instance handler
   _DEBUG("Thread: Yielding to emmit a cohort event\n");
   return lua_yield(L,args);
}

#define wait_ready_queue(i) POP(ready_queue,i)

/*thread main loop*/
static THREAD_RETURN_T THREAD_CALLCONV thread_main(void *t_val) {
   instance i=NULL;
   thread t= (thread)t_val;
   t->status=WAITING;
   while(1) {
      wait_ready_queue(i); //get continuation from the stack
      if(i==NULL) { //Thread kill request
         break; //exit the main loop
      }
      t->status=RUNNING; //change status to RUNNING   
      thread_resume_instance(i); //call continuation
      t->status=WAITING; //change status to WAITING
   }
   ADD(pool_size,-1);
   _DEBUG("Thread: Thread killed (pool_size=%ld)\n",READ(pool_size));
   t->status=DONE;
   if(t->destroy)
      free(t);
   return 0;
}

/*Get a thread descriptor from the lua stack*/
thread thread_get (lua_State *L, int i) {
//  thread t = luaL_checkudata (L, i, THREAD_METATABLE);
//  luaL_argcheck (L, t != NULL, i, "not a Thread or killed already killed");
	return lua_touserdata(L,i);
}

/*tostring method*/
int thread_tostring (lua_State *L) {
  thread t = luaL_checkudata (L, 1, THREAD_METATABLE);
  lua_pushfstring (L, "Thread (%p)", t);
  return 1;
}

/* Force kill a thread from Lua, even if they're executing*/
int thread_rawkill (lua_State *L) {
   thread t=thread_get(L,1);
   if(t) {
      THREAD_KILL(&t->thread);
      ADD(pool_size,-1);
      t->destroy=1;
   }
   return 0;
}

#ifdef PLATFORM_LINUX
static int leda_thread_set_affinity(lua_State * L) {

	thread t = luaL_checkudata (L, 1, THREAD_METATABLE);
	int core_id=lua_tointeger(L,2)-1;

   cpu_set_t cpuset;
   CPU_ZERO(&cpuset);
   CPU_SET(core_id, &cpuset);

   lua_pushinteger(L,pthread_setaffinity_np(t->thread, sizeof(cpu_set_t), &cpuset));
   return 1;
}

#else
static int leda_thread_set_affinity(lua_State * L) {
 	return lua_error(L,"Not implemented");
}
#endif


/* Kill a thread from Lua*/
int thread_destroy (lua_State *L) {
   thread t=thread_get(L,1);
   if(t->status != DONE || t->status != CANCELLED) {
      lua_pushnil(L);
      lua_pushliteral(L,"Tried to destroy an ongoing Thread.");
      return 2;
   }
   if(t->destroy)
      free(t);
   lua_pushboolean(L,1);
   return 1;
}

/* create a new thread and execute it (and returns a descriptor to the
 * newly created thread)
 */
int thread_new (lua_State *L) {
   //Allocate space for the thread descriptor
   thread t=calloc(1,sizeof(struct thread_data));
   //Set its status to PENDING
   t->status=PENDING;
   t->destroy=0;
   //Put a reference to the pointer of the thread descriptor
   //on the stack as a light userdata
   lua_pushlightuserdata( L, t);


   //set thread metatable for the userdata
   luaL_getmetatable (L, THREAD_METATABLE);
   lua_setmetatable(L,-2);
   
   //Create a new thread and execute it with the 'thread_main' function
   THREAD_CREATE( &t->thread, thread_main, t, 0 );
   ADD(pool_size,1);
   _DEBUG("Thread: Thread created (pool_size=%ld)\n",READ(pool_size));
   //Return the reference pointer of the thread descriptor
   return 1;
}

/* Get the status of a thread from Lua*/
static int thread_status (lua_State *L) {
   thread t=thread_get(L,1);
   lua_pushinteger(L,t->status);
   return 1;
}

/* Kill a thread from Lua*/
int thread_kill (lua_State *L) {
   push_ready_queue(NULL);
   return 0;
}

/* Join with a thread from Lua*/
static int thread_join (lua_State *L) {
	thread t=thread_get(L,1);
   pthread_join(t->thread,NULL);
   return 0;
}

/* Deallocate the thread handle pointer */
int thread_gc (lua_State *L) {
   thread t=thread_get(L,1);
   if(t->status == DONE) {
      t->destroy=1;
      return thread_destroy(L);
   } else {
      t->destroy=1;
   }

   return 0;
}

/*create a unique thread metatable*/
int thread_createmetatable (lua_State *L) {
	/* Create thread metatable */
	if (!luaL_newmetatable (L, THREAD_METATABLE)) {
		return 0;
	}
	/* load methods onto the newly created metatable */
	lua_pushliteral(L,"status");
   lua_pushcfunction(L,thread_status);
   lua_rawset(L,-3);
   
  	lua_pushliteral(L,"kill");
   lua_pushcfunction(L,thread_kill);
   lua_rawset(L,-3);

  	lua_pushliteral(L,"join");
   lua_pushcfunction(L,thread_join);
   lua_rawset(L,-3);

	lua_pushliteral(L,"set_affinity");
	lua_pushcfunction(L,leda_thread_set_affinity);
   lua_rawset(L,-3);


	/* define metamethods */
	lua_pushliteral (L, "__index");
	lua_pushvalue (L, -2);
	lua_settable (L, -3);

	lua_pushliteral (L, "__tostring");
	lua_pushcfunction (L, thread_tostring);
	lua_settable (L, -3);

	lua_pushliteral (L, "__metatable");
	lua_pushliteral (L, "You're not allowed to get the metatable of a Thread");
	lua_settable (L, -3);
	lua_pop(L,1); //pop metatable
	
	return 0;
}
