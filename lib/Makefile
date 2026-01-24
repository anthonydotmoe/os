O ?= ../out
SYSROOT := $(O)/sysroot
LIBDIR  := $(SYSROOT)/lib

# List libraries
LIBS := printf

.PHONY: all install clean $(LIBS)

all: install

install: $(LIBDIR) $(LIBS)
	@:

$(LIBDIR):
	@mkdir -p $@

# Each subdir should implement `install`
$(LIBS):
	$(MAKE) -C $@ install

clean:
	for d in $(LIBS); do $(MAKE) -C $$d clean; done
	rm -rf $(LIBDIR)