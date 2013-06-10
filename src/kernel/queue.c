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
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "queue.h"
#include "scheduler.h"

/* Arbitrary pointer type */
typedef void * __ptr;

struct queue_element {
   __ptr ptr;
   struct queue_element * next;
};

/* Queue internal structure (Locked Queue) */
struct LFQueue {
   struct queue_element * head;
   struct queue_element * tail;
   int size;
   int capacity;
   MUTEX_T lock;
   SIGNAL_T full_queue;
   SIGNAL_T empty_queue;
};

/* Create a new queue and returns it */
queue queue_new() {
   queue q=malloc(sizeof(struct LFQueue));
   if(q==NULL) return NULL; //malloc failed
   q->head=q->tail=NULL;
   q->size=0;
   q->capacity=-1;
   MUTEX_INIT(&q->lock);
   SIGNAL_INIT(&q->full_queue);
   SIGNAL_INIT(&q->empty_queue);
   return q;
}



/* Push a new pointer 'source' to the queue 'q'
 *
 * Note: If the queue is full, the thread is blocked until
 * some other thread issues a pop on this queue
 */
void queue_push(queue q, void ** source) {
   MUTEX_LOCK(&q->lock);
   if(q->capacity>=0)
      while(q->size==q->capacity)
         SIGNAL_WAIT(&q->full_queue,&q->lock,-1);
   _DEBUG("Queue: Push: %p size=%d\n",*source,q->size);

   struct queue_element * el=malloc(sizeof(struct queue_element));
   el->ptr=*source;
   el->next=NULL;
   
   if(q->tail==NULL) { //List was empty
      q->head=q->tail=el;
      ++ q->size;
      MUTEX_UNLOCK(&q->lock);
      SIGNAL_ONE(&q->empty_queue);
   } else {
      q->tail->next=el;
      q->tail=el;
      ++ q->size;
      MUTEX_UNLOCK(&q->lock);
   }
}

/* Set the maximum capacity of a queue */
void queue_set_capacity(queue q,int capacity) {
   MUTEX_LOCK(&q->lock);
   q->capacity=capacity;
   //TODO verify queue intregrity
   MUTEX_UNLOCK(&q->lock);
}

/* get the capacity of a queue */
int queue_capacity(queue q) {
  return q->capacity;
}

/* Return the current size of the queue */
int queue_size(queue q) {
   MUTEX_LOCK(&q->lock);
   int size=q->size;
   MUTEX_UNLOCK(&q->lock);
   return size;
}

/* Push a new pointer 'source' to the queue 'q'
 *
 * Note: The caller thread never blocks 
 *
 * Returns  'true' if sucessful
 *          'false' if the queue 'q' is full
 */
int queue_try_push(queue q, void ** source) {
   MUTEX_LOCK(&q->lock);
   if(q->capacity>=0)
      if(q->size==q->capacity) {
         MUTEX_UNLOCK(&q->lock);
         return FALSE;
      }
   struct queue_element * el=malloc(sizeof(struct queue_element));
   el->ptr=*source;
   el->next=NULL;
   if(q->size<=0) { //List was empty
      q->head=q->tail=el;
      ++ q->size;
      MUTEX_UNLOCK(&q->lock);
      SIGNAL_ONE(&q->empty_queue);   
   } else {
      q->tail->next=el;
      q->tail=el;
      ++ q->size;
      MUTEX_UNLOCK(&q->lock);
   }
   _DEBUG("Queue: Try push (success): %p tail=%p size=%d\n",*source,q->tail,q->size);
   return TRUE;
}

/* Verify if the queue 'q' is empty
 *
 * Return  'true' if the queue 'q' is empty
 *          'false' if otherwise
 */
int queue_isempty(queue q) {
   return queue_size(q)<=0;
}

/* Pop a new pointer from the queue 'q' and put it
 * on the 'destination' pointer.
 *
 * Note: If the queue is empty, the thread is blocked until
 * some other thread issues a push on this queue
 */
void queue_pop(queue q, void ** destination) {
   MUTEX_LOCK(&q->lock);
   -- q->size;
   while(q->head==NULL) {
         SIGNAL_WAIT(&q->empty_queue,&q->lock,-1);
      }
   *destination=q->head->ptr;
   _DEBUG("Queue: Popped: %p\n",*destination);
   struct queue_element * el=q->head;
   q->head=el->next;
   free(el);

   if(q->capacity>0 && q->size+1==q->capacity) { //List was full
      MUTEX_UNLOCK(&q->lock);
      SIGNAL_ONE(&q->full_queue);
   }
   MUTEX_UNLOCK(&q->lock);
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
   MUTEX_LOCK(&q->lock);
   while(q->size<=0) {
      MUTEX_UNLOCK(&q->lock);
      return FALSE;
   }
   -- q->size;
   *destination=q->head->ptr;
   _DEBUG("Queue: Try Pop (success): %p\n",*destination);
   struct queue_element * el=q->head;
   q->head=el->next;
   free(el);

   if(q->capacity>0 && q->size+1==q->capacity) { //List was full
      MUTEX_UNLOCK(&q->lock);
      SIGNAL_ONE(&q->full_queue);
   }
   MUTEX_UNLOCK(&q->lock);
   return TRUE;
}

/* Clear and free the queue 'q'
 *
 * Note: Thread unsafe. All other threads waiting on the queue 'q'
 * must have been terminated before calling this function.
 */
void queue_free(queue q) {
   _DEBUG("Queue: Free: %p\n",q);
    while(q->head) {
      __ptr p;
      if(queue_try_pop(q,&p))
         free(p);
   }
   SIGNAL_FREE(&q->full_queue);
   SIGNAL_FREE(&q->empty_queue);
   MUTEX_FREE(&q->lock);
   free (q);
}

