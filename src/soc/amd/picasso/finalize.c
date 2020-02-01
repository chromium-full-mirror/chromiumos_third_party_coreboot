/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2018 Advanced Micro Devices, Inc.
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

#include <cpu/x86/mp.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/msr.h>
#include <bootstate.h>
#include <timer.h>
#include <console/console.h>
#include <fmap.h>
#include <commonlib/helpers.h>
#include <commonlib/region.h>
#include <boot_device.h>
#include "config.h"
#include <soc/psp.h>
#include <spi_flash.h>
#include <soc/iomap.h>

static void per_core_finalize(void *unused)
{
	msr_t hwcr, mask;

	/* Finalize SMM settings */
	hwcr = rdmsr(HWCR_MSR);
	if (hwcr.lo & SMM_LOCK) /* Skip if already locked, avoid GPF */
		return;

	if (CONFIG(HAVE_SMI_HANDLER)) {
		mask = rdmsr(SMM_MASK_MSR);
		mask.lo |= SMM_TSEG_VALID;
		wrmsr(SMM_MASK_MSR, mask);
	}

	hwcr.lo |= SMM_LOCK;
	wrmsr(HWCR_MSR, hwcr);
}

static void finalize_cores(void)
{
	int r;
	printk(BIOS_SPEW, "Lock SMM configuration\n");

	r = mp_run_on_all_cpus(per_core_finalize, NULL);
	if (r)
		printk(BIOS_WARNING, "Failed to finalize all cores\n");
}

static void soc_finalize(void *unused)
{
	finalize_cores();

	post_code(POST_OS_BOOT);
}

BOOT_STATE_INIT_ENTRY(BS_OS_RESUME, BS_ON_ENTRY, soc_finalize, NULL);
BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_LOAD, BS_ON_EXIT, soc_finalize, NULL);

/* validate APOB header */
static int apob_header_valid(const struct apob_base_header *apob_header_ptr,
	const char *where)
{
	if (apob_header_ptr->signature != APOB_SIGNATURE) {
		printk(BIOS_WARNING, "Invalid %s APOB signature %x\n",
			where, apob_header_ptr->signature);
		return -1;
	}

	if (apob_header_ptr->size == 0 ||
			apob_header_ptr->size > CONFIG_MRC_SETTINGS_CACHE_SIZE) {
		printk(BIOS_WARNING, "%s APOB data is too large %x > %x\n",
			where,
			apob_header_ptr->size, CONFIG_MRC_SETTINGS_CACHE_SIZE);
		return -1;
	}

	return 0;
}

#define DEFAULT_MRC_CACHE "RW_MRC_CACHE"

/* Save APOB buffer to flash */
static void update_apob_in_flash(
	const struct apob_base_header *apob_src_ram, const struct region *region)
{
	struct apob_base_header *apob_rom;
	struct region_device read_rdev;
	struct region_device write_rdev;
	struct incoherent_rdev backing_irdev;
	const struct region_device *backing_rdev;
	bool update_needed = false;

	if (boot_device_ro_subregion(region, &read_rdev) < 0) {
		printk(BIOS_ERR, "Failed boot_device_ro_subregion\n");
		return;
	}

	if (boot_device_rw_subregion(region, &write_rdev) < 0) {
		printk(BIOS_ERR, "Failed boot_device_rw_subregion\n");
		return;
	}

	backing_rdev = incoherent_rdev_init(&backing_irdev, region, &read_rdev,
						&write_rdev);

	if (backing_rdev == NULL) {
		printk(BIOS_ERR, "Failed incoherent_rdev_init\n");
		return;
	}

	/* map flash region */
	apob_rom = rdev_mmap_full(backing_rdev);
	if (apob_rom == NULL) {
		printk(BIOS_ERR, "Error: Can't map APOB flash region\n");
		return;
	}

	if (apob_header_valid(apob_src_ram, "RAM") < 0)
		return;

	if (apob_header_valid(apob_rom, "ROM") < 0)
		update_needed = true;
	else if (memcmp(apob_src_ram, apob_rom, apob_src_ram->size)) {
		printk(BIOS_INFO, "APOB RAM copy differs from flash\n");
		update_needed = true;
	} else
		printk(BIOS_DEBUG, "APOB valid copy is already in flash\n");

	/* unmap the region before write to be safe */
	rdev_munmap(backing_rdev, apob_rom);

	if (!update_needed)
		return;

	/* write data to flash region */
	if (rdev_eraseat(backing_rdev, 0, CONFIG_MRC_SETTINGS_CACHE_SIZE) < 0) {
		printk(BIOS_ERR, "Error: APOB flash region erase failed\n");
		return;
	}

	if (rdev_writeat(backing_rdev, apob_src_ram, 0,
			apob_src_ram->size) < 0) {
		printk(BIOS_ERR, "Error: APOB flash region update failed\n");
		return;
	}

	printk(BIOS_INFO, "Updated APOB in flash\n");
}

static void save_apob(void *unused)
{
	const struct apob_base_header *apob_src_ram;
	uint32_t apob_ram_size;
	struct region region;
	const struct spi_flash *flash = boot_device_spi_flash();

	if (!flash) {
		printk(BIOS_ERR, "Error: No flash found, skipping APOB NV\n");
		return;
	}

	/* TODO: Find the APOB destination by parsing the PSP's tables
	 *       (once vboot is implemented).  Same for FMAP region below. */
	apob_src_ram = (struct apob_base_header *)CONFIG_PSP_APOB_DRAM_ADDRESS;
	apob_ram_size = apob_src_ram->size;

	if  (fmap_locate_area(DEFAULT_MRC_CACHE, &region) < 0) {
		printk(BIOS_ERR, "Error: No APOB NV region is found in flash\n");
		return;
	}

	printk(BIOS_SPEW, "Copy APOB from RAM 0x%p/0x%x to flash 0x%zx/0x%zx\n",
		apob_src_ram, apob_ram_size,
		region_offset(&region), region_sz(&region));
	update_apob_in_flash(apob_src_ram, &region);
}

/*
 * Ensure APOB is stored into SPI flash after PCI enumeration is done.
 */
BOOT_STATE_INIT_ENTRY(BS_DEV_ENUMERATE, BS_ON_EXIT, save_apob, NULL);
