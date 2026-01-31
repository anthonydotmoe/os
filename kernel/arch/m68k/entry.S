#include <asm/linkage.h>
#include <ipc/ipc.h>

/* `save_process_ctx` is a macro because we don't want another stack frame */
.macro	save_process_ctx

.endm


SYM_CODE_START(ExceptionHandler)
	movem.l	d0-d7/a0-a6,-(sp)	// save regs (15 longs)

	move.l	sp,-(sp)		// arg0 = &saved_regs_t
	jsr	ExceptionHandler_c
	addq.l	#4,sp			// pop arg

	movem.l	(sp)+,d0-d7/a0-a6	// restore regs
	rte				// return from exception/trap/interrupt
SYM_CODE_END(ExceptionHandler)

/*
Calling convention for TRAP #0:
	D0 - Call number
	D1 - src/dst endpoint	(N/A for KERNEL)
	D2 - message pointer
	D3 - Service type	IPC or KERNEL
*/

SYM_CODE_START(trap0_entry)
	save_process_ctx
SYM_CODE_END(trap_entry)

/* ========================================================================== */
/* void __noreturn restore_user_context(proc *p);                             */
/* Restore user context and switch from kernel to userspace.                  */
/* ========================================================================== */
SYM_FUNC_START(restore_user_context)
	addq.l	#4,sp			// disregard the return address
					// now (sp) points to the proc
	
	movem.l	(sp),d0-d7/a0-a6	// restore registers
	move.l	(#60,sp),usp		// restore stack pointer
	clr.w	-(sp)			// push dummy fmtvec
	move.l	(#64,sp),-(sp)		// push pc
	move.w	(#72,sp),-(sp)		// push sr (64 + 4 (just pushed) + 2)
	rte
SYM_FUNC_END(restore_user_context)

