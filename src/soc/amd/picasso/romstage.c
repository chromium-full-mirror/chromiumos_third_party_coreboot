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

#include <amdblocks/lpc.h>
#include <arch/cpu.h>
#include <arch/romstage.h>
#include <acpi/acpi.h>
#include <arch/exception.h>
#include <delay.h>
#include <pc80/mc146818rtc.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/smm.h>
#include <cpu/x86/bist.h>
#include <cpu/x86/cache.h>
#include <cpu/amd/mtrr.h>
#include <cpu/amd/msr.h>
#include <device/pci_ops.h>
#include <smp/node.h>
#include <console/uart.h>
#include <cbmem.h>
#include <console/console.h>
#include <commonlib/helpers.h>
#include <timestamp.h>
#include <program_loading.h>
#include <romstage_handoff.h>
#include <elog.h>
#include <soc/cpu.h>
#include <soc/northbridge.h>
#include <soc/pci_devs.h>
#include <soc/southbridge.h>
#include <soc/romstage.h>
#include <fsp/api.h>
#include "chip.h"

void __weak mainboard_romstage_early_init(void) {}
void __weak mainboard_romstage_entry_s3(int s3_resume) {}

static uint8_t telemetry_config_used(const config_t *config)
{
	if ((config->telemetry_vddcr_vdd_slope) ||
		(config->telemetry_vddcr_vdd_offset) ||
		(config->telemetry_vddcr_soc_slope) ||
		(config->telemetry_vddcr_soc_offset))
		return 1;

	return 0;
}

static void disable_rom_sharing(void)
{
	u8 byte;

	byte = pci_read_config8(SOC_LPC_DEV, LPC_PCI_CONTROL);
	byte &= ~VW_ROM_SHARING_EN;
	byte &= ~EXT_ROM_SHARING_EN;
	pci_write_config8(SOC_LPC_DEV, LPC_PCI_CONTROL, byte);
}

static void romstage_soc_early_init(void)
{
	msr_t mmconf;

	mmconf.hi = 0;
	mmconf.lo = CONFIG_MMCONF_BASE_ADDRESS | MMIO_RANGE_EN
			| fms(CONFIG_MMCONF_BUS_NUMBER) << MMIO_BUS_RANGE_SHIFT;
	wrmsr(MMIO_CONF_BASE, mmconf);

	fch_pre_init();

	if (CONFIG(DISABLE_SPI_FLASH_ROM_SHARING))
		disable_rom_sharing();
}

static void romstage_soc_init(int s3_resume)
{
	fch_early_init();
}

static int set_early_mtrrs(void)
{
	int mtrr;

	mtrr = get_free_var_mtrr();
	if (mtrr < 0)
		return -1;

	set_var_mtrr(mtrr, EARLY_DRAM_MTRR_BASE, EARLY_DRAM_MTRR_SIZE,
				MTRR_TYPE_WRBACK);

	mtrr = get_free_var_mtrr();
	if (mtrr < 0)
		return -1;

	set_var_mtrr(mtrr, FLASH_BASE_ADDR, CONFIG_ROM_SIZE,
				MTRR_TYPE_WRPROT);
	return 0;
}

static void clear_agesa_mtrrs(void)
{
	msr_t mtrr_cap = rdmsr(MTRR_CAP_MSR);
	int vmtrrs = mtrr_cap.lo & MTRR_CAP_VCNT;
	int i;
	msr_t mtrr = {
		.hi = 0,
		.lo = 0,
	};

	for (i = 0 ; i < vmtrrs ; i++) {
		wrmsr(MTRR_PHYS_MASK(i), mtrr);
		wrmsr(MTRR_PHYS_BASE(i), mtrr);
	}

	/* Disable WB from to region 4GB - TOM2 */
	msr_t sys_cfg = rdmsr(SYSCFG_MSR);
	sys_cfg.lo &= ~SYSCFG_MSR_TOM2WB;
	wrmsr(SYSCFG_MSR, sys_cfg);

	if (set_early_mtrrs())
		printk(BIOS_WARNING, "Warning: MTRRs not set properly for ramstage\n");
}

