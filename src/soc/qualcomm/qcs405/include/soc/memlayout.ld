/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2018, The Linux Foundation.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <memlayout.h>
#include <arch/header.ld>

/* SYSTEM_IMEM : 0x8600000 - 0x8607FFF */
#define SSRAM_START(addr) SYMBOL(ssram, addr)
#define SSRAM_END(addr) SYMBOL(essram, addr)

/* BOOT_IMEM   : 0x8C00000 - 0x8D80000 */
#define BSRAM_START(addr) SYMBOL(bsram, addr)
#define BSRAM_END(addr) SYMBOL(ebsram, addr)

SECTIONS
{
	SSRAM_START(0x8600000)
	REGION(qcsdi, 0x8600000, 0x8000, 4096)
	SSRAM_END(0x8608000)

	BSRAM_START(0x8C00000)
	REGION(fw_reserved2, 0x8C00000, 0x16000, 4096)
	OVERLAP_VERSTAGE_ROMSTAGE(0x8C16000, 104K)
	BOOTBLOCK(0x8C30000, 40K)
	TTB(0x8C3A000, 56K)
	VBOOT2_WORK(0x8C48000, 12K)
	STACK(0x8C4C000, 16K)
	TIMESTAMP(0x8C50000, 1K)
	PRERAM_CBMEM_CONSOLE(0x8C50400, 32K)
	PRERAM_CBFS_CACHE(0x8C58400, 70K)
	REGION(qclib, 0x8C6A000, 512K, 4096)
	REGION(pmic, 0x8CED000, 64K, 4096)
	REGION(qclib_serial_log, 0x8CFD000, 4K, 8)
	REGION(ddr_training, 0x8CFE000, 8K, 8)
	REGION(ddr_information, 0x8D00000, 256, 256)
	REGION(dcb, 0x8D01000, 4K, 8)
	REGION(qclib_shared_data, 0x8D78000, 32K, 4096)
	BSRAM_END(0x8D80000)

	DRAM_START(0x80000000)
	/* DDR Carveout for BL31 usage */
	REGION(dram_reserved, 0x85000000, 0x5100000, 4096)
	POSTRAM_CBFS_CACHE(0x9F800000, 384K)
	RAMSTAGE(0x9F860000, 128K)
}
