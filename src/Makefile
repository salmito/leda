include ../config.in

SRC_DIR=leda/

MODULE=leda

LUA_FILES=$(SRC_DIR)/*.lua
LIB_FILES=$(SRC_DIR)/kernel.so

all: 5.1

5.1: 
	cd $(SRC_DIR)/kernel && make LUA_INCDIR="$(LUA_INCDIR)"

5.2:
	cd $(SRC_DIR)/kernel && make LUA_INCDIR="$(LUA_5_2_INCDIR)"

debug:
	cd $(SRC_DIR)/kernel && make EXTRA_FLAGS="-ggdb -DDEBUG" LUA_INCDIR="$(LUA_INCDIR)"

5.2-debug: 
	cd $(SRC_DIR)/kernel && make LUA_INCDIR="$(LUA_5_2_INCDIR)" EXTRA_FLAGS="-ggdb -DDEBUG"

opt:
	cd $(SRC_DIR)/kernel && make LUA_INCDIR="$(LUA_INCDIR)" EXTRA_FLAGS="-Wall -s -O9 -msse4.2 -ffast-math -ftree-vectorize -funroll-loops"

clean:
	cd $(SRC_DIR)/kernel && make clean 

ultraclean:
	cd $(SRC_DIR)/kernel && make ultraclean
	cd $(SRC_DIR) && rm -f `find -iname *~`

install5.2: 5.2
	make install LUA_INSTALL_PATH=$(LUA_INSTALL_PATH_5_2) LIB_INSTALL_PATH=$(LIB_INSTALL_PATH_5_2)

uninstall5.2: 
	make uninstall LUA_INSTALL_PATH=$(LUA_INSTALL_PATH_5_2) LIB_INSTALL_PATH=$(LIB_INSTALL_PATH_5_2)

install-both:
	make clean; make install; make clean; make install5.2


install: all
	install $(SRC_DIR)/../leda.lua $(LUA_INSTALL_PATH)
	mkdir -p $(LUA_INSTALL_PATH)/leda
	mkdir -p $(LUA_INSTALL_PATH)/leda/utils
	mkdir -p $(LUA_INSTALL_PATH)/leda/stage
	mkdir -p $(LUA_INSTALL_PATH)/leda/controller
	install $(SRC_DIR)/controller/*.lua $(LUA_INSTALL_PATH)/leda/controller/
	install $(SRC_DIR)/stage/*.lua $(LUA_INSTALL_PATH)/leda/stage/
	install $(SRC_DIR)/utils/*.lua $(LUA_INSTALL_PATH)/leda/utils/
	install $(LUA_FILES) $(LUA_INSTALL_PATH)/leda
	mkdir -p $(LIB_INSTALL_PATH)/leda
	mkdir -p $(LIB_INSTALL_PATH)/leda/utils
	install $(LIB_FILES) $(LIB_INSTALL_PATH)/leda
	
uninstall:
	rm -f $(LUA_INSTALL_PATH)/leda.lua
	rm -rf $(LUA_INSTALL_PATH)/leda
	rm -rf $(LIB_INSTALL_PATH)/leda
