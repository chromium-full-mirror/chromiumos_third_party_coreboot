/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Intel Corp.
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

#define __SIMPLE_DEVICE__

#include <assert.h>
#include <stdint.h>
#include <console/console.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/smm.h>
#include <cpu/amd/msr.h>
#include <cpu/amd/mtrr.h>
#include <cbmem.h>
#include <arch/bert_storage.h>
#include <soc/northbridge.h>
#include <soc/iomap.h>
#include <amdblocks/acpimmio.h>
#include <fsp/util.h>

#define BERT_REGION_MAX_SIZE 0x100000

static uintptr_t top_of_lowmem(void)
{
	return biosram_read32(BIOSRAM_TOP_LOWMEM);
}

void bert_reserved_region(void **start, size_t *size)
{
	const struct cbmem_entry *bert;

	*start = 0;
	*size = 0;

	bert = cbmem_entry_find(CBMEM_ID_BERT_RAW_DATA);
	if (!bert)
		return;

	*start = cbmem_entry_start(bert);
	*size = cbmem_entry_size(bert);
}

static void init_topmem(void)
{
	const struct hob_header *hob = fsp_get_hob_list();
	const struct hob_resource *res;
	uintptr_t topmem = 0;

	if (!hob) {
		printk(BIOS_ERR, "Error: No HOB list was found, can't calculate cbmem_top()\n");
		biosram_write32(BIOSRAM_TOP_LOWMEM, 0);
		return;
	}

	for (; hob->type != HOB_TYPE_END_OF_HOB_LIST; hob = fsp_next_hob(hob)) {
		if (hob->type != HOB_TYPE_RESOURCE_DESCRIPTOR)
			continue;

		res = fsp_hob_header_to_resource(hob);
		if (res->type != EFI_RESOURCE_SYSTEM_MEMORY)
			continue;

		/* cbmem is highest available DRAM below 4GB */
		if (res->addr < (uint64_t)4 * GiB && res->addr + res->length > topmem)
			topmem = res->addr + res->length;
	}
	biosram_write32(BIOSRAM_TOP_LOWMEM, topmem);
}

void *cbmem_top(void)
{
	static int once;

	if (ENV_ROMSTAGE && !once) {
		init_topmem();
		once = 1;
	}

	/* 8MB alignment to keep MTRR usage low */
	return (void *)ALIGN_DOWN(top_of_lowmem(), 8 * MiB);
}

/*
 * For data stored in TSEG, ensure TValid is clear so R/W access can reach
 * the DRAM when not in SMM.
 */
static void clear_tvalid(void)
{
	msr_t hwcr = rdmsr(HWCR_MSR);
	msr_t mask = rdmsr(SMM_MASK_MSR);
	int tvalid = !!(mask.lo & SMM_TSEG_VALID);

	if (hwcr.lo & SMM_LOCK) {
		if (!tvalid) /* not valid but locked means still accessible */
			return;

		printk(BIOS_ERR, "Error: can't clear TValid, already locked\n");
		return;
	}

	mask.lo &= ~SMM_TSEG_VALID;
	wrmsr(SMM_MASK_MSR, mask);
}

void smm_region(uintptr_t *start, size_t *size)
{
	static int once;
	const struct cbmem_entry *smm;

	*start = 0;
	*size = 0;

	smm = cbmem_entry_find(CBMEM_ID_SMM_TSEG_SPACE);
	if (!smm)
		return;

	*start = ALIGN_UP((uintptr_t)cbmem_entry_start(smm), CONFIG_SMM_TSEG_SIZE);
	*size = CONFIG_SMM_TSEG_SIZE;

	if (!once) {
		clear_tvalid();
		once = 1;
	}
}

/* Add or find TSEG and BERT storage prior to ramstage.  ramstage may need
 * to be recovered from cached space in TSEG.  BERT storage is consumed at
 * ROMSTAGE_CBMEM_INIT_HOOK and ordering can't be enforced for a hook.
 */
static void alloc_reserved_in_cbmem(int unused)
{
	void *p;

	/* Make large enough so TSEG can have alignment = size, allowing a
	 * good mask.
	 */
	p = cbmem_add(CBMEM_ID_SMM_TSEG_SPACE, 2 * CONFIG_SMM_TSEG_SIZE);

	if (CONFIG(ACPI_BERT))
		p = cbmem_add(CBMEM_ID_BERT_RAW_DATA, BERT_REGION_MAX_SIZE);
}

ROMSTAGE_CBMEM_INIT_HOOK(alloc_reserved_in_cbmem)
