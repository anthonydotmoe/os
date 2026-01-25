#include "pgtable.h"
#include "asm/linkage.h"
#include "asm/bootinfo.h"
#include "asm/bootinfo-a68040pc.h"

/*
 *  Startup code
 * --------------------------------------------------------------------------
 */
	/*  It needs to be position independent from here until the kernel */
	/*  is mapped in at its virtual address. */
	/*  */

.globl kernel_pg_dir
.globl phys_kernel_start
.globl availmem
.globl init_mapped_size
.globl cachemode_pgtable
.globl cachemode_supervisor

/* Translation control register */
TC_ENABLE = 0x8000
TC_PAGE8K = 0x4000
TC_PAGE4K = 0x0000

/* Transparent translation registers */
TTR_ENABLE	= 0x8000	/* enable transparent translation */
TTR_ANYMODE	= 0x4000	/* user and kernel mode access */
TTR_KERNELMODE	= 0x2000	/* only kernel mode access */
TTR_USERMODE	= 0x0000	/* only user mode access */
TTR_CI		= 0x0400	/* inhibit cache */
TTR_RW		= 0x0200	/* read/write mode */
TTR_RWM		= 0x0100	/* read/write mask */
TTR_FCB2	= 0x0040	/* function code base bit 2 */
TTR_FCB1	= 0x0020	/* function code base bit 1 */
TTR_FCB0	= 0x0010	/* function code base bit 0 */
TTR_FCM2	= 0x0004	/* function code mask bit 2 */
TTR_FCM1	= 0x0002	/* function code mask bit 1 */
TTR_FCM0	= 0x0001	/* function code mask bit 0 */

/* Cache Control registers */
CC6_ENABLE_D	= 0x80000000	/* enable data cache (680[46]0) */
CC6_FREEZE_D	= 0x40000000	/* freeze data cache (68060) */
CC6_ENABLE_SB	= 0x20000000	/* enable store buffer (68060) */
CC6_PUSH_DPI	= 0x10000000	/* disable CPUSH invalidation (68060) */
CC6_HALF_D	= 0x08000000	/* half-cache mode for data cache (68060) */
CC6_ENABLE_B	= 0x00800000	/* enable branch cache (68060) */
CC6_CLRA_B	= 0x00400000	/* clear all entries in branch cache (68060) */
CC6_CLRU_B	= 0x00200000	/* clear user entries in branch cache (68060) */
CC6_ENABLE_I	= 0x00008000	/* enable instruction cache (680[46]0) */
CC6_FREEZE_I	= 0x00004000	/* freeze instruction cache (68060) */
CC6_HALF_I	= 0x00002000	/* half-cache mode for instruction cache (68060) */
CC3_ALLOC_WRITE	= 0x00002000	/* write allocate mode(68030) */
CC3_ENABLE_DB	= 0x00001000	/* enable data burst (68030) */
CC3_CLR_D	= 0x00000800	/* clear data cache (68030) */
CC3_CLRE_D	= 0x00000400	/* clear entry in data cache (68030) */
CC3_FREEZE_D	= 0x00000200	/* freeze data cache (68030) */
CC3_ENABLE_D	= 0x00000100	/* enable data cache (68030) */
CC3_ENABLE_IB	= 0x00000010	/* enable instruction burst (68030) */
CC3_CLR_I	= 0x00000008	/* clear instruction cache (68030) */
CC3_CLRE_I	= 0x00000004	/* clear entry in instruction cache (68030) */
CC3_FREEZE_I	= 0x00000002	/* freeze instruction cache (68030) */
CC3_ENABLE_I	= 0x00000001	/* enable instruction cache (68030) */

/* Miscellaneous definitions */
PAGESIZE	= 4096

ROOT_TABLE_SIZE  = 128	// 128 u32 root-level table descriptors
PTR_TABLE_SIZE   = 128	// 128 u32 pointer-level table descriptors
PAGE_TABLE_SIZE  = 64	// 64  u32 page descriptors
ROOT_INDEX_SHIFT = 25
PTR_INDEX_SHIFT  = 18
PAGE_INDEX_SHIFT = 12

#ifdef DEBUG
 /* When debugging use readable names for labels */
 #ifdef __STDC__
  #define L(name) .head.S.##name
 #else
  #define L(name) .head.S./**/name
 #endif
#else
 #ifdef __STDC__
  #define L(name) .L##name
 #else
  #define L(name) .L/**/name
 #endif
#endif

/* Macros to make the writing of subroutines easier:
 *
 * - func_start marks the beginning of the routine which sets up
 *   the frame register and saves the registers. It also defines
 *   another macro that automicatically restores the registers.
 *
 * - func_return marks the end of the routine and simply calls
 *   the prepared macro to restore registers and jump back to
 *   the caller.
 *
 * - func_define generates another macro to automatically put
 *   arguments onto the stack, call the subroutine,
 *   and clean up the stack again.
 *
 * Within subroutines, these macros can be used to access the
 * arguments on the stack. With STACK, some allocated memory
 * on the stack can be accessed, and ARG0 points to the return
 * address (Used by mmu_engage).
 */
#define STACK	stackstart(a6)
#define ARG0	4(a6)
#define ARG1	8(a6)
#define ARG2	12(a6)
#define ARG3	16(a6)
#define ARG4	20(a6)

.macro	func_start	name,saveregs,stack=0
L(\name):
	link.w	a6,#-\stack
	movem.l	\saveregs,-(sp)
.set	stackstart,-\stack

