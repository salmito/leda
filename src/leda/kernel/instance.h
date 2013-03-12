#ifndef _INSTANCE_H_
#define _INSTANCE_H_

#include "extra/threading.h"
#include "graph.h"

struct event_data;

/* lua state structure */
typedef struct instance_data {
   lua_State * L;
   stage_id stage;
   size_t args;
   int instance_number;
   time_d init_time;
//   long int recycled; //count the number of time this instance was recycled
} * instance;

lua_State * new_lua_state(bool_t libs);
void instance_init(size_t recycle_limit_t,size_t pending_limit_t);
instance instance_aquire(stage_id s);
instance instance_wait(stage_id s);
int instance_release(instance i);
int recycle_queue_capacity(stage_id s);
int event_queue_size(stage_id s);
int event_queue_capacity(stage_id s);
void instance_destroy(instance i);
bool_t instance_try_push_pending_event(stage_id dst, struct event_data * e);
void push_ready_queue(instance i);
int instance_set_maxpar(lua_State * L);
void event_wait_io(instance i);
void event_do_file_aio(instance i);
void event_sleep(instance i);
void instance_end(); //warning: thread unsafe

#endif //_INSTANCE_H_

