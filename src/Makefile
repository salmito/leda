include ../config.in

SRC_DIR=leda/

MODULE=leda

LUA_FILES=$(SRC_DIR)/l_connector.lua $(SRC_DIR)/l_stage.lua $(SRC_DIR)/l_graph.lua $(SRC_DIR)/debug.lua $(SRC_DIR)/utils.lua
LIB_FILES=$(SRC_DIR)/kernel.so

all: 5.1

5.1: utils
	cd $(SRC_DIR)/kernel && make LUA_INC_FLAGS="$(LUA_5_1_INC)" && cd -

5.2:
	cd $(SRC_DIR)/kernel && make LUA_INC_FLAGS="$(LUA_5_2_INC)" && cd -

debug: 5.1-debug

5.1-debug: clean
	cd $(SRC_DIR)/kernel && make all LUA_INC_FLAGS="$(LUA_5_1_INC)" DEBUG_FLAGS="-ggdb -DDEBUG" && cd -

5.2-debug: clean
	cd $(SRC_DIR)/kernel && make all LUA_INC_FLAGS="$(LUA_5_2_INC)" DEBUG_FLAGS="-ggdb -DDEBUG" && cd -

clean:
	cd $(SRC_DIR)/kernel && make clean && cd - && cd $(SRC_DIR)/utils/marshal/ && make clean

ultraclean:
	cd $(SRC_DIR)/kernel && make ultraclean && cd - && cd $(SRC_DIR)/utils/marshal/ && make clean

install5.2: 5.2
	make install LUA_INSTALL_PATH=$(LUA_INSTALL_PATH_5_2) LIB_INSTALL_PATH=$(LIB_INSTALL_PATH_5_2)

uninstall5.2: 
	make uninstall LUA_INSTALL_PATH=$(LUA_INSTALL_PATH_5_2) LIB_INSTALL_PATH=$(LIB_INSTALL_PATH_5_2)

windows32:
	cd $(SRC_DIR)/kernel && make LUA_INC_FLAGS="$(LUA_5_1_INC)" MINGW32="true" && cd -

install: install5.1

uninstall: uninstall5.1

install5.1: all
	install $(SRC_DIR)/../leda.lua $(LUA_INSTALL_PATH)
	mkdir -p $(LUA_INSTALL_PATH)/leda
	mkdir -p $(LUA_INSTALL_PATH)/leda/controller
	install $(SRC_DIR)/controller/*.lua $(LUA_INSTALL_PATH)/leda/controller/
	install $(LUA_FILES) $(LUA_INSTALL_PATH)/leda
	mkdir -p $(LIB_INSTALL_PATH)/leda
	mkdir -p $(LIB_INSTALL_PATH)/leda/utils
	install $(SRC_DIR)/utils/marshal.so $(LIB_INSTALL_PATH)/leda/utils
	install $(LIB_FILES) $(LIB_INSTALL_PATH)/leda
	
uninstall5.1:
	rm -f $(LUA_INSTALL_PATH)/leda.lua
	rm -rf $(LUA_INSTALL_PATH)/leda
	rm -rf $(LIB_INSTALL_PATH)/leda
	

utils: $(SRC_DIR)/utils/marshal.so

$(SRC_DIR)/utils/marshal.so: $(SRC_DIR)/utils/marshal/lmarshal.c
	cd $(SRC_DIR)/utils/marshal/ && make

