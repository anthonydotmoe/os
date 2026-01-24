O ?= ../out
SYSROOT := $(O)/sysroot
INCLUDEDIR := $(SYSROOT)/include

# Find all header files under this directory
HDRS := $(shell find . -type f -name '*.h')

# Convert "./asm/bootinfo.h" -> "$(INCLUDEDIR)/asm/bootinfo.h"
INST_HDRS := $(HDRS:./%=$(INCLUDEDIR)/%)

.PHONY: all install clean

all: install

install: $(INST_HDRS)

# Install rule: copy header, preserving directory structure
$(INCLUDEDIR)/%: %
	@mkdir -p $(@D)
	cp $< $@

clean:
	rm -rf $(INCLUDEDIR)
