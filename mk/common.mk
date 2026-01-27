# Common toolchain and flags for the project (GNU make)

# Build output directory
O ?= $(abspath out)

# Sysroot staging area
SYSROOT    ?= $(O)/sysroot
INCLUDEDIR ?= $(SYSROOT)/include
LIBDIR     ?= $(SYSROOT)/lib

# Toolchain selection
CROSS   ?= m68k-elf-
CC      ?= $(CROSS)gcc
AS      ?= $(CROSS)gcc
LD      ?= $(CROSS)ld
AR      ?= $(CROSS)ar
RANLIB  ?= $(CROSS)ranlib
OBJCOPY ?= $(CROSS)objcopy

# Common compilation flags
CPPFLAGS_COMMON := 
CFLAGS_COMMON   :=
ASFLAGS_COMMON  :=

CFLAGS_COMMON  += -ffreestanding -fno-builtin
CFLAGS_COMMON  += -Wall -Wextra -Wpedantic

CFLAGS_COMMON  += -m68040
ASFLAGS_COMMON += -Wa,-m68040 -Wa,--register-prefix-optional

# Dependency generation flags
DEPFLAGS := -MMD -MP

# Default include path to staged headers
CPPFLAGS_COMMON += -isystem$(INCLUDEDIR)

# Uility helpers
mkdir_p = @mkdir -p $(@D)
