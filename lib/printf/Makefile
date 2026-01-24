O ?= ../../out
SYSROOT := $(O)/sysroot
LIBDIR  := $(SYSROOT)/lib

CROSS ?= m68k-elf-
CC := $(CROSS)gcc
AR := $(CROSS)ar
RANLIB := $(CROSS)ranlib

# Where objects for this lib go
POUT := $(O)/lib/printf

CPPFLAGS += -I$(SYSROOT)/include -DPRINTF_INCLUDE_CONFIG_H
CFLAGS   += -m68040 -g -ffreestanding -fno-builtin -Wall -Wextra

# Sources
SRCS_C := printf.c
OBJS   := $(SRCS_C:%.c=$(POUT)/%.o)
DEPS   := $(OBJS:.o=.d)

# Archive name
LIBNAME := libprintf.a
OUTLIB  := $(LIBDIR)/$(LIBNAME)

.PHONY: all install clean

all: $(OUTLIB)

install: $(OUTLIB)
	@:

$(OUTLIB): $(OBJS) | $(LIBDIR)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

$(LIBDIR):
	@mkdir -p $@

$(POUT)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(POUT) $(OUTLIB)
