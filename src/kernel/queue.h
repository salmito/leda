#ifndef _QUEUE_H_
#define _QUEUE_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct LFQueue * queue;

#define PUSH(q,p) queue_push((q),(void **)&(p))
#define TRY_PUSH(q,p) queue_try_push((q),(void **)&(p))
#define POP(q,p) queue_pop((q),(void **)&(p))
#define TRY_POP(q,p) queue_try_pop((q),(void **)&(p))

queue queue_new();
void queue_push(queue q,void ** source);
int queue_try_push(queue q,void ** source);
void queue_set_capacity(queue q,int capacity);
int queue_capacity(queue q);
int queue_isempty(queue q);
int queue_try_pop(queue q, void ** destination);
void queue_pop(queue q, void ** destination);
int queue_size(queue q);

//thread unsafe
void queue_free(queue q);

#ifdef __cplusplus
}
#endif


#endif //_QUEUE_H_
