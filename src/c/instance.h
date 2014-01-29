#ifndef _INSTANCE_H
#define _INSTANCE_H

#include "leda.h"
#include "stage.h"
#include "event.h"

typedef struct instance_s * instance_t;

enum instance_flag_t {
	CREATED=0x0,
	INITIATED=0x01
};

struct instance_s {
   lua_State *L;
   stage_t stage;
   event_t ev;
   enum instance_flag_t flags;
};

instance_t leda_newinstance(stage_t s);
void leda_initinstance(instance_t i);
void leda_destroyinstance(instance_t i);


#endif
