#include <asm/linkage.h>

	.section .text
	.balign 8

.equ	FC_USER_DATA,1

/* Exported symbols */
.globl __copy_msg_from_user_begin
.globl __copy_msg_from_user_end
.globl __copy_msg_to_user_begin
.globl __copy_msg_to_user_end

/* ========================================================================== */
/* int copy_message_from_user(const void* user_mbuf, void* dst_kbuf);         */
/* Copies 64 bytes from user space to kernel buffer.                          */
/* Returns 0 on success, -1 on fault  (bad user pointer/protection)           */
/* ========================================================================== */
SYM_FUNC_START(copy_message_from_user)
		/* load args: user_mbuf in a0, dst_kbuf in a1 */
		move.l	4(sp),a0
		move.l	8(sp),a1

		/* save callee-saved regs we will use */
		movem.l	d2-d7/a2,-(sp)

		/* Set source function code */
		moveq	#FC_USER_DATA,d2
		movec	d2,sfc

__copy_msg_from_user_begin:
		/* Unrolled 64-byte copy: 16 longwords */

		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+

		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+

		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+

		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+

__copy_msg_from_user_end:
		movem.l	(sp)+,d2-d7/a2
		moveq	#0,d0
		rts

SYM_FUNC_END(copy_message_from_user)

/* ========================================================================== */
/* int copy_message_to_user(const void* src_kbuf, void* user_mbuf);           */
/* Copies 64 bytes from kernel buffer to user space.                          */
/* Returns 0 on success, -1 on fault.                                         */
/* ========================================================================== */
SYM_FUNC_START(copy_message_to_user)
		/* load args: src_kbuf in a0, user_mbuf in a1 */
		move.l	4(sp),a0
		move.l	8(sp),a1

		movem.l	d2-d7/a2,-(sp)

		moveq	#FC_USER_DATA,d2
		movec	d2,dfc

__copy_msg_to_user_begin:
		/* Unrolled 64-byte copy: 16 longwords */

		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+

		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+

		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+

		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+
		moves.l	(a0)+,d0
		move.l	d0,(a1)+

__copy_msg_to_user_end:
		movem.l	(sp)+,d2-d7/a2
		moveq	#0,d0
		rts
SYM_FUNC_END(copy_message_to_user)

/* ========================================================================== */
/* Failure trampoline                                                         */
/* Exception handler rewrites saved PC to here when a fault happens inside    */
/* either copy routine.                                                       */
/* Must match the register-save discipline of the copy routines.              */
/* ========================================================================== */
SYM_CODE_START(__user_copy_msg_pointer_failure)
		movem.l	(sp)+,d2-d7/a2
		moveq	#1,d0
		rts
SYM_CODE_END(__user_copy_msg_pointer_failure)