.macro	func_return_\name
	movem.l	(sp)+,\saveregs
	unlk	a6
	rts
.endm
.endm

.macro	func_return	name
	func_return_\name
.endm

.macro	func_call	name
	bsr	L(\name)
.endm

.macro	move_stack	nr,arg1,arg2,arg3,arg4
.if \nr
	move_stack	"(\nr-1)",\arg2,\arg3,\arg4
	move.l	\arg1,-(sp)
.endif
.endm

.macro	func_define	name,nr=0
.macro	\name		arg1,arg2,arg3,arg4
	move_stack	\nr,\arg1,\arg2,\arg3,\arg4
	func_call	\name
.if \nr
	lea	\nr*4(sp),sp
.endif
.endm
.endm

/* Function definitions */

func_define	mmu_map,4
func_define	mmu_map_tt,4
func_define	mmu_fixup_page_mmu_cache,1
func_define	mmu_temp_map,2
func_define	mmu_engage
func_define	mmu_get_root_table_entry,1
func_define	mmu_get_ptr_table_entry,2
func_define	mmu_get_page_table_entry,2
func_define	mmu_print
func_define	get_new_page

.macro	mmu_map_eq	arg1,arg2,arg3
	mmu_map	\arg1,\arg1,\arg2,\arg3
.endm

.macro	get_bi_record	record
	pea	\record
	func_call	get_bi_record
	addql	#4,%sp
.endm

func_define	serial_putc,1

.macro	putc	ch
	pea	\ch
	func_call serial_putc
	addq.l	#4,sp
.endm

func_define putn,1

.macro	puts	string
	.section .data
.Lstr\@:
	.string "\string"
	.previous
	pea	.Lstr\@(pc)
	func_call puts
	addq.l	#4,sp
.endm

.macro	dputc	ch
#ifdef DEBUG
	putc	\ch
#endif
.endm
.macro	dputn	nr
#ifdef DEBUG
	putn	\nr
#endif
.endm
.macro	dputs	string
#ifdef DEBUG
	puts	"\string"
#endif
.endm


/* Start of text area ------------------------------------------------------- */
	.section .head.text
SYM_CODE_START(_stext)
		bra	__start	/*  Jump over first page area */

		/*  */
		/*  */
		/*  There's room here for activities */
		/*  */
		/*  */

.equ	kernel_pg_dir,_stext
.equ	.,_stext+PAGESIZE	/*  PC = _stext + PAGESIZE */

SYM_CODE_END(_stext)

SYM_CODE_START(_start)
		bra	__start
SYM_CODE_END(_start)

/* Init text area ----------------------------------------------------------- */
	.section .init.text

/* -------------------------------------------------------------------------------------------------------- */
SYM_CODE_START(__start)
		lea	_stext(pc),sp	/*  Setup initial stack pointer */

		/*  Get the CPU & machine type */
		get_bi_record	BI_MACHTYPE
		lea	bi_machtype(pc),a1
		move.l	(a0),(a1)

		get_bi_record	BI_FPUTYPE
		lea	bi_fputype(pc),a1
		move.l	(a0),(a1)

		get_bi_record	BI_MMUTYPE
		lea	bi_mmutype(pc),a1
		move.l	(a0),(a1)

		get_bi_record	BI_CPUTYPE
		lea	bi_cputype(pc),a1
		move.l	(a0),(a1)

		/*  If 68040pc, stash the DUART address from bootinfo */
		cmp.l	#MACH_A68040PC,bi_machtype(pc)
		bne	L(test_nota68040pc)
		get_bi_record	BI_A68040PC_DUARTBASE
		lea	L(duartbase)(pc),a1
		move.l	(a0),(a1)
L(test_nota68040pc):

		/* TODO: Create a routine that initialized the serial port
		 * instead of assuming it is ready */
		/*  beq	serialinit */
		/*  --- Printing should work now */

		/*  Compute physical kernel start */
		lea	phys_kernel_start(pc),a0
		lea	_stext(pc),a1
		sub.l	#_stext,a1
		add.l	#PAGE_OFFSET,a1
		move.l	a1,(a0)

		/*  Welcome! and print debug information */
		dputs	"\nWelcome to the kernel!\n"

		dputs	"The kernel is located at:"
		dputn	a1
		dputc	'\n'

L(configure_cachemodes):

		/*  Determine the cache mode for pages holding MMU tables */
		/*  and for supervisor mode, unused for '020 and '030 */
		clr.l	d0
		clr.l	d1
		move.w	#_PAGE_CACHE,d0
		move.w	#_PAGE_CACHEW,d1

		/*  save cache mode for supervisor mode and page tables */
		lea	cachemode_supervisor(pc),a0
		move.l	d0,(a0)
		lea	cachemode_pgtable(pc),a0
		move.l	d1,(a0)

		/*  Raise interrupt level */
		move.w	#0x2700,sr

