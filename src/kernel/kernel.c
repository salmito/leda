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
#include <errno.h>

#include "thread.h"
#include "graph.h"
#include "queue.h"
#include "event.h"
#include "instance.h"
#include "stats.h"
#include "extra/lmarshal.h"
#include "extra/leda-io.h"

#include <event2/event.h>

#define __VERSION "0.2.9-unstable"

#define CONNECTOR_TIMEOUT 2.0

#define LEDA_NAME          "leda.kernel"
#define LEDA_ENV           "leda kernel environment"

#ifdef _WIN32
#define sleep(a) Sleep(a * 1000)
#endif

struct event_base *kernel_event_base;
char has_error_cb=0,
     has_event_cb=0,
     has_release_cb=0,
     has_destroy_cb=0,
     has_create_cb=0,
     has_active_cb=0;
/* initialized flag, 'true' if already initialized */
bool_t initialized=FALSE;

SIGNAL_T queue_used_cond;
MUTEX_T queue_used_lock;
extern queue ready_queue;
extern queue * event_queues;
extern queue * recycle_queues;

int leda_send(lua_State *L) {
   int i,id=lua_tointeger(L,1),args=lua_gettop(L)-1;
   if(id<0 || id>main_graph->n_s) {
      lua_pushnil(L);
      lua_pushliteral(L,"Invalid stage id");
      return 2;
   }
   lua_pushcfunction(L,emmit);
   lua_pushinteger(L,id);
   lua_pushinteger(L,-1); //connector id not known
   for(i=1;i<=args;i++)
      lua_pushvalue(L,i+1);
   lua_call(L,args+2,2);
   return 2;
}

/**
 * Event system callbacks using libvevent
 */
 lua_State * L_main;
 
 #include <event2/thread.h>
 
void kernel_error_event(evutil_socket_t fd, short events, void *arg) {
   lua_getfield(L_main, 3,"on_error");
   if(lua_type(L_main,-1)==LUA_TFUNCTION) {
      lua_pushinteger(L_main,((unsigned long int)arg)+1);
      lua_call(L_main,1,0);
   } else {
      _DEBUG("Lost on_error callback\n");
      lua_pop(L_main,1);
   }
 }
 
 void kernel_event_event(evutil_socket_t fd, short events, void *arg) {
   lua_getfield(L_main, 3,"on_event");
   if(lua_type(L_main,-1)==LUA_TFUNCTION) {
      struct kernel_event_t * ev=arg;
      lua_pushinteger(L_main,ev->id+1);
      lua_pushinteger(L_main,ev->ev);
      lua_pushinteger(L_main,ev->c_id+1);
      lua_pushinteger(L_main,ev->c_ev);
      lua_pushnumber(L_main,ev->time);
      free(ev);
      lua_call(L_main,5,0);
   } else {
      _DEBUG("Lost on_event callback\n");
      lua_pop(L_main,1);
   }
 }

void kernel_release_event(evutil_socket_t fd, short events, void *arg){
  lua_getfield(L_main, 3,"on_release");
   if(lua_type(L_main,-1)==LUA_TFUNCTION) {
      struct kernel_event_t * ev=arg;
      lua_pushinteger(L_main,ev->id+1);
      lua_pushnumber(L_main,ev->time);
      free(ev);
      lua_call(L_main,2,0);
   } else {
      _DEBUG("Lost on_release callback\n");
      lua_pop(L_main,1);
   }
}

//void kernel_destroy_event(evutil_socket_t fd, short events, void *arg);
//void kernel_create_event(evutil_socket_t fd, short events, void *arg);
//void kernel_active_event(evutil_socket_t fd, short events, void *arg);

 void kernel_null_event(evutil_socket_t fd, short events, void *arg) {
//    printf("NULL OCCURED\n");
 }

void kernel_timer_event(evutil_socket_t fd, short events, void *arg) {
   lua_getfield(L_main, 3,"on_timer");
   //printf("on_timer\n");
   if(lua_type(L_main,-1)==LUA_TFUNCTION) {
      lua_pushinteger(L_main,((long int)arg));
      lua_call(L_main,1,0);
   } else {
      _DEBUG("Lost on_timer callback\n");
      lua_pop(L_main,1);
   }
 }

