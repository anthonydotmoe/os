#include "linkage.h"

.globl exc_vector_table

IMPORT(trap0_entry)
IMPORT(trap1_entry)
IMPORT(ExceptionHandler)

	.section .text
	.balign 8
exc_vector_table:
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	trap0_entry
	.long	trap1_entry
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
	.long	ExceptionHandler