static void set_mtrrs_for_ramstage(void)
{
	uintptr_t mem_top;
	uintptr_t ramstage_wb_base;
	size_t ramstage_wb_size;
	int mtrr;

	mem_top = (uintptr_t)cbmem_top();

	clear_agesa_mtrrs(); /* TODO: make AGESA leave the MTRRs alone*/

	/* Cache anticipated ramstage location through the top of cbmem.
	 * Unlike some other implementations, TSEG is in cbmem so it will
	 * be cached as well.
	 */
	ramstage_wb_size = EARLY_RAMSTAGE_MTRR_SZ;
	ramstage_wb_base = mem_top - ramstage_wb_size;

	/* Ensure base and size are usable in a single MTRR pair */
	if (ramstage_wb_size != 1 << fms(ramstage_wb_size)) {
		ramstage_wb_size = 1 << (1 + fms(ramstage_wb_size));
		ramstage_wb_base = mem_top - ramstage_wb_size;
	}
	if (mem_top - EARLY_RAMSTAGE_MTRR_SZ < EARLY_DRAM_MTRR_TOP) {
		printk(BIOS_WARNING, "Warning: Skipping ramstage cacheable due to configuration\n");
		return;
	}

	if (!IS_ALIGNED(ramstage_wb_base, ramstage_wb_size)) {
		uintptr_t wb_end = ramstage_wb_base + ramstage_wb_size;
		// since we're in early stage, try only size * 2 for safety
		size_t new_size = ramstage_wb_size * 2;
		uintptr_t new_base = ALIGN_DOWN(ramstage_wb_base, new_size);
		// see if it fits
		if ((new_base <= ramstage_wb_base) &&
		    (new_base + new_size >= wb_end) &&
		    (new_base + new_size > new_base)) {
			printk(BIOS_INFO, "Expanding MTRR from %lx-%lx to %lx-%lx\n",
				   ramstage_wb_base, wb_end,
				   new_base, new_base + new_size);
			ramstage_wb_base = new_base;
			ramstage_wb_size = new_size;
		}
	}

	mtrr = get_free_var_mtrr();
	if (mtrr >= 0)
		set_var_mtrr(mtrr, ramstage_wb_base, ramstage_wb_size,
					MTRR_TYPE_WRBACK);
	else
		printk(BIOS_WARNING, "Warning: Unable to make ramstage cacheable\n");
}

static void enable_mtrr_and_cache(void)
{
	msr_t msr;
	int deftype = MTRR_TYPE_UNCACHEABLE; // default to UC

	msr = rdmsr(MTRR_DEF_TYPE_MSR);
	msr.lo &= ~0xff;
	msr.lo |= MTRR_DEF_TYPE_EN | deftype;
	wrmsr(MTRR_DEF_TYPE_MSR, msr);

	enable_cache();
}

