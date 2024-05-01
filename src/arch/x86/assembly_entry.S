/* SPDX-License-Identifier: GPL-2.0-only */

#include <rules.h>

/*
 * This path is for stages that are post bootblock. The gdt is reloaded
 * to accommodate platforms that are executing out of CAR. In order to
 * continue with C code execution one needs to set stack pointer and
 * clear .bss variables that are stage specific.
 */

#if CONFIG(RESET_VECTOR_IN_RAM)
	#define _STACK_TOP _eearlyram_stack
#else
	#define _STACK_TOP _ecar_stack
#endif

#ifdef __x86_64__
.code64
#else
.code32
#endif

.section ".text._start", "ax", @progbits
.global _start
_start:

	/* Migrate GDT to this text segment */
#ifdef __x86_64__
	call	gdt_init64
#else
	call	gdt_init
#endif

#if ENV_ROMSTAGE
	/*
	 * DEDEDE-HACK: Relocate persistent memory regions that moved between RO
	 * and RW stages. We should do this before initializing the stack pointer
	 * so that we don't need to worry about the case that the new _ecar_stack
	 * might overlap some of the data we want to save from the old layout,
	 * because otherwise it might get overwritten before we can copy it.
	 *
	 * The difference between the two layouts is that the size of the stack
	 * increased (i.e. _ecar_stack moved to a higher address), and we know
	 * that no persistent regions exist after _bss, so relocate everything
	 * between _ecar_stack and _bss. On Dedede, this is the CBMEM console,
	 * the timestamps and the FMAP cache (as well as unused areas for pdpt
	 * and ehci_dbg_info).
	 */

	/* We memmove() stuff forward, so need to copy backward */
	std

	movl	$(_bss), %ecx

	/* EDI = _bss(RW) - 4 (first destination word, from the back) */
	leal	-4(%ecx), %edi

	/* ESI = EDI - RW + RO = _bss(RO) - 4 (first source word, from the back) */
	leal	-CONFIG_DCACHE_BSP_STACK_SIZE+CONFIG_DCACHE_RO_STACK_SIZE(%edi), %esi

	/* ECX = (_bss - _ecar_stack) / 4 */
	movl	$(_ecar_stack), %eax
	sub	%eax, %ecx
	shr	$2, %ecx

	rep	movsl
#endif

	/* reset stack pointer to CAR/EARLYRAM stack */
	mov	$_STACK_TOP, %esp

	/* clear .bss section as it is not shared */
	cld
	xor	%eax, %eax
	movl	$(_ebss), %ecx
	movl	$(_bss), %edi
	sub	%edi, %ecx
	shrl	$2, %ecx
	rep	stosl

#if ((ENV_SEPARATE_VERSTAGE && CONFIG(VERSTAGE_DEBUG_SPINLOOP)) \
	|| (ENV_ROMSTAGE && CONFIG(ROMSTAGE_DEBUG_SPINLOOP)))

	/* Wait for a JTAG debugger to break in and set EBX non-zero */
	xor	%ebx, %ebx

debug_spinloop:
	cmp	$0, %ebx
	jz	debug_spinloop
#endif

	andl	$0xfffffff0, %esp
#if CONFIG(IDT_IN_EVERY_STAGE)
	call	exception_init
#endif

#if CONFIG(ASAN_IN_ROMSTAGE)
	call	asan_init
#endif
	call	car_stage_entry

	/* Expect to never return. */
1:
	jmp	1b
