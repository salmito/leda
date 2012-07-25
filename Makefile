SRC_DIR=src/

MODULE=leda

all:
	cd $(SRC_DIR) && make all && cd -

5.1:
	cd $(SRC_DIR) && make 5.1 && cd -

5.2: 
	cd $(SRC_DIR) && make 5.2 && cd -

debug:
	cd $(SRC_DIR) && make debug && cd -

5.1d: 
	cd $(SRC_DIR) && make 5.1d && cd -

5.2d:
	cd $(SRC_DIR) && make 5.2d && cd -

clean:
	cd $(SRC_DIR) && make clean

ultraclean:
	cd $(SRC_DIR) && make ultraclean

install5.2: 
	cd $(SRC_DIR) && make install5.2

uninstall5.2: 
	cd $(SRC_DIR) && make uninstall5.2

install5.1: 
	cd $(SRC_DIR) && make install5.1

uninstall5.1: 
	cd $(SRC_DIR) && make uninstall5.1

install:
	cd $(SRC_DIR) && make install
	
uninstall:
	cd $(SRC_DIR) && make uninstall

tar tgz:
	cd $(SRC_DIR) && make tar

