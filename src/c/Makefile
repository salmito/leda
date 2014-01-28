LUA_INCDIR=/usr/include/lua5.1

INCFLAGS=-I$(LUA_INCDIR)
CFLAGS=-fPIC $(INCFLAGS) -Wall $(EXTRA_FLAGS) -O2
#LIBEVENT_FLAGS=-levent -levent_pthreads
LIBTBB_FLAGS=-ltbb
LDFLAGS=$(EXTRA_FLAGS) $(PTHREAD_FLAGS) $(LIBTBB_FLAGS) # $(LIBEVENT_FLAGS)

LUA=lua

CC=gcc
G_PLUS_PLUS=g++

OBJ=  leda.o \
		stage.o \
		lf_queue.o \
		lf_hash.o \
		marshal.o \
		event.o \
		instance.o
      
HEADERS=*.h

_SO=.so

MODULE=../leda$(_SO)

all: $(MODULE)

%.o: %.c $(HEADERS) Makefile
	$(CC) -c -o $@ $< $(CFLAGS)

lf_hash.o: lf_hash.c
	$(CC) -std=c99 -c -o $@ $< $(CFLAGS)	

lf_queue.o: lf_queue.cpp
	$(G_PLUS_PLUS) -c -o $@ $< $(CFLAGS)
	
$(MODULE): $(OBJ)
	$(G_PLUS_PLUS) -shared $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o
	
ultraclean: clean
	rm -f ../leda.so

