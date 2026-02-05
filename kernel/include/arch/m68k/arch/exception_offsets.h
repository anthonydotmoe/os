#pragma once

/*
 * Deterministic offsets for exc_frame_header_t used by assembly.
 * Keep these in sync with kernel/arch/m68k/exception.h.
 */
#define EXC_HDR_SR_OFF  0
#define EXC_HDR_PC_OFF  2
#define EXC_HDR_SIZE    8

#ifndef __ASSEMBLER__
#include <stddef.h>

#include "exception.h"

_Static_assert(EXC_HDR_SR_OFF == offsetof(exc_frame_header_t, sr), "exc hdr sr offset");
_Static_assert(EXC_HDR_PC_OFF == offsetof(exc_frame_header_t, pc), "exc hdr pc offset");
_Static_assert(EXC_HDR_SIZE   ==   sizeof(exc_frame_header_t),     "exc hdr size");
#endif
