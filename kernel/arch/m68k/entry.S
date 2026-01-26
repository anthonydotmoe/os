#include "asm/linkage.h"

SYM_CODE_START(ExceptionHandler)
	movem.l	d0-d7/a0-a6,-(sp)
	move.l	sp,-(sp)	// Push address to stack frame
	jsr	ExceptionHandler_c
	stop	#0x2700
SYM_CODE_END(ExceptionHandler)
