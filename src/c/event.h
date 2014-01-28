#ifndef _EVENT_H_
#define _EVENT_H_

#include <stdlib.h>

typedef struct event_s * event_t;

event_t leda_newevent(const char * ev, size_t len);
void leda_destroyevent(event_t e);

#endif
