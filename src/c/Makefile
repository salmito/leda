LUA_INCDIR=/usr/include/lua5.1

INCFLAGS=-I$(LUA_INCDIR)
CFLAGS=-fPIC $(INCFLAGS) -Wall $(EXTRA_FLAGS) -O2
LIBEVENT_FLAGS=-levent -levent_pthreads
LDFLAGS=$(EXTRA_FLAGS) $(PTHREAD_FLAGS) $(LIBEVENT_FLAGS)

LUA=lua

CC=gcc
G_PLUS_PLUS=$(CC)

OBJ=  leda.o \
		stage.o
      
HEADERS=*.h

_SO=.so

MODULE=../leda$(_SO)

all: $(MODULE)

%.o: %.c $(HEADERS) Makefile
	$(CC) -c -o $@ $< $(CFLAGS)
	
$(MODULE): $(OBJ)
	$(G_PLUS_PLUS) -shared $^ -o $@ $(LDFLAGS)

clean:
	rm -f *.o
	
ultraclean: clean
	rm -f ../leda.so

