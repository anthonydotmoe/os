#pragma once

/*
 * Deterministic offsets for m68k_user_ctx_t used by assembly.
 * Keep these in sync with kernel/arch/m68k/context.h.
 */
#define M68K_CTX_GPR    0
#define M68K_CTX_D0     0
#define M68K_CTX_A0     32
#define M68K_CTX_USP    60
#define M68K_CTX_PC     64
#define M68K_CTX_SR     68
#define M68K_CTX_SIZE   72

#ifndef __ASSEMBLER__
#include <stddef.h>

#include "context.h"

_Static_assert(M68K_CTX_D0   == offsetof(m68k_user_ctx_t, d[0]), "m68k ctx d0 offset");
_Static_assert(M68K_CTX_A0   == offsetof(m68k_user_ctx_t, a[0]), "m68k ctx a0 offset");
_Static_assert(M68K_CTX_USP  == offsetof(m68k_user_ctx_t, usp),  "m68k ctx usp offset");
_Static_assert(M68K_CTX_PC   == offsetof(m68k_user_ctx_t, pc),   "m68k ctx pc offset");
_Static_assert(M68K_CTX_SR   == offsetof(m68k_user_ctx_t, sr),   "m68k ctx sr offset");
_Static_assert(M68K_CTX_SIZE ==   sizeof(m68k_user_ctx_t),       "m68k ctx size");
#endif
