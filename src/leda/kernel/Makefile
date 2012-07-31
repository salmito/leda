LUA_INC_FLAGS=/usr/include/lua5.1
INCFLAGS=-I$(LUA_INC_FLAGS)
CFLAGS=-fPIC $(INCFLAGS) -Wall -O2 $(DEBUG_FLAGS)
LDFLAGS_POSIX=-fPIC -Wall -O2 $(DEBUG_FLAGS) -ltbb -lpthread
LDFLAGS_MINGW32=-fPIC -Wall -O2 $(DEBUG_FLAGS) \
      -L extra/dll -ltbb -lpthread-i386 -llua51    
      
LUA=lua

CC=cc
G_PLUS_PLUS=g++

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
ifeq ($(MINGW32),true)
#INCFLAGS=-IC:\lua-5.1.4\src -IC:\tbb30_20110704oss\include
LDFLAGS=$(LDFLAGS_MINGW32)
_SO=.dll
CFLAGS=$(INCFLAGS) -Wall -O2 $(DEBUG_FLAGS)
CC=i586-mingw32msvc-cc
G_PLUS_PLUS=i586-mingw32msvc-g++
else
LDFLAGS=$(LDFLAGS_POSIX)
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
	rm -f $(OBJ) instance.lo
	
ultraclean:
	rm -f $(OBJ) instance.lo ../kernel.so ../kernel.dll