L(mmu_init):
		/*  */
		/*  mmu_init */
		/*  This block of code does what's necessary to map in the */
		/*  various kinds of machines for execution of the kernel. */
		/*  */
		/*  First, map the first 4, 8, or 16MB of kernel code & data */

		get_bi_record	BI_MEMCHUNK	/*  get first MEMCHUNK */
		move.l	4(a0),d0		/*  get its size */
		move.l	#16*1024*1024,d1	/*  start with 16MB */
		cmp.l	d0,d1			/*  compare MEMCHUNK size */
		bls	1f			/*  if 16MB < MEMCHUNK: done */
		lsr.l	#1,d1			/*  else try 8MB */
		cmp.l	d0,d1			/*  compare MEMCHUNK size */
		bls	1f			/*  if 8MB < MEMCHUNK: done */
		lsr.l	#1,d1			/*  else we'll map 4MB */
		lea	init_mapped_size(pc),a0
		move.l	d1,(a0)

		/*  create mapping for kernel code & data */
		mmu_map	#PAGE_OFFSET,phys_kernel_start(pc),d1,cachemode_supervisor(pc)

		/*  Map the DUART with transparent translation */
		mmu_map_tt	#1,L(duartbase)(pc),#0x10000000,#_PAGE_NOCACHE_S
		bra	L(mmu_init_done)

L(mmu_init_done):

		dputs	"\nMMU Setup is complete\n"

#ifdef DEBUG
		//mmu_print L(kernel_pgdir_ptr)(pc)
#endif

L(mmu_fixup):
#ifdef MMU_NOCACHE_KERNEL
		jbra	L(mmu_fixup_done)
#endif
		/*  first fix the page at the start of the kernel, that */
		/*  contains also kernel_pg_dir */
		move.l	phys_kernel_start(pc),d0
		sub.l	#PAGE_OFFSET,d0
		lea	_stext(pc),a0
		sub.l	d0,a0
		mmu_fixup_page_mmu_cache a0

		move.l	L(kernel_end)(pc),a0
		sub.l	d0,a0
		move.l	L(memory_start)(pc),a1
		sub.l	d0,a1
		bra	2f
1:
		mmu_fixup_page_mmu_cache a0
		add.w	#PAGESIZE,a0
2:
		cmp.l	a0,a1
		bgt	1b
L(mmu_fixup_done):

		//mmu_print L(kernel_pgdir_ptr)(pc)

L(begin_vm):
		dputs	"\nEngaging the MMU...\n"
		mmu_engage
		dputs	" Done!\n"


		/*  After this point no new memory is allocated and */
		/*  the start of available memory is stored in availmem. */
		/*  (The bootmem allocator requires now the physical address.) */
		move.l	L(memory_start),availmem

L(enable_caches):
		dputs	"Enabling caches\n"

		nop
		cpusha	bc
		nop

L(begin_init):
		/* lea	init_task,a2 */
		/* lea	init_thread_union+THREAD_SIZE,sp */

		jmp	start_kernel

L(die):		stop	#0x2000
		bra	L(die)

SYM_CODE_END(__start)


/* ------------------------------------------------------------------------------- */
		
	
/*  boot info offsets */
.equ		BIR_TAG,0
.equ		BIR_SIZE,2
.equ		BIR_DATA,4

/*  Find a tag record in the bootinfo structure */
/*  The bootinfo structure is located right after the kernel */
/*  Returns: d0: size (-1 if not found) */
/*           a0: data pointer (end-of-records if not found) */
func_start	get_bi_record,d1

		move.l	ARG1,d0	/*  get tag arg */
		lea	_end(pc),a0	/*  find bootinfo structure */
1:		tst.w	BIR_TAG(a0)	/*  is this the last tag? */
		beq	3f		/*  if not */
		cmp.w	BIR_TAG(a0),d0	/*  does the tag match? */
		beq	2f		/*  if not */
		add.w	BIR_SIZE(a0),a0	/*  index past the record */
		bra	1b		/*  try again */
2:		moveq	#0,d0		/*  clear d0 */
		move.w	BIR_SIZE(a0),d0	/*  get data size */
		lea	BIR_DATA(a0),a0	/*  get data pointer */
		bra	4f		/*  return */
3:		moveq	#-1,d0		/*  not found */
		lea	BIR_SIZE(a0),a0	/*  return end-of-records */
4:
func_return	get_bi_record

func_start	puts,d0/a0
		move.l	ARG1,a0	/*  get string arg */
		bra	2f
1:		serial_putc	d0
2:		move.b	(a0)+,d0
		bne	1b
func_return	puts

	/* 	putn */
	/* 	Output a number in hex notation with leading space */
func_start	putn,d0-d2
		putc	' '

		move.l	ARG1,d0	/*  get number arg */
		moveq	#7,d1		/*  count = 7 (8 + -1 digits) */
1:		rol.l	#4,d0		/*  rotate */
		move	d0,d2		/*  move next digit into d2 */
		and.b	#0x0F,d2	/*  mask d2 to one nyb */
		add.b	#'0',d2		/*  + ASCII '0' */
		cmp.b	#'9',d2		/*  is it greater than '9'? */
		bls	2f		/*  if not, print it */
		add.b	#'A'-('9'+1),d2	/*  if so, make it 'A'-'F' */
2:		serial_putc	d2	/*  print it */
		dbra	d1,1b		/*  while count != -1 */
func_return	putn


func_start	serial_putc,d0/a0

.equ	SRA,1
.equ	TBA,3

		move.l	ARG1,d0	/*  get char */
		cmpi.b	#'\n',d0	/*  is newline? */
		bne	1f

		serial_putc	#'\r'
1:
		move.l	L(duartbase)(pc),a0	/* get duartbase from var */
		tst.l	a0			/* If it is zero */
		beq	3f			/* Don't send a character */
2:		btst.b	#2,SRA(a0)		/* Check if SR[2] is zero */
		beq	2b
		move.b	d0,TBA(a0)		/* Move char to transmit buffer */
