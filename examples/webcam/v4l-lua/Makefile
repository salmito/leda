all:
	gcc -Wall -pedantic -fPIC v4l_lua.c core.c -shared -o ../v4l.so -I /usr/include/lua5.1/ -llua5.1 -lv4lconvert -lpthread 

clean:
	rm -f ../v4l.so
