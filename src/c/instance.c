#include "instance.h"
#include "event.h"

#include <lua.h>

struct instance_s {
   lua_State *L;
   event_t ev;
};
