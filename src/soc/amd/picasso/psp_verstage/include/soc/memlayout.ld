/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#if ENV_VERSTAGE

/*
 * Start of available space is 0x15000 and this is where the
 * header for the user app (verstage) must be mapped.
 * Size is 0x28000 bytes
 */
#define SRAM_START			0x15000
#define SRAM_SIZE			160K

#define VERSTAGE_START			0x15000
#define VERSTAGE_SIZE			60K
#define VBOOT_WORK_START		0x24000
#define VBOOT_WORK_SIZE			12K

/*
 * The temp stack can be made much smaller if needed - even 256 bytes
 * should be sufficient.  This is just for the function mapping the
 * actual stack.
 */
#define PSP_VERSTAGE_TEMP_STACK_START	0x39000
#define PSP_VERSTAGE_TEMP_STACK_SIZE	4K

/*
 * The top of the stack must be 4k aligned, so set the bottom as 4k aligned
 * and make the size a multiple of 4k
 */
#define PSP_VERSTAGE_STACK_START	0x3B000
#define PSP_VERSTAGE_STACK_SIZE		8K

ENTRY(_psp_vs_start)
SECTIONS
{
	. = VERSTAGE_START;
	_sram = .;
	_verstage = .;

		.text : { *(PSP_HEADER_DATA) }
		.text : { *(.text._psp_vs_start) }
		.text : { *(.text.Main) }
		.text : { *(.text) }

		.data : { *(.data) }
		.data : { *(PSP_FOOTER_DATA) }

		.bss : { *(.bss*) }

	_everstage = _verstage + VERSTAGE_SIZE;
	. = VBOOT_WORK_START;
	_vboot2_work = .;
	_evboot2_work = VBOOT_WORK_START + VBOOT_WORK_SIZE;

	PSP_VERSTAGE_TEMP_STACK_END = (PSP_VERSTAGE_TEMP_STACK_START + PSP_VERSTAGE_TEMP_STACK_SIZE );

	_stack = PSP_VERSTAGE_STACK_START;
	PSP_VERSTAGE_STACK_BASE = _stack;
	_estack = PSP_VERSTAGE_STACK_START + PSP_VERSTAGE_STACK_SIZE;
	PSP_VERSTAGE_STACK_END = _estack;
	_esram = SRAM_START + SRAM_SIZE;
}

#endif
