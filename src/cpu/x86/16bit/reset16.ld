/*
 * This file is part of the coreboot project.
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

/* _RESET_VECTOR: typically the top of the ROM */

SECTIONS {
#if (CONFIG(C_ENVIRONMENT_BOOTBLOCK) && CONFIG_C_ENV_BOOTBLOCK_SIZE > 0x10000)
	/* Trigger an error if I have an unuseable start address */
	_TOO_LOW = CONFIG_X86_RESET_VECTOR - 0xfff0;
	_bogus = ASSERT(_start16bit >= _TOO_LOW, "_start16bit too low. Please report.");

	/* .near_reset_vector positions code that must be reachable from the
	 * reset vector.  For a program size <= 64KB, this happens by default,
	 * however with > 64KB it is artificially enforced. */
	. = CONFIG_X86_RESET_VECTOR & 0xfffff000;
	.near_reset_vector . : {
		*(.near_reset_vector);
	}
#endif

	. = CONFIG_X86_RESET_VECTOR;
	.reset . : {
		*(.reset);
		. = 15;
		BYTE(0x00);
	}
}
