O ?= ../out
KOUT := $(O)/kernel
MAPOUT := $(KOUT)/kernel.map

SYSROOT  := $(O)/sysroot
LIBDIR   := $(SYSROOT)/lib
LIBS     := $(LIBDIR)/libprintf.a
CPPFLAGS += -Iinclude -I$(SYSROOT)/include
CFLAGS   += -m68040 -g -ffreestanding -fno-builtin -Wall
ASFLAGS  += -Wa,-m68040 -Wa,--register-prefix-optional
LDFLAGS  += -T link.ld -Map=$(MAPOUT) --print-memory-usage --no-warn-rwx-segments

SRCS_C	:= \
	main.c \
	early_alloc.c \
	printk.c \
	lib/format.c \
	arch/m68k/earlycon.c \
	arch/m68k/mm.c \
	arch/m68k/setup.c \
	arch/m68k/traps.c

SRCS_S	:= \
	arch/m68k/head.S \
	arch/m68k/entry.S

OBJS := $(SRCS_C:%.c=$(KOUT)/%.o) $(SRCS_S:%.S=$(KOUT)/%.o)
DEPS := $(OBJS:.o=.d)

all: $(KOUT)/kernel.elf

$(KOUT)/kernel.elf: $(OBJS) $(LIBS)
	$(LD) -o $@ $(OBJS) $(LIBS) $(LDFLAGS)

$(KOUT)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -MMD -MP -c -o $@ $<

$(KOUT)/%.o: %.S
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(ASFLAGS) -MMD -MP -c -o $@ $<

-include $(DEPS)

clean:
	rm -rf $(KOUT)
