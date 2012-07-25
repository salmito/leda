INCFLAGS=-I$(LUA_INC_FLAGS)
CFLAGS=-fPIC $(INCFLAGS) -Wall -O2 $(DEBUG_FLAGS)
LDFLAGS=-fPIC -pthread -ltbb -Wall -O2 $(DEBUG_FLAGS)
LUA=lua

OBJ=  kernel.o \
      graph.o \
      instance.o \
		queue.o \
		atomic.o \
      thread.o \
      extra/threading.o \
      #extra/tools.c \

HEADERS=*.h extra/*.h

_SO=.so
ifeq "$(findstring MINGW32,$(shell uname -s))" "MINGW32"
  _SO=.dll
endif

MODULE=../kernel$(_SO)

all: $(MODULE)

instance.o: instance.lo

queue.o: queue.cpp $(HEADERS) Makefile
	g++ -c -o $@ $< $(CFLAGS)

atomic.o: atomic.cpp $(HEADERS) Makefile
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

