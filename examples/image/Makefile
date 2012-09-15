# change these to reflect your Lua installation
LUA= /usr
LUAINC= $(LUA)/include/lua5.1
LUALIB= $(LUA)/lib
LUABIN= $(LUA)/bin

# no need to change anything below here
CFLAGS= $(INCS) $(DEFS) $(WARN) -O2
WARN= -Wall
INCS= -I$(LUAINC) -fPIC
LIBS= -lImlib2

OBJS= limlib2.o

SOS= limlib2.so

LIBFLAG= -shared

all: limlib2.so

limlib2.so: $(OBJS)
	$(CC) -o $@ $(LIBFLAG) $(OBJS) $(LIBS)

.PHONY: clean doc test
clean:
	rm -f $(OBJS) $(SOS) *.png core core.* a.out

