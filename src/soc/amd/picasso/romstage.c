/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015-2019 Advanced Micro Devices, Inc.
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

#include <arch/cpu.h>
#include <arch/romstage.h>
#include <arch/acpi.h>
#include <arch/exception.h>
#include <delay.h>
#include <pc80/mc146818rtc.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/smm.h>
#include <cpu/x86/bist.h>
#include <cpu/amd/msr.h>
#include <cbmem.h>
#include <console/console.h>
#include <commonlib/helpers.h>
#include <timestamp.h>
#include <program_loading.h>
#include <romstage_handoff.h>
#include <elog.h>
#include <soc/cpu.h>
#include <soc/northbridge.h>
#include <soc/southbridge.h>
#include <soc/romstage.h>

void __weak mainboard_romstage_early_init(void) {}
void __weak mainboard_romstage_entry_s3(int s3_resume) {}

static void romstage_soc_early_init(void)
{
	msr_t mmconf;

	mmconf.hi = 0;
	mmconf.lo = CONFIG_MMCONF_BASE_ADDRESS | MMIO_RANGE_EN
			| fms(CONFIG_MMCONF_BUS_NUMBER) << MMIO_BUS_RANGE_SHIFT;
	wrmsr(MMIO_CONF_BASE, mmconf);

	fch_pre_init();
}

static void romstage_soc_init(int s3_resume)
{
	fch_early_init();
}

asmlinkage void soc_hybrid_romstage_entry(uint32_t bist, uint64_t early_tsc)
{
	uintptr_t top_of_mem;
	int s3_resume;

	post_code(0x40);
	if (CONFIG(COLLECT_TIMESTAMPS)) {
		timestamp_init(early_tsc);
		timestamp_add_now(TS_START_ROMSTAGE);
	}

	/* Many of these tasks typically happen in bootblock, but execution
	 * begins in romstage for this device. */
	post_code(0x41);

	post_code(0x42);
	romstage_soc_early_init();
	romstage_mainboard_early_init();

	post_code(0x43);
	init_timer();
	sanitize_cmos();
	cmos_post_init();

	post_code(0x44);
	console_init();
	exception_init();

	post_code(0x45);
	report_bist_failure(bist);

	post_code(0x46);
	s3_resume = acpi_s3_resume_allowed() && acpi_is_wakeup_s3();
	romstage_soc_init(s3_resume);
	mainboard_romstage_entry_s3(s3_resume);

	post_code(0x47);
	u32 val = cpuid_eax(1);
	printk(BIOS_DEBUG, "Family_Model: %08x\n", val);

	post_code(0x48);
	if (!s3_resume && CONFIG(ELOG_BOOT_COUNT))
		boot_count_increment();

	post_code(0x49);
	top_of_mem = ALIGN_DOWN(rdmsr(TOP_MEM).lo, BIT(23));
	backup_top_of_low_cacheable(top_of_mem);

	post_code(0x4a);
	if (cbmem_recovery(s3_resume))
		printk(BIOS_CRIT, "Failed to recover cbmem\n");
	if (romstage_handoff_init(s3_resume))
		printk(BIOS_ERR, "Failed to set romstage handoff data\n");

	post_code(0x4b);
	run_ramstage();

	post_code(0x50); /* Should never see this post code. */
}
