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
#ifndef _INSTANCE_H_
#define _INSTANCE_H_

#include "extra/threading.h"
#include "graph.h"
#include "event.h"

/* lua state structure */
typedef struct instance_data {
   lua_State * L;
   stage_id stage;
   //true if the stage is serial (i.e. only one instance is allowed)
   bool_t serial;
   bool_t backpressure;
   size_t args;
   int instance_number;
//   long int recycled; //count the number of time this instance was recycled
} * instance;

void instance_init(size_t recycle_limit_t,size_t pending_limit_t);
instance instance_aquire(stage_id s);
int instance_release(instance i);
void instance_destroy(instance i);
bool_t instance_try_push_pending_event(instance src,stage_id dst, event e);
void push_ready_queue(instance i);

void instance_end(); //warning: thread unsafe

#endif //_INSTANCE_H_
