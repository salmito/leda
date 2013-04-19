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

#include "queue.h"

#include "tbb/concurrent_queue.h"

/* Arbitrary pointer type */
typedef void * __ptr;

/* Queue internal structure (Lock Free Queue) */
struct LFQueue {
   tbb::concurrent_bounded_queue<__ptr> * queue;
};

extern "C" {

/* Create a new queue and returns it */
queue queue_new() {
   queue q;
   q=new LFQueue();
   q->queue=new tbb::concurrent_bounded_queue<__ptr>();
   return q;
}

/* Push a new pointer 'source' to the queue 'q'
 *
 * Note: If the queue is full, the thread is blocked until
 * some other thread issues a pop on this queue
 */
void queue_push(queue q, void ** source) {
   __ptr p;
   p=*source;
   q->queue->push(p);
}

/* Set the maximum capacity of a queue */
void queue_set_capacity(queue q,int capacity) {
   q->queue->set_capacity(capacity);
}

/* Return the current size of the queue */
int queue_size(queue q) {
   return q->queue->size();
}

int queue_capacity(queue q) {
   return q->queue->capacity();
}


/* Push a new pointer 'source' to the queue 'q'
 *
 * Note: The caller thread never blocks 
 *
 * Returns  'true' if sucessful
 *          'false' if the queue 'q' is full
 */
int queue_try_push(queue q, void ** source) {
   __ptr p;
   p=*source;
   return (int)q->queue->try_push(p);
}

/* Verify if the queue 'q' is empty
 *
 * Return  'true' if the queue 'q' is empty
 *          'false' if otherwise
 */
int queue_isempty(queue q) {
   return (int)q->queue->empty();
}

/* Pop a new pointer from the queue 'q' and put it
 * on the 'destination' pointer.
 *
 * Note: If the queue is empty, the thread is blocked until
 * some other thread issues a push on this queue
 */
void queue_pop(queue q, void ** destination) {
   __ptr dest;
   q->queue->pop(dest);
   *destination = dest;
}

/* Pop a new pointer from the queue 'q' and put it
 * on the 'destination' pointer
 *
 * Note: The caller thread never blocks 
 *
 * Returns  'true' if sucessful
 *          'false' if the queue 'q' is empty
 */
int queue_try_pop(queue q, void ** destination) {
   __ptr dest;
   if(q->queue->try_pop(dest)) {
      *destination = dest;
      return 1;
   }
   return 0;
}

/* Clear and free the queue 'q'
 *
 * Note: Thread unsafe. All other threads waiting on the queue 'q'
 * must have been terminated before calling this function.
 */
void queue_free(queue q) {
   if(q) {
      q->queue->clear();
      delete (q->queue);
      delete (q);
   }
}

} //extern "C"




