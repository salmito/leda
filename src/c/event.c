#include "event.h"

#include <string.h>

event_t leda_newevent(const char * ev, size_t len) {
   event_t e=malloc(sizeof(struct event_s));
   e->data=malloc(len);
   memcpy(e->data,ev,len);
   e->len=len;
   return e;
}

void leda_destroyevent(event_t e) {
   free(e->data);
   free(e);
}
