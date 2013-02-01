make CC=arm-linux-androideabi-gcc LOCK=true CFLAGS="-DANDROID -fPIC -I/usr/include/lua5.1 -Wall -O2 -Ipath_to_libevent/include/" LDFLAGS="path_to_libevent/.libs/libevent.a -lm"
