INSTALL_PATH=/usr/local/lib/lua/5.1/

all:
	echo "Nothing to compile, use: sudo make install"

install: leda.lua leda/cmdline.lua leda/datadumper.lua leda/dumper.lua leda/runtime.lua leda/scheduler.lua leda/debug.lua leda/server.lua
	install leda.lua $(INSTALL_PATH)
	mkdir -p $(INSTALL_PATH)/leda
	install leda/cmdline.lua leda/datadumper.lua leda/dumper.lua leda/runtime.lua leda/scheduler.lua leda/debug.lua leda/server.lua $(INSTALL_PATH)/leda

uninstall:
	rm -rf $(INSTALL_PATH)/leda.lua $(INSTALL_PATH)/leda