3:
func_return	serial_putc

	/* 	mmu_map_tt */
	/*  */
	/* 	This function will work on all 680x0 machines. */
	/* 	On 030, 040 & 060 it will attempt to use Transparent Translation */
	/* 	Registers (tt1). */
	/* 	On 020 it will call the standard mmu_map which will use early */
	/* 	terminating descriptors */
func_start	mmu_map_tt,d0-d1/a0

		dputs	"\nmmu_map_tt()"
		dputs	"\n    ttr_num:"
		dputn	ARG1
		dputs	"\n  base_addr:"
		dputn	ARG2
		dputs	"\n   map_size:"
		dputn	ARG3
		dputs	"\n      flags:"
		dputn	ARG4
		dputc	'\n'

		/*  Extract the highest bit set */
		bfffo	ARG3{0:32},d1
		cmp.w	#8,d1		/*  if less than 8 bits of size */
		bcc	L(do_map)	/*  map it normally */

		/*  And get the mask */
		moveq	#-1,d0
		lsr.l	d1,d0
		lsr.l	#1,d0

		/*  Mask the address */
		move.l	d0,d1
		not.l	d1
		and.l	ARG2,d1

		/*  Generate the upper 16 bits of the TT register */
		lsr.l	#8,d0
		or.l	d0,d1
		clr.w	d1

		or.w	#TTR_ENABLE+TTR_KERNELMODE,d1
		or.l	ARG4,d1
		
		dputs	"          ---\n"
		dputs	"        TTR:"
		dputn	d1
		dputc	'\n'

		tst.l	ARG1
		bne	1f
		movec	d1,itt0
		movec	d1,dtt0
		bra	2f
1:
		movec	d1,itt1
		movec	d1,dtt1
		bra	2f

L(do_map):
		mmu_map_eq	ARG2,ARG3,ARG4

2:
func_return	mmu_map_tt

	/* 	mmu_map */
	/*  */
	/* 	This routine will map a range of memory using a pointer */
	/* 	table and allocate the pages on the fly from the kernel. */
	/* 	The pointer table does not have to be already linked into */
	/* 	the root table, this routine will do that if necessary. */
	/*  */
	/* 	NOTE */
	/* 	This routine will assert failure and use the serial_putc */
	/* 	routines in the case of a run-time error. For example, */
	/* 	if the address is already mapped. */
func_start	mmu_map,d0-d4/a0-a4

		dputs	"\nmmu_map()"
		dputs	"\n  virt_base:"
		dputn	ARG1
		dputs	"\n  phys_base:"
		dputn	ARG2
		dputs	"\n       size:"
		dputn	ARG3
		dputs	"\n      flags:"
		dputn	ARG4
		dputc	'\n'

		/*  Get logical address and round it down to 256KB */
		move.l	ARG1,d0
		and.l	#-(PAGESIZE*PAGE_TABLE_SIZE),d0
		move.l	d0,a3

		/*  Get the end address */
		move.l	ARG1,a4
		add.l	ARG3,a4
		subq.l	#1,a4

		/*  Get physical address and round it down to 256KB */
		move.l	ARG2,d0
		and.l	#-(PAGESIZE*PAGE_TABLE_SIZE),d0
		move.l	d0,a2

		/*  Add page attributes to the physical address */
		move.l	ARG4,d0
		or.w	#_PAGE_PRESENT+_PAGE_ACCESSED+_PAGE_DIRTY,d0
		add.w	d0,a2

		dputs	"          ---"
		dputs	"\n  page_desc:"
		dputn	a2	/*  Completed page descriptor */
		dputs	"\n virt_start:"
		dputn	a3	/*  Logical start address */
		dputs	"\n virt_end  :"
		dputn	a4	/*  Logical end address */
		dputc	'\n'


		add.w	#_PAGE_GLOBAL,a2

		/*
		 *  MMU 040 & 060 Support
		 *
		 *  The 040 does not support early terminating descriptors, as
		 *  the 030 does. Therefore, a third level of table is needed
		 *  for the 040, and that would be the page table. In Linux,
		 *  page tables are allocated directly from the memory above
		 *  the kernel.
		 */

L(mmu_map_do_map):
		/*  Calculate the offset into the root table */
		move.l	a3,d0
		moveq	#ROOT_INDEX_SHIFT,d1
		lsr.l	d1,d0
		mmu_get_root_table_entry d0

		/*  Calculate the offset into the pointer table */
		move.l	a3,d0
		moveq	#PTR_INDEX_SHIFT,d1
		lsr.l	d1,d0
		and.l	#PTR_TABLE_SIZE-1,d0
		mmu_get_ptr_table_entry	a0,d0

		/*  Calculate the offset into the page table */
		move.l	a3,d0
		moveq	#PAGE_INDEX_SHIFT,d1
		lsr.l	d1,d0
		and.l	#PAGE_TABLE_SIZE-1,d0
		mmu_get_page_table_entry a0,d0

		/*  The page table entry must not be busy */
		tst.l	(a0)
		bne	L(mmu_map_error)

		/*  Do the mapping and advance the pointers */
		move.l	a2,(a0)
		add.w	#PAGESIZE,a2
		add.w	#PAGESIZE,a3

		/*  Ready with mapping? */
		lea	-1(a3),a0
		cmp.l	a0,a4
		bhi	L(mmu_map_do_map)
		bra	L(mmu_map_done)

L(mmu_map_error):
		dputs	"mmu_map error:"
		dputn	a2
		dputn	a3
		dputc	'\n'

