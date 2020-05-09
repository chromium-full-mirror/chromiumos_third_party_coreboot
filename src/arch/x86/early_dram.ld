/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2006 Advanced Micro Devices, Inc.
 * Copyright (C) 2008-2010 coresystems GmbH
 * Copyright 2015 Google Inc
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

	/* This file is included inside a SECTIONS block */

	/* Enforce consistent positions for PRERAM_CBMEM_CONSOLE and TIMESTAMP
	 * regions by placing them near the top of the first stage's program
	 * region.  The stack and .bss are reused and do not rely on cross-stage
	 * access, however they are sized and positioned to prevent overwriting
	 * a previous stage's program in memory.
	 *
	 * Also enforce a consistent location for the vboot workbuf, shared
	 * out of the PSP.  In this case, also place it at a predictable
	 * location so that its address may be passed to amdfwtool.
	 */
	_WORKBUF_SIZE = CONFIG_PSP_SHAREDMEM_SIZE;
	_STACK_SIZE = CONFIG_EARLYRAM_BSP_STACK_SIZE;
	_CONSOLE_SIZE = CONFIG_PRERAM_CBMEM_CONSOLE_SIZE;
	_TIMESTAMPS_SIZE = 0x200;
	_BSS_SIZE = CONFIG_RAM_RESET_VECTOR_STAGE_BSS_SIZE;
	_TOP_4KB = 0x1000; /* regardless if allocated at .near_reset_vector */

	_bogus1 = ASSERT(_STACK_SIZE > 0x0, "Early BSP stack size not configured");

	. = CONFIG_X86_RESET_VECTOR + 0x10 - _STACK_SIZE - _CONSOLE_SIZE - _TIMESTAMPS_SIZE - _BSS_SIZE - _WORKBUF_SIZE - _TOP_4KB;
	EARLYRAM_STACK(., _STACK_SIZE)
	PRERAM_CBMEM_CONSOLE(., _CONSOLE_SIZE)
	TIMESTAMP(., _TIMESTAMPS_SIZE)
	.bss . : {
		. = ALIGN(ARCH_POINTER_ALIGN_SIZE);
		_bss = .;
		*(.bss)
		*(.bss.*)
		*(.sbss)
		*(.sbss.*)
		. = ALIGN(ARCH_POINTER_ALIGN_SIZE);
		_ebss = .;
	}

. = CONFIG_X86_RESET_VECTOR - _WORKBUF_SIZE - _TOP_4KB + 16;
	VBOOT2_PSP_SHARED_WORK(., _WORKBUF_SIZE)

/* Implementation must fit within 64KB and _start16bit is guaranteed
 * to be reachable from the reset vector.
 */
. = CONFIG_X86_RESET_VECTOR & 0xfffff000;
.near_reset_vector . : {
	*(.near_reset_vector);
}

. = CONFIG_X86_RESET_VECTOR;
.reset . : {
	*(.reset);
	. = 15;
	BYTE(0x00);
}

	gdtptr16_offset = gdtptr16 & 0xffff;
	nullidt_offset = nullidt & 0xffff;
	ap_sipi_vector_in_rom = (_start16bit >> 12) & 0xff;
