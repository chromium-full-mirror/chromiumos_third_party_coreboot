/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2000, Ron Minnich rminnich@lanl.gov
 *                     Advanced Computing Lab, LANL
 * Copyright (C) 2019 Advanced Micro Devices, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <arch/x86/prologue.inc>
#include <cpu/x86/16bit/entry16.inc>
#include <cpu/x86/16bit/reset16.inc>
#include <cpu/x86/32bit/entry32.inc>

	movl	%cr0, %eax
	andl	$0x9fffffff, %eax /* CD, NW = 0 since we're already in DRAM */
	movl	%eax, %cr0

	cld

	movl	$(_earlyram_stack_end), %esp
	movl	$CONFIG_EARLYRAM_BSP_STACK_SIZE, %ecx

	/* Send execution to the soc directory to allow assignment of a
	 * different stack location, e.g. if an AP is running.  soc should
	 * move ESP, and load ECX with the thread's stack size if it
	 * decides a change is required.
	 */
	jmp	soc_reassign_stack /* expect returning jmp to zero_stacks */

.globl zero_stacks
zero_stacks:
	movl	%esp, %edi
	subl	%ecx, %edi
	shrl	$2, %ecx
	xorl	%eax, %eax
	rep	stosl

	/* Align the stack, but use caution if adding arguments for the function
	 * below.  The stack must be a 16-byte aligned at the call instruction.
	 */
	andl	$0xfffffff0, %esp
	subl	$4, %esp

	rdtsc
	push	%edx		/* save timestamp */
	push	%eax
	push	%ebp		/* and BIST */

	/* void hybrid_romstage_entry(uint32_t bist, uint64_t early_tsc); */
	call	soc_hybrid_romstage_entry

/* This is here for linking purposes. */
.weak soc_hybrid_romstage_entry
soc_hybrid_romstage_entry:
1:
	jmp	1b
