#include "lf_queue.h"

#include <tbb/concurrent_queue.h>
#include <stdlib.h>

typedef void * __ptr;

/* Queue internal structure (Lock Free Queue) */
struct LFqueue {
   tbb::concurrent_bounded_queue<__ptr> * queue;
};

extern "C" {
	

/* Create a new queue */
LFqueue_t leda_lfqueue_new() {
   LFqueue_t q;
   q=new LFqueue();
   q->queue=new tbb::concurrent_bounded_queue<__ptr>();
   return q;
}

void leda_lfqueue_push(LFqueue_t q,void ** source) {
   __ptr p;
   p=*source;
   q->queue->push(p);
}

int leda_lfqueue_try_push(LFqueue_t q,void ** source) {
	__ptr p;
   p=*source;
   return q->queue->try_push(p);
}

int leda_lfqueue_try_pop(LFqueue_t q, void ** destination) {
   __ptr dest;
   if(q->queue->try_pop(dest)) {
      *destination = dest;
      return 1;
   }
   return 0;
}

void leda_lfqueue_pop(LFqueue_t q, void ** destination) {
   __ptr dest;
   q->queue->pop(dest);
   *destination = dest;
}

void leda_lfqueue_setcapacity(LFqueue_t q,int capacity) {
	return q->queue->set_capacity(capacity);
}

int leda_lfqueue_getcapacity(LFqueue_t q) {
   return q->queue->capacity();
}

int leda_lfqueue_isempty(LFqueue_t q) {
   return (int)q->queue->empty();
}

int leda_lfqueue_size(LFqueue_t q) {
	return q->queue->size();
}

//possibly thread unsafe
void leda_lfqueue_free(LFqueue_t q) {
 if(q) {
      q->queue->clear();
      delete (q->queue);
      delete (q);
   }
}



}
