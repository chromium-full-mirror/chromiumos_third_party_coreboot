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

#include <console/console.h>
#include <cpu/x86/msr.h>
#include <amdblocks/psp.h>

#define PSP_MAILBOX_OFFSET		0x10570
#define MSR_CU_CBBCFG			0xc00110a2

void *soc_get_mbox_address(void)
{
	uintptr_t psp_mmio;

	psp_mmio = rdmsr(MSR_CU_CBBCFG).lo;
	if (psp_mmio == 0xffffffff) {
		printk(BIOS_WARNING, "PSP: MSR value uninitialized\n");
		return 0;
	}

	return (void *)(psp_mmio + PSP_MAILBOX_OFFSET);
}
