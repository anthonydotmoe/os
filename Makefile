# Build output directory
O ?= $(abspath out)
export O

# Toolchain selection
CROSS ?= m68k-elf-
CC := $(CROSS)gcc
AS := $(CROSS)gcc
LD := $(CROSS)ld
AR := $(CROSS)ar
OBJCOPY := $(CROSS)objcopy

export CC AS LD AR OBJCOPY CROSS

.PHONY: all world headers kernel #libs servers drivers commands image clean

all: world

world: headers kernel
	@:

# 1. stage headers into $(O)/sysroot/include
headers:
	$(MAKE) -C include install

# 2. build libraries
libs: headers
	$(MAKE) -C lib all

# 3. Build kernel
kernel: #headers libs
	$(MAKE) -C kernel all

servers: headers libs
	$(MAKE) -C servers all

commands: headers libs
	$(MAKE) -C commands all

# drivers last
drivers: headers libs servers commands
	$(MAKE) -C drivers all

image: world
	$(MAKE) -C tools image

clean:
	rm -rf $(O)
