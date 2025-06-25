/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __RISCV64_ASM_H
#define __RISCV64_ASM_H

#define ALIGN .align 2

#define ENDPROC(name) \
	.type name, %function; \
	END(name)

#define ENTRY(name) \
	.section .text.name, "ax", %progbits; \
	.global name; \
	ALIGN; \
	name:

#define END(name) \
	.size name, .-name

#endif	/* __RISCV64_ASM_H */
