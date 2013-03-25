LUA_INC_FLAGS=/usr/include/lua5.1

INCFLAGS=-I$(LUA_INC_FLAGS)
CFLAGS=-fPIC $(INCFLAGS) -Wall -O2 $(DEBUG_FLAGS)
LDFLAGS=$(DEBUG_FLAGS) $(LD_FLAGS) -ltbb -lpthread -levent -levent_pthreads
      
LUA=lua

CC=cc
G_PLUS_PLUS=$(CC)

OBJ=  kernel.o \
      graph.o \
      instance.o \
		mutex.o \
      thread.o \
      event.o \
      stats.o \
      extra/threading.o \
      extra/leda-io.o \
      extra/lmarshal.o \
      
ifdef LOCK
OBJ+= queue.o \
      atomic.o    
else
OBJ+= tbb_queue.o \
      tbb_atomic.o 
endif
HEADERS=*.h extra/*.h

ifdef STATS_OFF
   CFLAGS+=-DSTATS_OFF
endif


ifeq ($(MINGW32),true)
   _SO=.dll
else
   _SO=.so
endif

MODULE=../kernel$(_SO)

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
	rm -f ../kernel.so ../kernel.dll

