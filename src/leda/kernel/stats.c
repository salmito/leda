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

typedef struct __stats {
   atomic avg;
   atomic n;
   atomic events;
   atomic active;
} * s_stats;

s_stats stats;
static int nstages;

void stats_update_time(int id, int t) {
   ADD(stats[id].avg,(t-READ(stats[id].avg))/(ADD(stats[id].n,1)+1));
   SUB(stats[id].active,1);
}

void stats_update_events(int id, int e) {
   ADD(stats[id].events,e);
}

void stats_active_instance(int id) {
   ADD(stats[id].active,1);
}

void stats_reset(int id) {
   if(id>=0) {
      STORE(stats[id].avg,0);
      STORE(stats[id].n,0);
      STORE(stats[id].events,0);
   } else {
      int i;
      for(i=0;i<nstages;i++) {
         STORE(stats[i].avg,0);
         STORE(stats[i].n,0);
         STORE(stats[i].events,0);
      }
   }
}

void stats_push(lua_State * L) {
   int i;
   lua_newtable(L);
   for(i=0;i<nstages;i++) {
      lua_newtable(L);
      lua_pushstring(L,STAGE(i)->name);
      lua_setfield(L,-2,"name");
      lua_pushnumber(L,READ(stats[i].active));
      lua_setfield(L,-2,"active");
      lua_pushnumber(L,event_queue_size(i));
      lua_setfield(L,-2,"event_queue_size");
      lua_pushnumber(L,event_queue_capacity(i));
      lua_setfield(L,-2,"event_queue_capacity");
      lua_pushnumber(L,READ(stats[i].avg));
      lua_setfield(L,-2,"average_latency");
      lua_pushnumber(L,READ(stats[i].n));
      lua_setfield(L,-2,"times_executed");
      lua_pushnumber(L,READ(stats[i].events));
      lua_setfield(L,-2,"events_pushed");
      lua_rawseti(L,-2,i+1);
   }
}

void stats_init(int n) {
   int i;
   nstages=n;
   stats=malloc(sizeof(struct __stats)*n);
   for(i=0;i<n;i++) {
      stats[i].avg=atomic_new(0);
      stats[i].n=atomic_new(0);
      stats[i].active=atomic_new(0);
      stats[i].events=atomic_new(0);
   }
}

void stats_free() {
   int i;
   for(i=0;i<nstages;i++) {
      atomic_free(stats[i].avg);
      atomic_free(stats[i].n);
      atomic_free(stats[i].active);
      atomic_free(stats[i].events);
   }
   nstages=0;
   free(stats);
}