int add_timer(lua_State * L) {
   double t;
   int n;
   t = lua_tonumber(L,1);
   n = lua_tointeger(L,2);
   
   struct event *listener_event = event_new(kernel_event_base, -1, EV_READ|EV_PERSIST, kernel_timer_event, (void *)(long int)n);
   struct timeval to={t,(t-((int) t))*1000000L};
   event_add(listener_event, &to);
   return 0;
}

/*
* Run a graph defined by the lua declaration
*
* Returns: 'true' in case of sucess
*          'nil' in case of error, with an error message
*/
int leda_run(lua_State * L) {
   evthread_use_pthreads();
   kernel_event_base = event_base_new();
   if (!kernel_event_base) {
   	luaL_error(L,"Error opening a new event_base for the kernel"); //error
   }
   
   //Create graph internal representation
   graph g=to_graph(L,2);
   
   main_graph=g;
   //parameter must be a table for controller
   luaL_checktype(L,3, LUA_TTABLE);
   int default_maxpar=lua_tointeger(L,4);
   //parameter must be a socket descriptor for the process   
   int process_fd=lua_tointeger(L,5);
   //initiate instances for the graph
   //queues are initially unbounded (-1 limit)
   instance_init(default_maxpar,-1);
   #ifndef STATS_OFF
      stats_init(g->n_s,g->n_c);
   #endif
   event_init_t(process_fd);
   
   //first, iterate through the stages
   //to push pending sends.
   //Each send will emmit a event to the corresponding stage
   lua_getfield(L,1,"stages");
   luaL_checktype(L,-1,LUA_TFUNCTION);
   lua_pushvalue(L,1);
   lua_call(L,1,1);
   luaL_checktype(L,-1,LUA_TTABLE);
   lua_pushnil(L);
   while (lua_next(L, -2) != 0) {
      lua_pop(L,1);
      lua_getfield(L,1,"stagesid");
      lua_pushvalue(L,-2);
      lua_gettable(L,-2);
      stage_id id=lua_tointeger(L,-1);
      lua_pop(L,2);
      lua_getfield(L,-1,"pending");
      if(lua_type(L,-1)!=LUA_TTABLE) continue;
      int i,n=lua_objlen(L,-1);
      for(i=1;i<=n;i++) {
         lua_pushcfunction(L,emmit);
         lua_pushinteger(L,id);
         lua_pushinteger(L,-1); //Connector id not known
         int begin=lua_gettop(L);
         lua_getglobal(L,"unpack"); //Push unpack function
         luaL_checktype(L,-1,LUA_TFUNCTION);
         lua_rawgeti(L,-5,i);
         lua_call(L,1,LUA_MULTRET); //Unpack the pending arguments
         int args=lua_gettop(L)-begin;
         _DEBUG("Kernel: Emmiting pending event\n");
         lua_call(L,args+2,2); //Call the emmit function
         _DEBUG("Kernel: Emmited pending event\n");
         bool_t ok=lua_toboolean(L,-2); 
       
         if(!ok) {
            char const * str=lua_tostring(L,-1);
            luaL_error(L,"Error emmiting event for stage '%s': %s\n",g->s[id]->name,str);
         }
         lua_pop(L,2);
      }
      lua_pop(L,1);
      lua_newtable(L);
      lua_setfield(L,-1,"pending");
   }
   
   lua_getfield(L, 3,"on_error");
   if(lua_type(L,-1)==LUA_TFUNCTION) {
      _DEBUG("Kernel: Controller has an error callback\n");
      has_error_cb=1;
   }
   lua_pop(L,1);

   lua_getfield(L, 3,"on_event");
   if(lua_type(L,-1)==LUA_TFUNCTION) {
      _DEBUG("Kernel: Controller has an event callback\n");
      has_event_cb=1;
   }
   lua_pop(L,1);

   lua_getfield(L, 3,"on_release");
   if(lua_type(L,-1)==LUA_TFUNCTION) {
      _DEBUG("Kernel: Controller has an release callback\n");
      has_release_cb=1;
   }
   lua_pop(L,1);

   lua_getfield(L, 3,"on_destroy");
   if(lua_type(L,-1)==LUA_TFUNCTION) {
      _DEBUG("Kernel: Controller has an destroy callback\n");
      has_destroy_cb=1;
   }
   lua_pop(L,1);

   lua_getfield(L, 3,"on_create");
   if(lua_type(L,-1)==LUA_TFUNCTION) {
      _DEBUG("Kernel: Controller has an create callback\n");
      has_create_cb=1;
   }
   lua_pop(L,1);

   lua_getfield(L, 3,"on_active");
   if(lua_type(L,-1)==LUA_TFUNCTION) {
      _DEBUG("Kernel: Controller has an active callback\n");
      has_active_cb=1;
   }
   lua_pop(L,1);


   //call the init function of a controller, if defined
   lua_getfield(L, 3,"init");
  if(lua_type(L,-1)==LUA_TFUNCTION) {
      lua_pushvalue(L,1);
      lua_pushvalue(L,2);
      _DEBUG("Kernel: Calling controller init function\n");
      lua_call(L,2,0);
   } else {
      lua_pop(L,1);
      luaL_error(L,"Controller does not defined an init method");
   }
    _DEBUG("Kernel: Running Graph '%s'\n",g->name);

   #ifndef STATS_OFF

   L_main=L;
   
   struct event *listener_event = event_new(kernel_event_base, -1, EV_READ|EV_PERSIST, kernel_null_event, NULL);
   event_add(listener_event, NULL);
    
   event_base_dispatch(kernel_event_base);
   #else
      while(1) sleep(1000);
   #endif
   return 0;
}

