TOP := $(abspath ..)
include $(TOP)/mk/common.mk

SUBDIRS := printf

.PHONY: all install clean $(SUBDIRS)

all: install

install: $(SUBDIRS)
	@:

$(SUBDIRS):
	$(MAKE) -C $@ install TOP=$(TOP) O=$(O)

clean:
	for d in $(LIBS); do $(MAKE) -C $$d clean TOP=$(TOP) O=$(O); done
