#include "asm/linkage.h"

SYM_CODE_START(ExceptionHandler)
	movem.l	d0-d7/a0-a6,-(sp)	// save regs (15 longs)

	move.l	sp,-(sp)		// arg0 = &saved_regs_t
	jsr	ExceptionHandler_c
	addq.l	#4,sp			// pop arg

	movem.l	(sp)+,d0-d7/a0-a6	// restore regs
	rte				// return from exception/trap/interrupt
SYM_CODE_END(ExceptionHandler)
