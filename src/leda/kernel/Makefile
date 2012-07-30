INCFLAGS=-I$(LUA_INC_FLAGS)
CFLAGS=-fPIC $(INCFLAGS) -Wall -O2 $(DEBUG_FLAGS)
LDFLAGS=-fPIC -pthread -ltbb -Wall -O2 $(DEBUG_FLAGS)
LUA=lua

OBJ=  kernel.o \
      graph.o \
      instance.o \
		mutex.o \
      thread.o \
      event.o \
      extra/threading.o \
      extra/leda-io.o \
		tbb_queue.o \
		tbb_atomic.o \

HEADERS=*.h extra/*.h

_SO=.so
ifeq "$(findstring MINGW32,$(shell uname -s))" "MINGW32"
  _SO=.dll
endif

MODULE=../kernel$(_SO)

all: $(MODULE)

instance.o: instance.lo

tbb_queue.o: tbb_queue.cpp $(HEADERS) Makefile
	g++ -c -o $@ $< $(CFLAGS)

tbb_atomic.o: tbb_atomic.cpp $(HEADERS) Makefile
	g++ -c -o $@ $< $(CFLAGS)

%.o: %.c $(HEADERS) Makefile
	$(CC) -c -o $@ $< $(CFLAGS)
	
%.lo: %.lua
	$(LUA) extra/bin2c.lua $< -o $@

$(MODULE): $(OBJ)
	$(CC) -shared $^ -o $@ $(LDFLAGS)

clean:
	rm -f $(OBJ) instance.lo
	
ultraclean:
	rm -f $(OBJ) instance.lo $(MODULE)

