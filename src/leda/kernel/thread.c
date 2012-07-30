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

#include "thread.h"
#include "queue.h"
#include "instance.h"
#include "event.h"

MUTEX_T debug_lock;
atomic pool_size;
extern queue ready_queue;
extern queue * event_queues;

/* Thread subsystem internal functions */
void emmit_self_and_pass_the_thread(instance caller);
void emmit_and_continue(instance caller);
void emmit_self(instance i);
void emmit_pending_thread(stage_id id, instance caller);

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
      case EMMIT_CONTINUATION_AND_PASS_THREAD:
         return "EMMIT_CONTINUATION_AND_PASS_THREAD";
//      case EMMIT_AND_CONTINUE:
//         return "EMMIT_AND_CONTINUE";
      case YIELDED:
      default:
         return "YIELDED";
   }
   return "UNKNOWN";
}

/* Call an instance loaded with 'args' values at the top of its stack */
void thread_call(instance i) {
   _DEBUG("Thread: CALLING STAGE '%s' in=%d top=%d args=%d ready_queue_size=%d event_queue='%d'\n",
         main_graph->s[i->stage]->name,i->instance_number,lua_gettop(i->L),(int)i->args,
         (int)queue_size(ready_queue),
         (int)queue_size(event_queues[i->stage]));
   
   //dump_stack(i->L);
   
   //resume main instance coroutine
   lua_call(i->L,i->args,LUA_MULTRET);
   
    
   
   int status=0;
   
   if(lua_isnumber(i->L, 1)) status=lua_tointeger(i->L,1);
   
   _DEBUG("Thread: Stage '%s' returned status code '%s'\n",main_graph->s[i->stage]->name,get_return_status_name(status));
//   dump_stack(i->L);
   
   switch(status) {
      case ENDED: //stage finished, release instance
         _DEBUG("thread: Stage '%s' finished top=%d agrs=%d stage=%d\n",
         main_graph->s[i->stage]->name,lua_gettop(i->L),(int)i->args,(int)i->stage);
         instance_release(i); //release the instance
         break;
         
      case EMMIT_CONTINUATION_AND_PASS_THREAD: //stage called emmit_call_self
         lua_remove(i->L,1); //pop the status from the stack
         emmit_self_and_pass_the_thread(i);
         break;
         
      case YIELDED: 
      default:  
         //handler coroutine yielded, put it again in the ready queue
         //It will be resumed with the value 'true' as return value
         emmit_self(i);
   }
 

}

/* Put the current executing instance in the tail of the ready queue */
void emmit_self(instance i) {
   _DEBUG("Thread: Stage '%d' yielded itself top=%d\n",(int)i->stage,lua_gettop(i->L));
   //clear the stack (throw out yield arguments)
   lua_settop(i->L,0);
   //Get the  main coroutine of the instance's handler
   lua_getglobal(i->L, "handler");
   //Put it on the bottom of the instance's stack
   lua_pushboolean(i->L,TRUE);
   //Set the previous number of arguments
   i->args=1;
   
   //Push it to the ready queue
   push_ready_queue(i);
}

/* Caller thread has yielded with code EMMIT_SELF, therefore
 * pass direcly to the aquired instance and put the continuaiton
 * of the current instance on the ready queue 
 * (passind 'true' as resume value).
 *
 * Arguments:  instance 'caller'
 *                Instance of the continuation of the current stage (caller)
 *             Note: any other argument is poped and copyied to 
 *             the aquired instance 
 */
