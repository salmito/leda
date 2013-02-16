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
#include <stdio.h>

#include "atomic.h"
#include "thread.h"
#include "graph.h"
#include "instance.h"
#include "stats.h"

#include <event2/event.h>
extern struct event_base *kernel_event_base;

typedef struct __s_stats {
   atomic avg;
   atomic n;
   atomic executed;
   atomic events;
   atomic error;
   atomic active;
} * s_stats;

typedef struct __c_stats {
   atomic communication_time;
   atomic n;
   atomic events;
} * c_stats;

s_stats sstats;
c_stats cstats;
static int nstages;
static int nconnectors;

int stats_latency_reset(lua_State* L) {
   int i;
   for(i=0;i<nstages;i++) {
      STORE(sstats[i].avg,0);
      STORE(sstats[i].n,0);
   }
   for(i=0;i<nconnectors;i++) {
      STORE(cstats[i].communication_time,0);
      STORE(cstats[i].n,0);
   }
   return 0;
}

void stats_update_time(int id, long int t) {
   ADD(sstats[id].avg,t);
   ADD(sstats[id].n,1);
   if(has_release_cb) {
      struct kernel_event_t * arg=malloc(sizeof(struct kernel_event_t));
      arg->id=id;
      arg->time=t;
      struct event *listener_event = event_new(kernel_event_base, -1, 0, kernel_release_event, arg);
      event_add(listener_event, NULL);
      event_active(listener_event,0,0);
   }
}

void stats_update_events(int id, int e,int c_id, long int t) {
   ADD(sstats[id].events,e);
   ADD(cstats[c_id].communication_time,t);
   ADD(cstats[c_id].n,1);
   ADD(cstats[c_id].events,1);
   
   if(has_event_cb) {
      struct kernel_event_t * arg=malloc(sizeof(struct kernel_event_t));
      arg->id=id;
      arg->ev=READ(sstats[id].events);
      arg->c_id=c_id;
      arg->c_ev=READ(cstats[c_id].n);
      arg->time=t;
      struct event *listener_event = event_new(kernel_event_base, -1, 0, kernel_event_event, arg);
      event_add(listener_event, NULL);
      event_active(listener_event,0,0);
   }
}

void stats_update_error(int id, int e) {
   ADD(sstats[id].error,e);
   if(has_error_cb) {
      struct event *listener_event = event_new(kernel_event_base, -1, 0, kernel_error_event, (void *)(long unsigned int)id);
      event_add(listener_event, NULL);
      event_active(listener_event,0,0);
   }
}

void stats_active_instance(int id) {
   ADD(sstats[id].executed,1);
   ADD(sstats[id].active,1);
}

void stats_inactive_instance(int id) {
   SUB(sstats[id].active,1);
}


int stats_reset(lua_State * L) {
   int i;
   for(i=0;i<nstages;i++) {
      STORE(sstats[i].avg,0);
      STORE(sstats[i].n,0);
      STORE(sstats[i].executed,0);
      STORE(sstats[i].events,0);
      STORE(sstats[i].error,0);
   }
   for(i=0;i<nconnectors;i++) {
      STORE(cstats[i].communication_time,0);
      STORE(cstats[i].n,0);
      STORE(cstats[i].events,0);
   }
   return 0;
}

void stats_push(lua_State * L) {
   int i=0;
   int n_s=nstages;
   if(lua_type(L,1)==LUA_TNUMBER) {
      i=lua_tointeger(L,1)-1;
      n_s=lua_tointeger(L,1)-1;
   }
   lua_newtable(L);
   for(i=0;i<n_s;i++) {
      lua_newtable(L);
      lua_pushstring(L,STAGE(i)->name);
      lua_setfield(L,-2,"name");
      lua_pushnumber(L,READ(sstats[i].active));
      lua_setfield(L,-2,"active");
      lua_pushnumber(L,recycle_queue_capacity(i));
      lua_setfield(L,-2,"maxpar");
      lua_pushnumber(L,event_queue_size(i));
      lua_setfield(L,-2,"event_queue_size");
      lua_pushnumber(L,event_queue_capacity(i));
      lua_setfield(L,-2,"event_queue_capacity");
      long int nn=READ(sstats[i].n);
      if(nn<=0) nn=1;
      lua_pushnumber(L,((double)READ(sstats[i].avg))/(100000*nn));
      lua_setfield(L,-2,"average_latency");
      lua_pushnumber(L,READ(sstats[i].executed));
      lua_setfield(L,-2,"times_executed");
      lua_pushnumber(L,READ(sstats[i].events));
      lua_setfield(L,-2,"events_pushed");
      lua_pushnumber(L,READ(sstats[i].error));
      lua_setfield(L,-2,"errors");
      lua_rawseti(L,-2,i+1);
   }
   lua_newtable(L);
   if(lua_type(L,1)!=LUA_TNUMBER) {
      for(i=0;i<nconnectors;i++) {
         lua_newtable(L);
         lua_pushstring(L,CONNECTOR(i)->name);
         lua_setfield(L,-2,"key");
         lua_pushstring(L,STAGE(CONNECTOR(i)->p)->name);
         lua_setfield(L,-2,"producer");
         lua_pushstring(L,STAGE(CONNECTOR(i)->c)->name);
         lua_setfield(L,-2,"consumer");
         long int nn=READ(cstats[i].n);
         if(nn<=0) nn=1;
         lua_pushnumber(L,((double)READ(cstats[i].communication_time))/(1000000*nn));
         lua_setfield(L,-2,"average_latency");
         lua_pushnumber(L,READ(cstats[i].events));
         lua_setfield(L,-2,"events_pushed");
         lua_rawseti(L,-2,i+1);
      }
   }
}

void stats_init(int ns, int nc) {
   int i;
   nstages=ns;
   nconnectors=nc;
   sstats=calloc(ns,sizeof(struct __s_stats));
   for(i=0;i<ns;i++) {
      sstats[i].avg=atomic_new(0);
      sstats[i].n=atomic_new(0);
      sstats[i].active=atomic_new(0);
      sstats[i].events=atomic_new(0);
      sstats[i].executed=atomic_new(0);
      sstats[i].error=atomic_new(0);
   }
   cstats=calloc(nc,sizeof(struct __c_stats));
   for(i=0;i<nc;i++) {
      cstats[i].communication_time=atomic_new(0);
      cstats[i].n=atomic_new(0);
      cstats[i].events=atomic_new(0);
   }
}

void stats_free() {
   int i;
   for(i=0;i<nstages;i++) {
      atomic_free(sstats[i].avg);
      atomic_free(sstats[i].n);
      atomic_free(sstats[i].active);
      atomic_free(sstats[i].events);
      atomic_free(sstats[i].executed);
      atomic_free(sstats[i].error);
   }
   nstages=0;
   for(i=0;i<nconnectors;i++) {
      atomic_free(cstats[i].communication_time);
      atomic_free(cstats[i].n);
      atomic_free(cstats[i].events);
   }
   nconnectors=0;
   
   free(sstats);
   free(cstats);
}
