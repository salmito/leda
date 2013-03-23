# ======================================================================
# Makefile - Copyright (C) 2005-2006 Varol Kaptan
# see LICENSE for more information
# ======================================================================
# vim: set ts=3:

ARCH = $(shell uname -p)
OS   = $(shell uname -s)

GLUT_LIBS = -lglut
GL_LIBS = -lGL -lGLU

CFLAGS=-fPIC -I/usr/include/lua5.1

LUAGL = ../luagl.so
LUAGLUT = ../luaglut.so
MEMARRAY = ../memarray.so

CC = gcc -Wall $(CFLAGS)
GCC_OPT_SHARED = -shared
LDSO = gcc $(LFLAGS) $(GCC_OPT_SHARED)

ifeq ($(OS),Darwin)
	LUAGL = luagl.bundle
	LUAGLUT = luaglut.bundle
	MEMARRAY = memarray.bundle
	GCC_OPT_SHARED = -bundle -undefined dynamic_lookup
	GL_LIBS = -framework OpenGL
	# Default to Apple GLUT on Mac OS X (you can still use the others)
	GLUT_LIBS = -framework GLUT
endif

ifeq ($(OS),MINGW32_NT-5.1)
	LUAGL = luagl.dll
	LUAGLUT = luaglut.dll
	MEMARRAY = memarray.dll
	GL_LIBS = -lopengl32
	GLUT_LIBS = -lglu32 -lglut32
	LIBS += -Wl,--export-all-symbols
endif

ifdef USE_FREEGLUT
GLUT_DEFS = -DUSE_FREEGLUT
GLUT_LIBS = -lglut
endif

ifdef USE_OPENGLUT
GLUT_DEFS = -DUSE_OPENGLUT
GLUT_LIBS = -lopenglut
endif

ifdef USE_OPENGLEAN
GLUT_DEFS = -DUSE_OPENGLEAN
GLUT_LIBS = -lopenglean
endif

TARGETS = $(LUAGL) $(LUAGLUT) $(MEMARRAY)

all: $(TARGETS)

#install: $(TARGETS)
#	mkdir -p $(HOME)/lib/lua
#	cp $^ $(HOME)/lib/lua

$(LUAGLUT): luaglut.o
	$(LDSO) -o $@ $< $(LIBS) $(GL_LIBS) $(GLUT_LIBS)

$(LUAGL): luagl.o
	$(LDSO) -o $@ $< $(LIBS) $(GL_LIBS)

$(MEMARRAY): memarray.o
	$(LDSO) -o $@ $< $(LIBS)

menus.h: menus.lua
	xxd -i menus.lua | sed 's/^unsigned/static const unsigned/' >menus.h

luaglut.o: luaglut.c macros.h menus.h
	$(CC) $(DEFS) $(GLUT_DEFS) -c $<

luagl.o: luagl.c macros.h
	$(CC) $(DEFS) -c $<

memarray.o: memarray.c
	$(CC) $(DEFS) -DMEMARRAY_USE_OPENGL -c $<

macros.h: macros.lua
	lua macros.lua >macros.h

clean:
	rm -f *.o $(TARGETS)

devclean: clean
	rm -f macros.h menus.h
