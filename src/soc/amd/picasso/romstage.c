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
#include <cpu/x86/cache.h>
#include <cpu/x86/msr.h>
#include <cpu/x86/mtrr.h>
#include <cpu/x86/smm.h>
#include <cpu/x86/bist.h>
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
#include <security/vboot/symbols.h>
#include <soc/cpu.h>
#include <soc/memmap.h>
#include <soc/mrc_cache.h>
#include <soc/northbridge.h>
#include <soc/pci_devs.h>
#include <soc/southbridge.h>
#include <soc/romstage.h>
#include <fsp/api.h>
#include "chip.h"
#include <2struct.h>

void __weak romstage_mainboard_early_init(void) {}
void __weak romstage_mainboard_init(void) {}

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

static void romstage_soc_init(void)
{
	fch_early_init();
}

static int set_early_mtrrs(void)
{
	msr_t top_mem;
	msr_t sys_cfg;
	msr_t mtrr_def_type;
	msr_t fixed_mtrr_ram;
	msr_t fixed_mtrr_mmio;
	struct var_mtrr_context mtrr_ctx;

	var_mtrr_context_init(&mtrr_ctx, NULL);
	top_mem = rdmsr(TOP_MEM);
	/* Enable RdDram and WrDram attributes in fixed MTRRs. */
	sys_cfg = rdmsr(SYSCFG_MSR);
	sys_cfg.lo |= SYSCFG_MSR_MtrrFixDramModEn;

	/* Fixed MTRR constants. */
	fixed_mtrr_ram.lo = fixed_mtrr_ram.hi =
		((MTRR_TYPE_WRBACK | MTRR_READ_MEM | MTRR_WRITE_MEM) <<  0) |
		((MTRR_TYPE_WRBACK | MTRR_READ_MEM | MTRR_WRITE_MEM) <<  8) |
		((MTRR_TYPE_WRBACK | MTRR_READ_MEM | MTRR_WRITE_MEM) << 16) |
		((MTRR_TYPE_WRBACK | MTRR_READ_MEM | MTRR_WRITE_MEM) << 24);
	fixed_mtrr_mmio.lo = fixed_mtrr_mmio.hi =
		((MTRR_TYPE_UNCACHEABLE) <<  0) |
		((MTRR_TYPE_UNCACHEABLE) <<  8) |
		((MTRR_TYPE_UNCACHEABLE) << 16) |
		((MTRR_TYPE_UNCACHEABLE) << 24);

	/* Prep default MTRR type. */
	mtrr_def_type = rdmsr(MTRR_DEF_TYPE_MSR);
	mtrr_def_type.lo &= ~MTRR_DEF_TYPE_MASK;
	mtrr_def_type.lo |= MTRR_TYPE_UNCACHEABLE;
	mtrr_def_type.lo |= MTRR_DEF_TYPE_EN | MTRR_DEF_TYPE_FIX_EN;

	disable_cache();

	wrmsr(SYSCFG_MSR, sys_cfg);

	clear_all_var_mtrr();

	var_mtrr_set(&mtrr_ctx, 0, ALIGN_DOWN(top_mem.lo, 8*MiB), MTRR_TYPE_WRBACK);
	var_mtrr_set(&mtrr_ctx, FLASH_BASE_ADDR, CONFIG_ROM_SIZE, MTRR_TYPE_WRPROT);

	/* Set up RAM caching for everything below 1MiB except for 0xa0000-0xc0000 . */
	wrmsr(MTRR_FIX_64K_00000, fixed_mtrr_ram);
	wrmsr(MTRR_FIX_16K_80000, fixed_mtrr_ram);
	wrmsr(MTRR_FIX_16K_A0000, fixed_mtrr_mmio);
	wrmsr(MTRR_FIX_4K_C0000, fixed_mtrr_ram);
	wrmsr(MTRR_FIX_4K_C8000, fixed_mtrr_ram);
	wrmsr(MTRR_FIX_4K_D0000, fixed_mtrr_ram);
	wrmsr(MTRR_FIX_4K_D8000, fixed_mtrr_ram);
	wrmsr(MTRR_FIX_4K_E0000, fixed_mtrr_ram);
	wrmsr(MTRR_FIX_4K_E8000, fixed_mtrr_ram);
	wrmsr(MTRR_FIX_4K_F0000, fixed_mtrr_ram);
	wrmsr(MTRR_FIX_4K_F8000, fixed_mtrr_ram);

	wrmsr(MTRR_DEF_TYPE_MSR, mtrr_def_type);

	/* Enable Fixed and Variable MTRRs. */
	sys_cfg.lo |= SYSCFG_MSR_MtrrFixDramEn | SYSCFG_MSR_MtrrVarDramEn;
	sys_cfg.lo |= SYSCFG_MSR_TOM2En | SYSCFG_MSR_TOM2WB;
	wrmsr(SYSCFG_MSR, sys_cfg);

	enable_cache();

	return 0;
}