L(mmu_map_done):
func_return	mmu_map

	/*  */
	/* 	mmu_fixup */
	/*  */
	/* 	On the 040 class machines, all pages that are used for the */
	/* 	MMU have to be fixed up. */
func_start	mmu_fixup_page_mmu_cache,d0/a0
		dputs	"mmu_fixup_page_mmu_cache()"
		dputs	"\n  page_addr:"
		dputn	ARG1
		dputc	'\n'

		/*  Calculate the offset into the root table */
		move.l	ARG1,d0
		moveq	#ROOT_INDEX_SHIFT,d1
		lsr.l	d1,d0
		mmu_get_root_table_entry d0

		/*  Calculate the offset into the pointer table */
		move.l	ARG1,d0
		moveq	#PTR_INDEX_SHIFT,d1
		lsr.l	d1,d0
		and.l	#PTR_TABLE_SIZE-1,d0
		mmu_get_ptr_table_entry a0,d0

		/*  Calculate the offset into the page table */
		move.l	ARG1,d0
		moveq	#PAGE_INDEX_SHIFT,d1
		lsr.l	d1,d0
		and.l	#PAGE_TABLE_SIZE-1,d0
		mmu_get_page_table_entry a0,d0

		move.l	(a0),d0
		andi.l	#_CACHEMASK040,d0
		or.l	cachemode_pgtable(pc),d0
		move.l	d0,(a0)

		dputc	'\n'
func_return	mmu_fixup_page_mmu_cache

	/*  */
	/* 	mmu_temp_map */
	/*  */
	/* 	create a temporary mapping to enable the MMU */
func_start	mmu_temp_map,d0-d1/a0-a1
		dputs	"\nmmu_temp_map()"
		dputs	"\n  phys_base:"
		dputn	ARG1
		dputs	"\n  virt_base:"
		dputn	ARG2
		dputs	"\n          ---\n"

		lea	L(temp_mmap_mem)(pc),a1

		/*  Calculate the offset in the root table */
		move.l	ARG2,d0
		moveq	#ROOT_INDEX_SHIFT,d1
		lsr.l	d1,d0
		mmu_get_root_table_entry d0

		/*  Check if the table is temporary allocated; */
		/*  if so, we have to reuse it */
		move.l	(a0),d0
		cmp.l	L(memory_start)(pc),d0
		bcc	1f

		/*  Temporary allocate a ptr table and */
		/*  insert it into the root table */
		move.l	(a1),d0
		add.l	#PTR_TABLE_SIZE*4,(a1)
		or.w	#_PAGE_TABLE+_PAGE_ACCESSED,d0
		move.l	d0,(a0)
		dputs	"  (new ptr):"
		dputn	d0
		dputc	'\n'
1:
		/*  Mask the root table entry for the ptr table */
		and.w	#-ROOT_TABLE_SIZE,d0
		move.l	d0,a0

		/*  Calculate the offset into the pointer table */
		move.l	ARG2,d0
		moveq	#PTR_INDEX_SHIFT,d1
		lsr.l	d1,d0
		and.l	#PTR_TABLE_SIZE-1,d0
		lea	(a0,d0.l*4),a0
		dputs	"    ptr off:"
		dputn	a0
		dputc	'\n'

		/*  Check if a temporary page table is already allocated */
		move.l	(a0),d0
		bne	2f

		/*  Temporary allocate a page table and */
		/*  insert it into the ptr table */
		move.l	(a1),d0
		/*  The 512 should be PAGE_TABLE_SIZE*4, but that violates */
		/*  the alignment restriction for pointer tables on the '0[46]0 */
		add.l	#512,(a1)
		or.w	#_PAGE_TABLE+_PAGE_ACCESSED,d0
		move.l	d0,(a0)
		dputs	" (new page):"
		dputn	d0
		dputc	'\n'
2:
		/*  Mask the ptr table entry for the page table */
		and.w	#-PTR_TABLE_SIZE,d0
		move.l	d0,a0

		/*  Calculate the offset into the page table */
		move.l	ARG2,d0
		moveq	#PAGE_INDEX_SHIFT,d1
		lsr.l	d1,d0
		and.l	#PAGE_TABLE_SIZE-1,d0
		lea	(a0,d0.l*4),a0
		dputs	"   page off:"
		dputn	a0
		dputc	'\n'

		/*  Insert the address into the page table */
		move.l	ARG1,d0
		and.w	#-PAGESIZE,d0
		or.w	#_PAGE_PRESENT+_PAGE_ACCESSED+_PAGE_DIRTY,d0
		move.l	d0,(a0)
		dputs	"   inserted:"
		dputn	d0
		dputc	'\n'
func_return	mmu_temp_map

	/*  */
	/* 	mmu_engage */
	/*  */
func_start	mmu_engage,d0-d2/a0-a3

		moveq	#ROOT_TABLE_SIZE-1,d0
		/*  Temporarily use a different root table. */
		lea	L(kernel_pgdir_ptr)(pc),a0
		move.l	(a0),a2
		move.l	L(memory_start)(pc),a1
		move.l	a1,(a0)
		move.l	a2,a0
1:
		move.l	(a0)+,(a1)+
		dbra	d0,1b

		lea	L(temp_mmap_mem)(pc),a0
		move.l	a1,(a0)

		move.w	#PAGESIZE-1,d0