void platform_fsp_memory_init_params_cb(FSPM_UPD *mupd, uint32_t version)
{
	FSP_M_CONFIG *mcfg = &mupd->FspmConfig;
	const config_t *config = config_of_soc();

	mcfg->pci_express_base_addr = CONFIG_MMCONF_BASE_ADDRESS;

	mcfg->serial_port_base = uart_platform_base(CONFIG_UART_FOR_CONSOLE);
	mcfg->serial_port_use_mmio = CONFIG(DRIVERS_UART_8250MEM);
	mcfg->serial_port_stride = CONFIG(DRIVERS_UART_8250MEM_32) ? 4 : 1;
	mcfg->serial_port_baudrate = get_uart_baudrate();
	mcfg->serial_port_refclk = uart_platform_refclk();

	if (config != NULL) {
		mcfg->system_config = config->system_config;

		if ((config->slow_ppt_limit) &&
			(config->fast_ppt_limit) &&
			(config->slow_ppt_time_constant) &&
			(config->stapm_time_constant)) {
			mcfg->slow_ppt_limit = config->slow_ppt_limit;
			mcfg->fast_ppt_limit = config->fast_ppt_limit;
			mcfg->slow_ppt_time_constant = config->slow_ppt_time_constant;
			mcfg->stapm_time_constant = config->stapm_time_constant;
		}

		mcfg->sustained_power_limit = config->sustained_power_limit;
		mcfg->prochot_l_deassertion_ramp_time = config->prochot_l_deassertion_ramp_time;
		mcfg->thermctl_limit = config->thermctl_limit;
		mcfg->psi0_current_limit = config->psi0_current_limit;
		mcfg->psi0_soc_current_limit = config->psi0_soc_current_limit;
		mcfg->vddcr_soc_voltage_margin = config->vddcr_soc_voltage_margin;
		mcfg->vddcr_vdd_voltage_margin = config->vddcr_vdd_voltage_margin;
		mcfg->vrm_maximum_current_limit = config->vrm_maximum_current_limit;
		mcfg->vrm_soc_maximum_current_limit = config->vrm_soc_maximum_current_limit;
		mcfg->vrm_current_limit = config->vrm_current_limit;
		mcfg->vrm_soc_current_limit = config->vrm_soc_current_limit;
		mcfg->sb_tsi_alert_comparator_mode_en = config->sb_tsi_alert_comparator_mode_en;
		mcfg->core_dldo_bypass = config->core_dldo_bypass;
		mcfg->min_soc_vid_offset = config->min_soc_vid_offset;
		mcfg->aclk_dpm0_freq_400MHz = config->aclk_dpm0_freq_400MHz;

		if (telemetry_config_used(config)) {
			mcfg->telemetry_vddcr_vdd_slope = config->telemetry_vddcr_vdd_slope;
			mcfg->telemetry_vddcr_vdd_offset = config->telemetry_vddcr_vdd_offset;
			mcfg->telemetry_vddcr_soc_slope = config->telemetry_vddcr_soc_slope;
			mcfg->telemetry_vddcr_soc_offset = config->telemetry_vddcr_soc_offset;
		}
	}
}

asmlinkage void soc_hybrid_romstage_entry(uint32_t bist, uint64_t early_tsc)
{
	int s3_resume;
	int on_bsp = boot_cpu();
	int early_mtrr_err;
	msr_t s3_resume_entry = {
		.hi = (uint64_t)(uintptr_t)s3_bsp_reentry >> 32,
		.lo = (uintptr_t)s3_bsp_reentry & 0xffffffff,
	};

	post_code(0x40);
	if (CONFIG(COLLECT_TIMESTAMPS) && on_bsp) {
		timestamp_init(early_tsc);
		timestamp_add_now(TS_START_ROMSTAGE);
	}

	/* Many of these tasks typically happen in bootblock, but execution
	 * begins in romstage for this device. */
	post_code(0x41);
	early_mtrr_err = set_early_mtrrs();

	if (on_bsp) {
		post_code(0x42);
		romstage_soc_early_init();
		console_init();
		mainboard_romstage_early_init(); /* espi + port 80 init */

		post_code(0x43);
		init_timer();
		sanitize_cmos();
		cmos_post_init();

		post_code(0x44);
		exception_init();
	}

	post_code(0x45);
	report_bist_failure(bist);

	post_code(0x46);
	s3_resume = acpi_s3_resume_allowed() && acpi_is_wakeup_s3();
	if (on_bsp) {
		romstage_soc_init(s3_resume);
		mainboard_romstage_entry_s3(s3_resume);

		/* Trigger the microcode to stash the CPU state and resume vector
		 * into the C6 save area. */
		if (!s3_resume)
			wrmsr(S3_RESUME_EIP, s3_resume_entry);
	}

	post_code(0x47);
	u32 val = cpuid_eax(1);
	printk(BIOS_DEBUG, "Family_Model: %08x\n", val);

	if (early_mtrr_err)
		printk(BIOS_WARNING, "Early MTRRs were not set properly\n");

	post_code(0x49);
	fsp_memory_init(s3_resume);
	/* APs do not return to here and continue  */

	post_code(0x4a);
	set_mtrrs_for_ramstage();
	enable_mtrr_and_cache();
	run_ramstage();

	post_code(0x50); /* Should never see this post code. */
}
