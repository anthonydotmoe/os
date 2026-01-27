# Generic rules to build a static library and install it to $(SYSROOT)/lib

# Require: LIB and SRCS (at least)
ifndef LIB
$(error LIB is not set (e.g. LIB=printf))
endif

ifndef SRCS
$(error SRCS is not set (e.g. SRCS=printf.c))
endif

# Include common variables/tools/flags
include $(TOP)/mk/common.mk

# Allow per-lib overrides/additions

CPPFLAGS ?=
CFLAGS   ?=

# Derived names/paths
LIBNAME := lib$(LIB).a
OUTLIB  := $(LIBDIR)/$(LIBNAME)

# Where objects for this library live
#   out/lib/<libname>/...
LOUT    := $(O)/lib/$(LIB)

# Sources -> objects
SRCS_C := $(filter %.c,$(SRCS))
SRCS_S := $(filter %.S,$(SRCS))

OBJS_C := $(SRCS_C:%.c=$(LOUT)/%.o)
OBJS_S := $(SRCS_S:%.S=$(LOUT)/%.o)
OBJS   := $(OBJS_C) $(OBJS_S)
DEPS   := $(OBJS:.o=.d)

# Headers to install (optional)
# INCS can be empty, if set, they'll be staged under INCLUDEDIR
INCS       ?=
INC_SUBDIR ?= $(LIB)
INST_HDRS  := $(INCS:%=$(INCLUDEDIR)/$(INC_SUBDIR)/%)

.PHONY: all install clean

all: $(OUTLIB)

install: $(OUTLIB) $(INST_HDRS)
	@:

# Build/install the archive
$(OUTLIB): $(OBJS) | $(LIBDIR)
	$(AR) rcs $@ $(OBJS)
	$(RANLIB) $@

$(LIBDIR):
	@mkdir -p $@

# Install headers
$(INCLUDEDIR)/$(INC_SUBDIR)/%: %
	@mkdir -p $(@D)
	cp $< $@

# Compile C
$(LOUT)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS_COMMON) $(CPPFLAGS) $(CFLAGS_COMMON) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

# Assemble S
$(LOUT)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS_COMMON) $(CPPFLAGS) $(CFLAGS_COMMON) $(CFLAGS) $(ASFLAGS_COMMON) $(DEPFLAGS) -c -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(LOUT) $(OUTLIB) $(INST_HDRS)