void emmit_self_and_pass_the_thread(instance caller) {
//   dump_stack(caller->L);
   stage_id dst_id=lua_tointeger(caller->L,1);
   int const args=lua_gettop(caller->L)-1;

   _DEBUG("Thread: Stage '%d' emmited itself and called stage '%d' top=%d\n",
      (int)caller->stage,(int)dst_id,lua_gettop(caller->L));

   //Get instance of the stage 'dst_id' (callee)
   instance callee=instance_aquire(dst_id);
   
   if(!callee) { //error getting an instance from the 
                 //recycle queue, try to emmit an event insted
      event e=extract_event_from_lua_state(caller->L, 2, args);
      if(!instance_try_push_pending_event(caller,dst_id,e)) {
        _DEBUG("Thread: ERROR: Event queue for the stage '%s' is full.\n",
            main_graph->s[dst_id]->name);
         lua_settop(caller->L,0);
         lua_getglobal(caller->L, "handler");
         lua_pushnil(caller->L);
         lua_pushfstring(caller->L,"Event queue for the stage '%s' is full.",
            main_graph->s[dst_id]->name);
         caller->args=2;
         return push_ready_queue(caller);
      }
      //true to caller
      lua_settop(caller->L,0);
      lua_getglobal(caller->L, "handler");
      lua_pushboolean(caller->L,TRUE);
      caller->args=1;
      _DEBUG("Thread: Warning: Cannot call stage directly, emmit event for stage '%s' insted.\n",
      main_graph->s[dst_id]->name);
      return push_ready_queue(caller);
   }
   
   //if got the instance, pass the thread to it and emmit an event for self

   //Get the  main coroutine of the callee's handler
   lua_getglobal(callee->L, "handler");
   //push arguments from caller to callee instance
   copy_values_directly(callee->L, caller->L, 2, args);
   callee->args=args;
   
   lua_settop(caller->L,0);
   //Get the  main coroutine of the caller's handler
   lua_getglobal(caller->L, "handler");
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
   thread_call(callee);
}

/* Pass thread direcly to another instance
 * Note: This will block the sender until the 
 * instance finishes its execution.
 *
 * Arguments:  int stage_id
 *                The internal id of the stage to be called
 *
 *             Note: any other argument is poped and copyied to 
 *             the aquired instance 
 */
int call(lua_State * L) {
   _DEBUG("Thread: Calling top=%d\n",lua_gettop(L));
   stage_id dst_id=lua_tointeger(L,1);
   int top=lua_gettop(L);

   instance callee=instance_aquire(dst_id);

   if(!callee) { //error getting an instance from the 
             //recycle queue, try to emmit an event insted
             //(fallback to event-driven)
      lua_getfield(L, LUA_REGISTRYINDEX, "__SELF" );
      instance caller=lua_touserdata(L,-1);
      lua_pop(L,1);
      
      event e=extract_event_from_lua_state(L, 2, top-1);
      if(!instance_try_push_pending_event(caller,dst_id,e)) {
      
         lua_pushnil(caller->L);
         lua_pushfstring(caller->L,"Event queue for the stage '%s' is full.",
            main_graph->s[dst_id]->name);
         return 2;
      }
      //true to caller
      lua_pushboolean(caller->L,TRUE);
      return 1;
   }

   //Get the  main coroutine of the instance's handler
   lua_getglobal(callee->L, "handler");
   //push arguments to instance
   copy_values_directly(callee->L, L, 2, top-1);
   
   callee->args=top-1;
   
   //Resume coroutine
   thread_call(callee);
   
   //Returns 'true' to caller
   lua_pushboolean(L,TRUE);
   return 1;
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
   //(instance src,stage_id dst_id, int from, int args)
   stage_id dst_id=lua_tointeger(L,1);
   int const args=lua_gettop(L)-1;

   _DEBUG("Thread: Emmiting event to stage '%s' top=%d\n",STAGE(dst_id)->name,lua_gettop(L));
   instance dst=instance_aquire(dst_id);
   _DEBUG("Thread: Result stage=%s instance='%p'\n",STAGE(dst_id)->name,dst);
   
   if(!dst) {   //error getting an instance from the 
               //recycle queue, try to emmit an event insted
      lua_getfield( L, LUA_REGISTRYINDEX, "__SELF" );
      instance caller=lua_touserdata(L,-1);
      lua_pop(L,1);
      
      event e=extract_event_from_lua_state(L, 2, args);
      if(!instance_try_push_pending_event(caller,dst_id,e)) {
         //error, event queue is full, push FALSE to sender
         lua_pushnil(L);
         lua_pushfstring(L,"Event queue for the stage '%s' is full.",
         STAGE(dst_id)->name);
         _DEBUG("Thread: Event queue for the stage '%s' is full\n",STAGE(dst_id)->name);
         return 2;
      }
      _DEBUG("Thread: Event emmited for stage '%s'\n",STAGE(dst_id)->name);
      lua_pushboolean(L,TRUE);
      return 1;
   }

   _DEBUG("Thread: Emmit: got an idle instance (%d) of stage '%s'\n",dst->instance_number,STAGE(dst_id)->name);   
   //got an idle instance from recycle queue  
   //Get the  main coroutine of the instance's handler
   lua_getglobal(dst->L, "handler");
   //push arguments to instance
   copy_values_directly(dst->L, L, 2, args);
   
   dst->args=args;
   push_ready_queue(dst);

   lua_pushboolean(L,TRUE);
   return 1;
}