void platform_fsp_memory_init_params_cb(FSPM_UPD *mupd, uint32_t version)
{
	FSP_M_CONFIG *mcfg = &mupd->FspmConfig;
	const config_t *config = config_of_soc();

	mupd->FspmArchUpd.NvsBufferPtr = soc_fill_mrc_cache();

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

static void check_workbuf(uintptr_t workbuf_location)
{
	int workbuf_invalid = 0;

	if (*(unsigned int *)workbuf_location != VB2_SHARED_DATA_MAGIC) {
		workbuf_invalid = 1;
	}

	/* TODO: Check PSP mailbox registers b/153700436 */

	if (workbuf_invalid) {
		printk(BIOS_ERR,"ERROR: VBOOT workbuf not valid.\n");

		printk(BIOS_DEBUG,"Signature: %#08x\n",*(unsigned int *)workbuf_location);

		/* TODO: Reboot into recovery b/152638343 */
		die("Halting.\n");
	}
}

asmlinkage void soc_hybrid_romstage_entry(uint32_t bist, uint64_t early_tsc)
{
	int s3_resume;
	int early_mtrr_err;
	msr_t s3_resume_entry = {
		.hi = (uint64_t)(uintptr_t)s3_bsp_reentry >> 32,
		.lo = (uintptr_t)s3_bsp_reentry & 0xffffffff,
	};

	post_code(0x40);
	if (CONFIG(COLLECT_TIMESTAMPS)) {
		timestamp_init(early_tsc);
		timestamp_add_now(TS_START_ROMSTAGE);
	}

	/* Many of these tasks typically happen in bootblock, but execution
	 * begins in romstage for this device. */
	post_code(0x41);
	early_mtrr_err = set_early_mtrrs();

	post_code(0x42);

	romstage_soc_early_init();
	romstage_mainboard_early_init();

	console_init();

	if (CONFIG(VBOOT_STARTS_BEFORE_BOOTBLOCK))
		check_workbuf((uintptr_t)_vboot2_work);

	post_code(0x43);
	init_timer();
	sanitize_cmos();
	cmos_post_init();

	post_code(0x44);
	exception_init();

	post_code(0x45);
	report_bist_failure(bist);

	post_code(0x46);
	s3_resume = acpi_s3_resume_allowed() && acpi_is_wakeup_s3();

	romstage_soc_init();
	romstage_mainboard_init();

	/* Trigger the microcode to stash the CPU state and resume vector
	 * into the C6 save area. */
	if (!s3_resume)
		wrmsr(S3_RESUME_EIP, s3_resume_entry);

	post_code(0x47);
	u32 val = cpuid_eax(1);
	printk(BIOS_DEBUG, "Family_Model: %08x\n", val);

	if (early_mtrr_err)
		printk(BIOS_WARNING, "Early MTRRs were not set properly\n");

	post_code(0x49);
	fsp_memory_init(s3_resume);
	soc_update_mrc_cache();

	memmap_stash_early_dram_usage();

	post_code(0x4a);
	run_ramstage();

	post_code(0x50); /* Should never see this post code. */
}