2:
		clr.l	(a1)+
		dbra	d0,2b
		
		/*  Determine if temp mappings are required */
		lea	2b(pc),a0		/*  Load physical address of lab 2 */
		move.l	#2b,a1			/*  Load program address of lab 2 */
		cmp.l	a0,a1			/*  If equal: */
		beq	3f			/*  Skip to engaging the MMU */

		/*  Create temporary mappings */
		mmu_temp_map	a0,a0
		mmu_temp_map	a0,a1
		add.w	#PAGESIZE,a0	/*  +1 PAGESIZE */
		add.w	#PAGESIZE,a1
		mmu_temp_map	a0,a0
		mmu_temp_map	a0,a1
3:
		move.l	L(memory_start)(pc),a3
		move.l	phys_kernel_start(pc),d2

		/* Engage the 040 MMU */
		//mmu_print a3
L(mmu_decisive_moment):
		nop			/*  sync pipeline */
		cinva	bc		/*  invalidate both caches */
		nop			/*  sync pipeline */
		pflusha			/*  flush all ATC entries */
		nop			/*  sync pipeline */
		movec	a3,srp		/*  install supervisor root pointer */
		move.l	#TC_ENABLE+TC_PAGE4K,d0 /*  prepare TC reg */
		movec	d0,tc		/*  ENABLE THE MMU */
		jmp	4f.l		/*  long jump to synchronize prefetch */
4:		nop
		movec	a2,srp		/*  restore SRP to original root */
		nop			/*  sync pipeline */
		cinva	bc		/*  invalidate both caches */
		nop			/*  sync pipeline */
		pflusha			/*  flush all ATC entries */

		/* Cleanup */
						// D2 has start of physical memory
		sub.l	#PAGE_OFFSET,d2		// Subtract the load offset
		sub.l	d2,a2			// A2 has old pagedir ptr, adjust to logical
		move.l	a2,L(kernel_pgdir_ptr)	// Store fixed kernel_pgdir_ptr
		sub.l	d2,a6			// Fix frame pointer
		sub.l	d2,sp			// Fix stack pointer
		sub.l	d2,ARG0			// Fix return address
func_return	mmu_engage

	//	mmu_get_root_table_entry
	//
func_start	mmu_get_root_table_entry,d0/a1

		move.l	L(kernel_pgdir_ptr)(pc),a0	// Load root table pointer
		tst.l	a0				// if table pointer != NULL
		bne	2f				// continue with this pointer

		dputs	"\nmmu_init:"

		/*  Find the start of free memory, get_bi_record does this for us, */
		/*  as the bootinfo structure is located directly behind the kernel. */
		/*  We simply search for the last entry. */
		get_bi_record	BI_LAST
		add.w	#PAGESIZE-1,a0	// then find the next page
		move.l	a0,d0
		and.w	#-PAGESIZE,d0

		dputs	"\n  First free page of memory at:"
		dputn	d0
		dputc	'\n'

		lea	L(memory_start)(pc),a0	/* get memory_start */
		move.l	d0,(a0)			/* memory_start = first free page */
		lea	L(kernel_end)(pc),a0	/* get kernel_end */
		move.l	d0,(a0)			/* kernel_end = (same) first free page */

		/* Set up the root table to live at the first page (_stext),
		 * since the init code in <FILENAME> will expect the kernel page
		 * table root to be there. The rest of the page is used for
		 * more pointer tables by mmu_get_ptr_table_entry.
		 */
		lea	_stext(pc),a0				// physical addr _stext -> a0
		lea	L(mmu_ptr_table_alloc_cursor)(pc),a1	// pointer table cursor -> a1
		move.l	a0,(a1)					// *pointer table cursor = _stext
		add.l	#ROOT_TABLE_SIZE*4,(a1)			// pointer table cursor += sizeof(POINTER_TABLE)

		lea	L(mmu_num_pointer_tables)(pc),a1	// a1 <- num_pointer_tables
		addq.l	#1,(a1)					// num_pointer_tables++

		/*  Clear the entire page  */
		move.l	a0,a1				// _stext -> a1
		move.w	#PAGESIZE/4-1,d0		// count = PAGESIZE/sizeof(long)-1
1:
		clr.l	(a1)+				// do { *root_base = 0L; }
		dbra	d0,1b				// while (-1 != count--)

		lea	L(kernel_pgdir_ptr)(pc),a1	// kernel root pointer -> a1
		move.l	a0,(a1)				// *kernel root pointer = _stext

		dputs	"  The kernel pagetable root will be located at:"
		dputn	a0
		dputc	'\n'

2:
		move.l	ARG1,d0
		lea	(a0,d0.l*4),a0
func_return	mmu_get_root_table_entry

func_start	mmu_get_ptr_table_entry,d0/a1
		move.l	ARG1,a0
		move.l	(a0),d0
		bne	2f

		/*  Keep track of the number of pointer tables we use */
		dputs	"\nmmu_get_new_ptr_table:"
		lea	L(mmu_num_pointer_tables)(pc),a0
		move.l	(a0),d0
		addq.l	#1,d0

		/*  See if there is a free pointer table */
		/*  in our cache of pointer tables */
		lea	L(mmu_ptr_table_alloc_cursor)(pc),a1
		and.w	#7,d0
		bne	1f

		/*  Get a new pointer table page from above the kernel memory */
		get_new_page
		move.l	a0,(a1)
1:
		/*  There is an unused pointer table in our cache... use it */
		move.l	(a1),d0
		add.l	#PTR_TABLE_SIZE*4,(a1)

		dputn	d0
		dputc	'\n'

		/*  Insert the new pointer table into the root table */
		move.l	ARG1,a0
		or.w	#_PAGE_TABLE+_PAGE_ACCESSED,d0
		move.l	d0,(a0)
