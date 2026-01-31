O ?= ../out
KOUT := $(O)/kernel
MAPOUT := $(KOUT)/kernel.map

SYSROOT  := $(O)/sysroot
LIBDIR   := $(SYSROOT)/lib
LIBS     := $(LIBDIR)/libprintf.a
CPPFLAGS += -Iinclude -I$(SYSROOT)/include

LDS_SRC  := arch/m68k/link.ld.S
LDS_OUT  := $(KOUT)/link.ld

# Common cc/as flags
K_FLAGS   = -m68040 -g -ffreestanding -fno-builtin \
            -std=gnu23 -msoft-float -Os \
            -Wall -Wextra
CFLAGS   += $(K_FLAGS)
ASFLAGS  += $(K_FLAGS)
ASFLAGS  += -Wa,-m68040 -Wa,--register-prefix-optional
LDFLAGS  += -T $(LDS_OUT) -Map=$(MAPOUT) --print-memory-usage --no-warn-rwx-segments


SRCS_C	:= \
	main.c \
	early_alloc.c \
	printk.c \
	lib/format.c \
	arch/m68k/earlycon.c \
	arch/m68k/exception.c \
	arch/m68k/mm.c \
	arch/m68k/mm_debug.c \
	arch/m68k/setup.c

SRCS_S	:= \
	arch/m68k/entry.S \
	arch/m68k/head.S \
	arch/m68k/klib.S

OBJS := $(SRCS_C:%.c=$(KOUT)/%.o) $(SRCS_S:%.S=$(KOUT)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean

all: $(KOUT)/kernel.elf

$(KOUT)/kernel.elf: $(LDS_OUT) $(OBJS) $(LIBS)
	$(LD) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

$(KOUT)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

$(KOUT)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -MMD -MP -c -o $@ $<

$(LDS_OUT): $(LDS_SRC)
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) -E -P -x c -D__LINKER__ -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(KOUT)
