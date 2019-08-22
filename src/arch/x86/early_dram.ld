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

/* Force the early storage to immedately follow the 64KB code in order to ease
 * MTRR programming and memory reserving.
 */
. = CONFIG_ROMSTAGE_ADDR + 0x10000;
.earlyram.data . (NOLOAD) : {
	_earlyram_region_start = . ;

	_earlyram_stack_start = .;
	. += CONFIG_EARLYRAM_AP_STACK_SIZE * (CONFIG_MAX_CPUS - 1);
	. += CONFIG_EARLYRAM_BSP_STACK_SIZE;
	_earlyram_stack_end = .;

	PRERAM_CBMEM_CONSOLE(., CONFIG_PRERAM_CBMEM_CONSOLE_SIZE)

	TIMESTAMP(., 0x200)

	_earlyram_unallocated_start = .;

	_earlyram_region_end = . + CONFIG_EARLYRAM_SIZE - (. - _earlyram_region_start);
}

_bogus = ASSERT((CONFIG_EARLYRAM_SIZE == 0) || (SIZEOF(.earlyram.data) <= CONFIG_EARLYRAM_SIZE), "EARLYRAM area is too full");
_bogus2 = ASSERT(CONFIG_EARLYRAM_BSP_STACK_SIZE > 0x0, "BSP stack size not configured");

/* Implementation must fit within 64KB and _start16bit is guaranteed
 * to be reachable from the reset vector.
 */
_RESET_VECTOR = CONFIG_ROMSTAGE_ADDR + 0x10000 - 0x10;

. = _RESET_VECTOR;
.reset . : {
	*(.reset);
	. = 15;
	BYTE(0x00);
}

	gdtptr16_offset = gdtptr16 & 0xffff;
	nullidt_offset = nullidt & 0xffff;
	ap_sipi_vector_in_rom = (_start16bit >> 12) & 0xff;
