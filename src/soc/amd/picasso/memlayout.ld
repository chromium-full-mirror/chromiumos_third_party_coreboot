/* SPDX-License-Identifier: GPL-2.0-only */

#if ENV_X86

#include <memlayout.h>
#include <arch/header.ld>

#define EARLY_MEMLAYOUT	"src/arch/x86/early_dram.ld"

SECTIONS
{
	/*
	 * It would be good to lay down RAMSTAGE, ROMSTAGE, etc consecutively
	 * like other architectures/chipsets it's not possible because of
	 * the linking games played during romstage creation by trying
	 * to find the final landing place in CBFS for XIP. Therefore,
	 * conditionalize with macros.
	 */
#if ENV_RAMSTAGE
	RAMSTAGE(CONFIG_RAMBASE, 8M)

#elif ENV_ROMSTAGE
	/* The 1M size is not allocated. It's just for basic size checking.
	 * Link at 32MiB address and rely on cbfstool to relocate to XIP. */
	ROMSTAGE(CONFIG_ROMSTAGE_ADDR, 1M)

	#include EARLY_MEMLAYOUT
#elif ENV_VERSTAGE
	/* The 1M size is not allocated. It's just for basic size checking.
	 * Link at 32MiB address and rely on cbfstool to relocate to XIP. */
	VERSTAGE(CONFIG_VERSTAGE_ADDR, 1M)

	#include EARLY_MEMLAYOUT
#elif ENV_BOOTBLOCK
	/* This is for C_ENVIRONMENT_BOOTBLOCK. arch/x86/bootblock.ld contains
	 * the logic for the romcc linking. */
	BOOTBLOCK(CONFIG_X86_RESET_VECTOR - CONFIG_C_ENV_BOOTBLOCK_SIZE + 0x10,
			CONFIG_C_ENV_BOOTBLOCK_SIZE)

	#include EARLY_MEMLAYOUT
#endif
}

#if ENV_BOOTBLOCK
/* Bootblock specific scripts which provide more SECTION directives. */
#include <cpu/x86/16bit/entry16.ld>
#include <cpu/x86/16bit/reset16.ld>
#include <arch/x86/id.ld>
#endif  /* ENV_BOOTBLOCK */

#endif /* ENV_X86 */

#if ENV_ARM
#include "psp_verstage/memlayout.ld"
#endif
