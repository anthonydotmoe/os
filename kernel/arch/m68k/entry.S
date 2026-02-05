#include "linkage.h"
//#include <ipc/ipc.h>

#include "arch/exception_offsets.h"
#include "arch/context_offsets.h"
 
	.section .text

/* `save_process_ctx` is a macro because we don't want another stack frame */
.macro	save_process_ctx_fmt0
	move.l	a0,-(sp)		// save a0
	move.l	4+EXC_HDR_SIZE(sp),a0	// load ctx ptr to a0

	// store regs directly into current proc ctx
	movem.l	d0-d7/a0-a6,M68K_CTX_GPR(a0)	// a0 will be wrong
	move.l	(sp)+,M68K_CTX_A0(a0)		// patch a0
	movec.l	usp,d0				// save usp
	move.l	d0,M68K_CTX_USP(a0)

	// store pc/sr from exception header
	move.l	EXC_HDR_PC_OFF(sp),M68K_CTX_PC(a0)
	move.w	EXC_HDR_SR_OFF(sp),M68K_CTX_SR(a0)

	// pop the exception frame
	addq.l	#8,sp
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
	save_process_ctx_fmt0
	bra	kernel_call_entry
SYM_CODE_END(trap0_entry)

SYM_CODE_START(trap1_entry)
	save_process_ctx_fmt0
	bra	ipc_entry
SYM_CODE_END(trap1_entry)

/* For now, expect the `kernel_call` function to reach in to get the args */
SYM_CODE_START(kernel_call_entry)
	jsr	kernel_call		// arg 1 (proc ptr) already on stack
	addq.l	#4,sp			// pop the proc ctx
	bra	switch_to_user		// back to the kernel

SYM_CODE_END(kernel_call_entry)

/* For now, expect the `do_ipc` function to reach in to get the args */
SYM_CODE_START(ipc_entry)
	jsr	do_ipc			// arg 1 (proc ptr) already on stack
	addq.l	#4,sp			// pop the proc ctx
	bra	switch_to_user		// back to the kernel
SYM_CODE_END(ipc_entry)

/* ========================================================================== */
/* void __noreturn restore_user_context(m68k_user_ctx_t *ctx);                */
/* Restore user context and switch from kernel to userspace.                  */
/* ========================================================================== */
SYM_FUNC_START(restore_user_context)
	move.l	4(sp),a0		// a0 = ctx
	addq.l	#4,sp			// discard return address (noreturn)

	// build the exception frame first, then restore registers
	move.w	M68K_CTX_SR(a0),d0
	move.l	M68K_CTX_PC(a0),d1
	clr.w	-(sp)			// push dummy fmtvec
	move.l	d1,-(sp)		// push pc
	move.w	d0,-(sp)		// push sr

	move.l	M68K_CTX_USP(a0),d0	// restore user stack pointer
	movec.l	d0,usp

	movem.l	M68K_CTX_GPR(a0),d0-d7/a0-a6
	rte
SYM_FUNC_END(restore_user_context)
