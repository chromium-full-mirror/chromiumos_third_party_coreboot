/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/asm.h>

/*
 * Our entry point
 */
ENTRY(_entry)

	/* Save off the location of the coreboot tables */
	lla t0, cb_header_ptr
	sd a1, 0(t0)

	lla t0, boot_hartid
	sd a0, 0(t0)

	/* Setup new stack */
	lla t0, 1f
	ld sp, 0(t0)
	//mov sp, x1

	/* Let's rock. */
	j start_main

	ret
ENDPROC(_entry)

.section .entry, "ax", %progbits
.align 3
.global cb_header_ptr
cb_header_ptr:
.quad	0
.global boot_hartid
boot_hartid:
.quad	0
1:
.quad	_stack
