#include "event.h"

#include <string.h>

struct event_s {
   char * ev;
   size_t len;
};

event_t leda_newevent(const char * ev, size_t len) {
   event_t e=malloc(sizeof(struct event_s));
   e->ev=malloc(len);
   memcpy(e->ev,ev,len);
   e->len=len;
   return e;
}

void leda_destroyevent(event_t e) {
   free(e->ev);
   free(e);
}
