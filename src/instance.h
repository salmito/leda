#ifndef _INSTANCE_H
#define _INSTANCE_H

#include "leda.h"
#include "stage.h"
#include "event.h"

typedef struct instance_s * instance_t;

enum instance_flag_t {
	CREATED=0x0,
	IDLE,
	READY,
	WAITING_IO
};

struct instance_s {
   lua_State *L;
   stage_t stage;
   event_t ev;
   enum instance_flag_t flags;
   int args;
};

instance_t leda_newinstance(stage_t s);
void leda_initinstance(instance_t i);
void leda_destroyinstance(instance_t i);
void leda_putinstance(instance_t i);

#endif