/* Kernel Lua function to get the size of the ready queue*/
int leda_ready_queue_size(lua_State * L) {
   lua_pushinteger(L,queue_size(ready_queue));
   return 1;
}

int leda_ready_queue_capacity(lua_State * L) {
   lua_pushinteger(L,queue_capacity(ready_queue));
   return 1;
}

int leda_ready_queue_isempty(lua_State * L) {
   lua_pushboolean(L,queue_isempty(ready_queue));
   return 1;
}

int leda_getmetatable(lua_State *L) {
   if(lua_type(L,1)==LUA_TSTRING) {
      const char *tname=lua_tostring(L,1);
      luaL_getmetatable(L,tname);
   } else {
      if(!lua_getmetatable (L,1)) {
         lua_pushnil(L);
         lua_pushliteral(L,"Metatable not found for the provided value");
         return 2;
      }
   }
   return 1;
}

int leda_setmetatable(lua_State *L) {
   lua_pushvalue(L,2);
   lua_setmetatable(L,1);
   lua_pushvalue(L,1);
   return 1;
}


/* Works on linux and windows
 * adapted from:
 * http://stackoverflow.com/questions/4586405/get-number-of-cpus-in-linux-using-c
 */
int leda_number_of_cpus(lua_State *L) {
   long nprocs = -1;
   long nprocs_max = -1;
   #ifdef _WIN32
      #ifndef _SC_NPROCESSORS_ONLN
         SYSTEM_INFO info;
         GetSystemInfo(&info);
         #define sysconf(a) info.dwNumberOfProcessors
         #define _SC_NPROCESSORS_ONLN
      #endif
   #endif
   #ifdef _SC_NPROCESSORS_ONLN
      nprocs = sysconf(_SC_NPROCESSORS_ONLN);
      if (nprocs < 1) {
         luaL_error(L,"Could not determine number of CPUs online:\n%s\n", 
            strerror (errno));
      }
      nprocs_max = sysconf(_SC_NPROCESSORS_CONF);
      if (nprocs_max < 1) {
         luaL_error(L, "Could not determine number of CPUs configured:\n%s\n", 
            strerror (errno));
      }
      lua_pushnumber(L,nprocs);
      lua_pushnumber(L,nprocs_max);
      return 2;
   #else
      luaL_error(L, "Could not determine number of CPUs");
      exit (EXIT_FAILURE);
   #endif
}
/* Kernel Lua function to sleep for a time in miliseconds*/
int leda_sleep_(lua_State * L) {
   lua_Number n=lua_tonumber(L,1);
   usleep((useconds_t)(n*1000000.0));
   return 0;
}