2:
		/*  Extract the pointer table entry */
		and.w	#-PTR_TABLE_SIZE,d0
		move.l	d0,a0
		move.l	ARG2,d0
		lea	(a0,d0.l*4),a0

func_return	mmu_get_ptr_table_entry

func_start	mmu_get_page_table_entry,d0/a1
		move.l	ARG1,a0
		move.l	(a0),d0
		bne	2f

		/*  If the page table entry doesn't exist, we allocate a complete */
		/*  new page and use it as one contiguous big page table which can */
		/*  cover 4MB of memory, nearly all mappings have that alignment. */
		get_new_page
		add.w	#_PAGE_TABLE+_PAGE_ACCESSED,a0

		/*  align pointer table entry for a page of page tables */
		move.l	ARG1,d0
		and.w	#-(PAGESIZE/PAGE_TABLE_SIZE),d0
		move.l	d0,a1

		/*  Insert the page tables into the pointer entries */
		moveq	#PAGESIZE/PAGE_TABLE_SIZE/4-1,d0
1:
		move.l	a0,(a1)+
		lea	PAGE_TABLE_SIZE*4(a0),a0
		dbra	d0,1b

		/*  Now we can get the initialized pointer table entry */
		move.l	ARG1,a0
		move.l	(a0),d0
2:
		/*  Extract the page table entry */
		and.w	#-PAGE_TABLE_SIZE,d0
		move.l	d0,a0
		move.l	ARG2,d0
		lea	(a0,d0.l*4),a0
func_return	mmu_get_page_table_entry

func_start	get_new_page,d0/a1
		dputs	"\nget_new_page:"

		/*  allocate the page and adjust L(memory_start) */
		lea	L(memory_start)(pc),a0
		move.l	(a0),a1
		add.l	#PAGESIZE,(a0)

		/*  clear the new page */
		move.l	a1,a0
		move.w	#PAGESIZE/4-1,d0
1:
		clr.l	(a1)+
		dbra	d0,1b

		dputn	a0
		dputc	'\n'
func_return	get_new_page

/* #ifdef MMU_PRINT */

	/* 	mmu_print */
	/* 	From Linux Kernel */
	/* 	This algorithm will print out the current MMU mappings. */
	/*  */
	/* 	Input: */
	/* 	ARG1 -	root table pointer */
	/* 		Everything else is calculated from this. */
func_start	mmu_print,a0-a6/d0-d7

#define mmu_next_valid		0
#define mmu_start_logical	4
#define mmu_next_logical	8
#define mmu_start_physical	12
#define mmu_next_physical	16

#define MMU_PRINT_INVALID	-1
#define MMU_PRINT_VALID		1
#define MMU_PRINT_UNINITED	0

#define putZc(z,n)	jbne 1f; putc z; jbra 2f; 1: putc n; 2:

		move.l	ARG1,a5
		lea	L(mmu_print_data)(pc),a0
		move.l	#MMU_PRINT_UNINITED,mmu_next_valid(a0)

		puts	"\n68040 MMU Report\n"
		puts	"Root Pointer:"
		putn	(a5)
		putc	'\n'
		
#ifdef __GORY_AND_DETAILED
		/*  The following if/endif block is a tight algorithm for dumping */
		/*  the 040 MMU Map in gory detail. It really isn't that practical */
		/*  unless the MMU Map algorithm appears to go awry and you need */
		/*  to debug it at the entry per entry level. */
		move.l	#ROOT_TABLE_SIZE,d5
/* #if 0 */
/* 		move.l	(a5)+,d7	; Burn an entry to skip the */
/* 		subq.l	#1,d5		; kernel mappings, they probably work. */
/* #endif */
1:
		tst.l	d5
		beq	L(mmu_print_done)
		subq	#1,d5
		move.l	(a5)+,d7
		btst	#1,d7
		beq	1b

2:
		putn	d7
		andi.l	#0xFFFFFE00,d7
		move.l	d7,a4
		move.l	#PTR_TABLE_SIZE,d4
		putc	' '
3:
		tst.l	d4
		beq	11f
		subq	#1,d4
		move.l	(a4)+,d7
		btst	#1,d7
		beq	3b
4:
		putn	d7
		andi.l	#0xFFFFFF00,d7
		move.l	d7,a3
		move.l	#PAGE_TABLE_SIZE,d3
5:		move.l	#8,d2
6:		tst.l	d3
		beq	31f
		subq	#1,d3
		move.l	(a3)+,d6
		btst	#0,d6
		beq	6b
7:		tst.l	d2
		beq	8f
		subq	#1,d2
		putc	' '
		bra	91f
8:		putc	'\n'
		move.l	#8+1+8+1+1,d2
9:		putc	' '
		dbra	d2,9b
		move.l	#7,d2
91:		putn	d6
		bra	6b

31:		putc	'\n'
		move.l	#8+1,d2
32:		putc	' '
		dbra	d2,32b
		bra	3b

11:		putc	'\n'
		bra	1b
#endif /* __GORY_AND_DETAILED */

		lea	kernel_pg_dir(pc),a5
		move.l	a5,a0	/*  a0 has the address of the root table ptr */
		move.l	#0x00000000,a4	/*  logical address */
		moveq.l	#0,d0
