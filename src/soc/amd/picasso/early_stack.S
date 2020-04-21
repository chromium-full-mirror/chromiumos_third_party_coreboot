/*
 * This file is part of the coreboot project.
 *
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

#include <cpu/x86/lapic_def.h>

.code32
.align 4

.global s3_bsp_reentry
s3_bsp_reentry:
	/* Align the stack before reentry to C functions */
	movl	$(_eearlyram_stack), %esp
	andl	$0xfffffff0, %esp
	subl	$4, %esp

	rdtsc
	pushl	%edx		/* save timestamp */
	pushl	%eax
	pushl	$0		/* BIST is N/A */

	/* void hybrid_romstage_entry(uint32_t bist, uint64_t early_tsc); */
	call	soc_hybrid_romstage_entry
	/* Execution does not return here */
1:
	hlt
	jmp 1b

.macro is_ap
	movl	%ecx, %ebx
	movl	$LAPIC_BASE_MSR, %ecx
	rdmsr
	movl	%ebx, %ecx
	testl	$LAPIC_BASE_MSR_BOOTSTRAP_PROCESSOR, %eax
.endm

/* Provide 0-based nth AP stack number, based on initial LAPIC ID */
.macro map_ap_apic_to_stack
	/* Picasso is single-die, single-socket, etc.  Assume both threads
	 * may be enabled.  This could be made more elegant for more
	 * complex systems. */
	decl	%ebx
.endm

.globl soc_reassign_stack
soc_reassign_stack:
	is_ap
	jne	zero_stacks

	/* Adjust ESP to point to top of AP stacks then move again depending
	 * on the initial LAPIC ID. */
	subl	$CONFIG_EARLYRAM_BSP_STACK_SIZE, %esp

	movl	$0x1, %eax
	cpuid
	shrl	$24, %ebx
	map_ap_apic_to_stack

	movl	$CONFIG_EARLYRAM_AP_STACK_SIZE, %eax
	mull	%ebx
	subl	%eax, %esp
	movl	$CONFIG_EARLYRAM_AP_STACK_SIZE, %ecx

	jmp	zero_stacks