/* Kernel Lua function to get the pointer of a lua value as a string
 * (used as a unique name for objects)
 */
int leda_to_pointer(lua_State * L) {
   lua_pushfstring(L,"%p",lua_topointer(L,1));
   return 1;
}

int leda_get_stats(lua_State * L) {
   STATS_PUSH(L);
   return 2;
}

/* Get the size of the thread pool */
int leda_get_thread_pool_size(lua_State * L) {
   lua_pushinteger(L,READ(pool_size));
   return 1;
}

int leda_set_capacity(lua_State * L) {
   queue q=NULL;
   int i=lua_tointeger(L,1);
   if(i<0) {
      q=ready_queue;
   } else if(i>=main_graph->n_s) {
         lua_pushnil(L);
         lua_pushliteral(L,"Invalid stage id");
         return 2;
   } else {
      q=event_queues[i];
   }
   if(!q) {
      lua_pushnil(L);
      lua_pushliteral(L,"Queue error");
      return 2;
   }
   int cap=lua_tointeger(L,2);
   queue_set_capacity(q,cap);
   lua_pushboolean(L,1);
   return 1;
}

/* Leda's kernel info  */
static void set_leda_info (lua_State *L) {
	lua_pushliteral (L, "_DESCRIPTION");
	lua_pushliteral (L, "Leda");
	lua_settable (L, -3);
	lua_pushliteral (L, "_VERSION");
	lua_pushliteral (L, __VERSION);
	lua_settable (L, -3);
}

/* Load the Leda's kernel C API into a lua_State
 */
int luaopen_leda_kernel (lua_State *L) {
	/* Leda's kernel library functions */
	struct luaL_Reg leda_funcs[] = {
	   {"run", leda_run},
  	   {"to_pointer", leda_to_pointer},
  	   {"encode", mar_encode},
  	   {"decode", mar_decode},
  	   {"clone", mar_clone},
  	   {"gettime", leda_gettime},
 	   {"send", leda_send},
  	   {"build_graph", graph_build},
  	   {"getmetatable", leda_getmetatable},  	   
 	   {"setmetatable", leda_setmetatable}, 
      {"cpu", leda_number_of_cpus},
  	   //functions for controllers
  	   {"add_timer", add_timer},
  	   {"thread_new", thread_new},
  	   {"thread_kill", thread_kill},
  	   {"thread_rawkill", thread_rawkill},
  	   {"stats", leda_get_stats},
  	   {"reset_stats", stats_reset},
  	   {"stats_latency_reset", stats_latency_reset},
  	   {"maxpar", instance_set_maxpar},
 	   {"ready_queue_size", leda_ready_queue_size},
 	   {"sleep",leda_sleep_},
  	   {"set_capacity", leda_set_capacity},
 	   {"ready_queue_capacity", leda_ready_queue_capacity},
  	   {"thread_pool_size", leda_get_thread_pool_size},
		{NULL, NULL},
	};
	
   /* Initialize some debug related variables and the thread subsystem */
   if(initialized) {
   	lua_getglobal(L,"leda");
   	return 1;
   }
   initialized=TRUE;
   #ifdef DEBUG
//   MUTEX_INIT(&debug_lock);
   #endif
   thread_init(-1);
	
	/* Load main library functions */
   _DEBUG("Kernel: Loading leda main API\n");
	dump_stack(L);	
	REGISTER_LEDA(L, LEDA_NAME, leda_funcs);
	dump_stack(L);
	set_leda_info (L);
	dump_stack(L);
 	/* Create the thread metatable */
   thread_createmetatable(L);
 	dump_stack(L);
 	/* Create the graph metatable */
   graph_createmetatable(L);
	dump_stack(L);
   _DEBUG("Kernel: Leda's kernel loaded successfully.\n");
 	return 1;
}