/* Yield itself, and tell to the caller thread to pass direcly to the 
 * aquired instance and put the continuaiton of the current instance
 * on the ready queue (passind 'true' as resume value).
 * 
 * Note: This will yield one event for the ready queue
 * with the continuaiton of the current instance.
 *
 * Arguments:  int stage_id
 *                The internal id of the stage to be called
 *
 *             Note: any other argument is poped and copyied to 
 *             the aquired instance 
 */
int emmit_self_call(lua_State * L) {
   //Push status code EMMIT_SELF_AND_PASS_THREAD to the bottom of the stack
   lua_pushinteger(L,EMMIT_CONTINUATION_AND_PASS_THREAD);
   lua_insert(L,1);
   //Get the number of yielded values (arguments to pass 
   //to the next stage)
   int args=lua_gettop(L);
   //Yield current instance handler with 
   //code EMMIT_CONTINUATION_AND_PASS_THREAD (+ args)
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
      thread_call(i); //call continuation
      t->status=WAITING; //change status to WAITING
   }
   ADD(pool_size,-1);
   _DEBUG("Thread: Thread killed (pool_size=%d)\n",READ(pool_size));
   t->status=DONE;
   return NULL;
}

/*Get a thread descriptor from the lua stack*/
thread thread_get (lua_State *L, int i) {
  thread t = luaL_checkudata (L, i, THREAD_METATABLE);
  luaL_argcheck (L, t != NULL, i, "not a Thread or killed already killed");
  return t;
}

/*tostring method*/
int thread_tostring (lua_State *L) {
  thread t = luaL_checkudata (L, 1, THREAD_METATABLE);
  lua_pushfstring (L, "Thread (%p)", t);
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
   //Put a reference to the pointer of the thread descriptor
   //on the stack as a light userdata
   lua_pushlightuserdata( L, t);


   //set thread metatable for the userdata
   luaL_getmetatable (L, THREAD_METATABLE);
   lua_setmetatable(L,-2);
   
   //Create a new thread and execute it with the 'thread_main' function
   THREAD_CREATE( &t->thread, thread_main, t, 0 );
   ADD(pool_size,1);
   _DEBUG("Thread: Thread created (pool_size=%d)\n",READ(pool_size));
   //Return the reference pointer of the thread descriptor
   return 1;
}

/* Get the status of a thread from Lua*/
int thread_status (lua_State *L) {
   thread t=thread_get(L,1);
   lua_pushinteger(L,t->status);
   return 1;
}

/* Kill a thread from Lua*/
int thread_kill (lua_State *L) {
   push_ready_queue(NULL);
   return 0;
}

/* Kill a thread from Lua*/
int thread_destroy (lua_State *L) {
   thread t=thread_get(L,1);
   if(t->status != DONE) {
      luaL_error(L,"Tried to destroy an ongoing Thread.");
   }
   free(t);
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
	
	return 1;
}
