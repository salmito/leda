LUA_INSTALL_PATH=/usr/share/lua/5.1
LIB_INSTALL_PATH=/usr/local/lib/lua/5.1

LUA_INSTALL_PATH_5_2=/usr/share/lua/5.2
LIB_INSTALL_PATH_5_2=/usr/local/lib/lua/5.2

LUA_5_2_INC=/usr/local/include
LUA_5_1_INC=/usr/include/lua5.1

DEFAULT_LUA_INC=$(LUA_5_1_INC)

SRC_DIR=leda/

LUA_FILES=$(SRC_DIR)/l_connector.lua $(SRC_DIR)/l_stage.lua $(SRC_DIR)/l_graph.lua $(SRC_DIR)/debug.lua $(SRC_DIR)/utils.lua
LIB_FILES=$(SRC_DIR)/kernel.so

all:
	cd $(SRC_DIR)/kernel && make all LUA_INC_FLAGS="$(DEFAULT_LUA_INC)" && cd -

5.1: clean
	cd $(SRC_DIR)/kernel && make all LUA_INC_FLAGS="$(LUA_5_1_INC)" && cd -

5.2: clean
	cd $(SRC_DIR)/kernel && make all LUA_INC_FLAGS="$(LUA_5_2_INC)" && cd -

debug: clean
	cd $(SRC_DIR)/kernel && make all LUA_INC_FLAGS="$(DEFAULT_LUA_INC)" DEBUG_FLAGS="-ggdb -DDEBUG" && cd -

5.1d: clean
	cd $(SRC_DIR)/kernel && make all LUA_INC_FLAGS="$(LUA_5_1_INC)" DEBUG_FLAGS="-ggdb -DDEBUG" && cd -

5.2d: clean
	cd $(SRC_DIR)/kernel && make all LUA_INC_FLAGS="$(LUA_5_2_INC)" DEBUG_FLAGS="-ggdb -DDEBUG" && cd -

clean:
	cd $(SRC_DIR)/kernel && make clean

ultraclean:
	cd $(SRC_DIR)/kernel && make ultraclean

install5.2: 5.2
	make install LUA_INSTALL_PATH=$(LUA_INSTALL_PATH_5_2) LIB_INSTALL_PATH=$(LIB_INSTALL_PATH_5_2)

uninstall5.2: 
	make uninstall LUA_INSTALL_PATH=$(LUA_INSTALL_PATH_5_2) LIB_INSTALL_PATH=$(LIB_INSTALL_PATH_5_2)

install: all
	install $(SRC_DIR)/../leda.lua $(LUA_INSTALL_PATH)
	mkdir -p $(LUA_INSTALL_PATH)/leda
	mkdir -p $(LUA_INSTALL_PATH)/leda/controller
	install $(SRC_DIR)/controller/*.lua $(LUA_INSTALL_PATH)/leda/controller/
	install $(LUA_FILES) $(LUA_INSTALL_PATH)/leda
	mkdir -p $(LIB_INSTALL_PATH)/leda
	install $(LIB_FILES) $(LIB_INSTALL_PATH)/leda
	
uninstall:
	rm -f $(LUA_INSTALL_PATH)/leda.lua
	rm -rf $(LUA_INSTALL_PATH)/leda
	rm -rf $(LIB_INSTALL_PATH)/leda

tar tgz:
ifeq "$(VERSION)" ""
	echo "Usage: make tar VERSION=x.x"; false
else
	$(MAKE) clean
	-rm -rf $(MODULE)-$(VERSION)
	mkdir $(MODULE)-$(VERSION)
	tar c * --exclude=.svn --exclude=.DS_Store --exclude="_*" \
	        --exclude="*.tgz" --exclude="*.rockspec" \
	        --exclude=lanes.dev --exclude="$(MODULE)-*" --exclude=xcode \
		    --exclude="*.obj" --exclude="*.dll" --exclude=timeit.dat \
	   | (cd $(MODULE)-$(VERSION) && tar x)
	tar czvf $(MODULE)-$(VERSION).tgz $(MODULE)-$(VERSION)
	rm -rf $(MODULE)-$(VERSION)
	md5sum $(MODULE)-$(VERSION).tgz
endif

