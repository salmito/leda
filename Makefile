SRC_DIR=src/

MODULE=leda

all:
	cd $(SRC_DIR) && make all
	
%:
	cd $(SRC_DIR) && make $@

tar tgz: ultraclean
ifeq "$(VERSION)" ""
	echo "Usage: make tar VERSION=x.x"; false
else
	-rm -rf $(MODULE)-$(VERSION)
	mkdir $(MODULE)-$(VERSION)
	tar c * --exclude="*.tar.gz" --exclude=".git" --exclude="$(MODULE)-*" | (cd $(MODULE)-$(VERSION) && tar x)
	tar czvf $(MODULE)-$(VERSION).tar.gz $(MODULE)-$(VERSION)
	rm -rf $(MODULE)-$(VERSION)
	md5sum $(MODULE)-$(VERSION).tar.gz > $(MODULE)-$(VERSION).md5
endif