40:
		/*  Increment the logical address and preserve in d5 */
		move.l	a4,d5
		addi.l	#PAGESIZE<<13,d5
		move.l	(a0)+,d6
		btst	#1,d6
		bne	41f
		bsr	L(mmu_print_tuple_invalidate)
		bra	48f
41:
		move.l	#0,d1
		andi.l	#0xFFFFFE00,d6
		move.l	d6,a1
42:
		move.l	a4,d5
		addi.l	#PAGESIZE<<6,d5
		move.l	(a1)+,d6
		btst	#1,d6
		bne	43f
		bsr	L(mmu_print_tuple_invalidate)
		bra	47f
43:
		move.l	#0,d2
		andi.l	#0xFFFFFF00,d6
		move.l	d6,a2
44:
		move.l	a4,d5
		addi.l	#PAGESIZE,d5
		move.l	(a2)+,d6
		btst	#0,d6
		bne	45f
		bsr	L(mmu_print_tuple_invalidate)
		bra	46f
45:
		movem.l	d0-d1,-(sp)
		move.l	a4,d0
		move.l	d6,d1
		andi.l	#0xFFFFF4E0,d1
		bsr	L(mmu_print_tuple)
		movem.l	(sp)+,d0-d1
46:
		move.l	d5,a4
		addq	#1,d2
		cmpi.b	#64,d2
		bne	44b
47:
		move.l	d5,a4
		addq	#1,d1
		cmpi.b	#128,d1
		bne	42b
48:
		move.l	d5,a4		/*  move to the next logical address */
		addq	#1,d0
		cmpi.b	#128,d0
		bne	40b

		/* Print transparent translation register state */
.macro		mmu_print_ttr_state	ttr
		movec	\ttr,d0
		move.l	d0,d1
		andi.w	#0x8000,d1	/*  is it valid? */
		beq	1f		/*  no, bail out */

		move.l	d0,d1
		andi.l	#0xFF000000,d1	/*  get the address */
		putn	d1
		puts	" =="
		putn	d1

		move.l	d0,d6
		bsr	L(mmu_040_print_flags_tt)
1:
.endm

		mmu_print_ttr_state dtt1
		mmu_print_ttr_state dtt0

		bra	L(mmu_print_done)

L(mmu_040_print_flags):
		btst.l	#10,d6
		putZc(' ','G')	/*  global bit */
		btst.l	#7,d6
		putZc(' ','S')	/*  supervisor bit */
L(mmu_040_print_flags_tt):
		btst.l	#6,d6
		bne	3f
		putc	'C'
		btst.l	#5,d6
		putZc('w','c')	/*  write through or copy-back */
		bra	4f
3:
		putc	'N'
		btst.l	#5,d6
		putZc('s',' ')	/*  serialized or not */
4:
		rts
L(mmu_print_done):
		putc	'\n'
func_return	mmu_print

L(mmu_print_tuple_invalidate):
		movem.l	a0/d7,-(sp)
		lea	L(mmu_print_data)(pc),a0
		tst.l	mmu_next_valid(a0)
		bmi	L(mmu_print_tuple_invalidate_exit)

		move.l	#MMU_PRINT_INVALID,mmu_next_valid(a0)
		putc	'\n'
		putn	a4
		puts	" ##\n"
L(mmu_print_tuple_invalidate_exit):
		movem.l	(sp)+,a0/d7
		rts

L(mmu_print_tuple):
		movem.l	d0-d7/a0,-(sp)

		lea	L(mmu_print_data)(pc),a0

		tst.l	mmu_next_valid(a0)
		ble	L(mmu_print_tuple_print)

		cmp.l	mmu_next_physical(a0),d1
		beq	L(mmu_print_tuple_increment)

L(mmu_print_tuple_print):
	 	putc	'\n'
		putn	d0
		puts	" ->"
		putn	d1

		move.l	d1,d6
		bsr	L(mmu_040_print_flags)

L(mmu_print_tuple_record):
		move.l	#MMU_PRINT_VALID,mmu_next_valid(a0)
		move.l	d1,mmu_next_physical(a0)

L(mmu_print_tuple_increment):
		move.l	d5,d7
		sub.l	a4,d7
		add.l	d7,mmu_next_physical(a0)

L(mmu_print_tuple_exit):
		movem.l	(sp)+,d0-d7/a0
		rts

/* endif // MMU_PRINT */

/*  Initialization data */
/* ------------------------------------------------------------------------------- */
	.section	.init.data

init_mapped_size:
		.long	0	/*  */
phys_kernel_start:
		.long	0	/*  determined physical load address */

L(mmu_print_data):
		.long	0	/*  valid flag */
		.long	0	/*  start logical */
		.long	0	/*  next logical */
		.long	0	/*  start physical */
		.long	0	/*  next physical */

L(mmu_ptr_table_alloc_cursor):
		.long	0	/*  */
L(mmu_num_pointer_tables):
		.long	0	/*  */
L(kernel_end):
		.long	0	/*   */
L(memory_start):
		.long	0	/*   */
L(kernel_pgdir_ptr):
		.long	0	/*   */
L(temp_mmap_mem):
		.long	0	/*   */
L(duartbase):
		.long	0	/*  68040pc 68681 DUART base address */

/*  .data section */
/* ------------------------------------------------------------------------------- */
	.section	.data
			.align  4

availmem:
		.long	0	/*  (global) available memory */
cachemode_pgtable:
		.long	0	/*  (global) cache mode for pagetable pages */
cachemode_supervisor:
		.long	0	/*  (global) cache mode for supervisor pages */
