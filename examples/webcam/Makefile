all:
	cd v4l-lua && make && cd -
	cd luaglut && make && cd -
	cd ../image && make && cd - && ln -sf ../image/limlib2.so . 
	gcc -shared -o filter.so filter.c -fPIC -O2 -I/usr/include/lua5.1 -lm

clean:
	cd v4l-lua && make clean && cd -
	cd luaglut && make clean && cd -
	cd ../image && make clean && cd -
	rm -f *.so
