INCFLAGS=-I$(LUA_INCDIR)
CFLAGS=-fPIC $(INCFLAGS) -Wall $(EXTRA_FLAGS) -O2
TBBLIB_FLAG=-ltbb
LDFLAGS=$(EXTRA_FLAGS) -lpthread -levent -levent_pthreads
     
LUA=lua

CC=gcc
G_PLUS_PLUS=$(CC)

OBJ=  kernel.o \
      graph.o \
      instance.o \
      scheduler.o \
      event.o \
      extra/threading.o \
      extra/memarray.o \
      extra/leda-io.o \
      extra/lmarshal.o \
#		extra/mutex.o \
      
ifdef DISABLE_TBB
OBJ+= queue.o \
      atomic.o    
else
OBJ+= tbb_queue.o \
      tbb_atomic.o 
LDFLAGS+=$(TBBLIB_FLAG)
endif
HEADERS=*.h extra/*.h

ifdef DISABLE_STATS
   CFLAGS+=-DSTATS_OFF
   OBJ+=stats.o
else
   OBJ+=stats.o
endif

_SO=.so

MODULE=../leda$(_SO)

all: $(MODULE)

instance.o: instance.lo

tbb_queue.o: tbb_queue.cpp $(HEADERS) Makefile
	$(G_PLUS_PLUS) -c -o $@ $< $(CFLAGS)

tbb_atomic.o: tbb_atomic.cpp $(HEADERS) Makefile
	$(G_PLUS_PLUS) -c -o $@ $< $(CFLAGS)

%.o: %.c $(HEADERS) Makefile
	$(CC) -c -o $@ $< $(CFLAGS)
	
%.lo: %.lua
	$(LUA) extra/bin2c.lua $< -o $@

$(MODULE): $(OBJ)
	$(G_PLUS_PLUS) -shared $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o extra/*.o instance.lo
	
ultraclean: clean
	rm -f ../leda.so

