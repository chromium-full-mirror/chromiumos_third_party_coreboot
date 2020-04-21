/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015-2016 Advanced Micro Devices, Inc.
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

#include <device/pci_ops.h>
#include <arch/ioapic.h>
#include <arch/acpi.h>
#include <arch/acpigen.h>
#include <arch/cpu.h>
#include <cbmem.h>
#include <console/console.h>
#include <cpu/amd/mtrr.h>
#include <cpu/x86/lapic_def.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/msr.h>
#include <device/device.h>
#include <device/pci.h>
#include <device/pci_ids.h>
#include <romstage_handoff.h>
#include <soc/cpu.h>
#include <soc/northbridge.h>
#include <soc/southbridge.h>
#include <soc/pci_devs.h>
#include <soc/iomap.h>
#include <soc/acpi.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <arch/bert_storage.h>
#include <fsp/util.h>

#include "chip.h"

static void read_resources(struct device *dev)
{
	msr_t tom = rdmsr(TOP_MEM);
	uint32_t mem_useable = (uintptr_t)cbmem_top();
	int idx = 0x10;
	const struct hob_header *hob = fsp_get_hob_list();
	const struct hob_resource *res;

	/* 0x0 - 0x9ffff */
	ram_resource(dev, idx++, 0, 0xa0000 / KiB);

	/* 0xa0000 - 0xbffff: legacy VGA */
	mmio_resource(dev, idx++, 0xa0000 / KiB, 0x20000 / KiB);

	/* 0xc0000 - 0xfffff: Option ROM */
	reserved_ram_resource(dev, idx++, 0xc0000 / KiB, 0x40000 / KiB);

	/* 1MB - bottom of DRAM consumed for hybrid romstage and storage */
	ram_resource(dev, idx++, (1 * MiB) / KiB,
			(CONFIG_ROMSTAGE_ADDR - (1 * MiB)) / KiB);

	/* DRAM consumed for hybrid romstage and storage */
	reserved_ram_resource(dev, idx++, CONFIG_ROMSTAGE_ADDR / KiB,
			ROMSTAGE_SIZE / KiB);

	/* top of DRAM consumed early - low top useable RAM
	 * cbmem_top() accounts for low UMA and TSEG if they are used. */
	ram_resource(dev, idx++, ROMSTAGE_TOP / KiB,
			(mem_useable - ROMSTAGE_TOP) / KiB);

	/* Low top useable RAM -> Low top RAM (bottom pci mmio hole) */
	reserved_ram_resource(dev, idx++, mem_useable / KiB,
					(tom.lo - mem_useable) / KiB);

	mmconf_resource(dev, MMIO_CONF_BASE);

	if (!hob) {
		printk(BIOS_ERR, "Error: %s incomplete because no HOB list was found\n",
				__func__);
		return;
	}

	for (; hob->type != HOB_TYPE_END_OF_HOB_LIST; hob = fsp_next_hob(hob)) {

		if (hob->type != HOB_TYPE_RESOURCE_DESCRIPTOR)
			continue;

		res = fsp_hob_header_to_resource(hob);

		if (res->type == EFI_RESOURCE_SYSTEM_MEMORY && res->addr < mem_useable)
			continue; /* 0 through low usable was set above */
		if (res->type == EFI_RESOURCE_MEMORY_MAPPED_IO)
			continue; /* Done separately */

		if (res->type == EFI_RESOURCE_SYSTEM_MEMORY)
			ram_resource(dev, idx++, res->addr / KiB, res->length / KiB);
		else if (res->type == EFI_RESOURCE_MEMORY_RESERVED)
			reserved_ram_resource(dev, idx++, res->addr / KiB, res->length / KiB);
		else
			printk(BIOS_ERR, "Error: failed to set resources for type %d\n",
					res->type);
	}
}

unsigned long acpi_fill_mcfg(unsigned long current)
{

	current += acpi_create_mcfg_mmconfig((acpi_mcfg_mmconfig_t *)current,
					     CONFIG_MMCONF_BASE_ADDRESS,
					     0,
					     0,
					     CONFIG_MMCONF_BUS_NUMBER);

	return current;
}

static void northbridge_fill_ssdt_generator(struct device *device)
{
	msr_t msr;
	char pscope[] = "\\_SB.PCI0";

	acpigen_write_scope(pscope);
	msr = rdmsr(TOP_MEM);
	acpigen_write_name_dword("TOM1", msr.lo);
	msr = rdmsr(TOP_MEM2);
	/*
	 * Since XP only implements parts of ACPI 2.0, we can't use a qword
	 * here.
	 * See http://www.acpi.info/presentations/S01USMOBS169_OS%2520new.ppt
	 * slide 22ff.
	 * Shift value right by 20 bit to make it fit into 32bit,
	 * giving us 1MB granularity and a limit of almost 4Exabyte of memory.
	 */
	acpigen_write_name_dword("TOM2", (msr.hi << 12) | msr.lo >> 20);
	acpigen_pop_len();
}

static struct amd_fsp_acpi_hob_info *find_acpi_guid_hob(const uint8_t *guid)
{
	const struct hob_header *hob = fsp_get_hob_list();
	struct amd_fsp_acpi_hob_info *guid_hob;
	const struct hob_resource *hob_data;

	if (!hob) {
		printk(BIOS_ERR, "Error: No HOB list was found. Cannot find ACPI hob\n");
		return NULL;
	}

	for (; hob->type != HOB_TYPE_END_OF_HOB_LIST; hob = fsp_next_hob(hob)) {
		if (hob->type != HOB_TYPE_GUID_EXTENSION) {
			continue;
		}
		hob_data = fsp_hob_header_to_resource(hob);
		guid_hob = (struct amd_fsp_acpi_hob_info *)hob;

		if (memcmp(hob_data->owner_guid, guid, 16) == 0) {
			return guid_hob;
		}
	}

	return NULL;
}

static unsigned long agesa_write_acpi_tables(struct device *device, unsigned long current,
					     acpi_rsdp_t *rsdp)
{
	struct amd_fsp_acpi_hob_info *data;
	uint8_t guid[] = AMD_FSP_ACPI_HOB_BASE_GUID;
	printk(BIOS_DEBUG, "Searching for AGESA FSP ACPI Tables\n");

	/* SSDT */
	memcpy(guid, "SSDT", 4);
	data = find_acpi_guid_hob(guid);
	if (data != NULL) {
		printk(BIOS_DEBUG, "Found SSDT\n");
		acpi_add_table(rsdp, data->hob_payload);
	}
	/* CRAT */
	memcpy(guid, "CRAT", 4);
	data = find_acpi_guid_hob(guid);
	if (data != NULL) {
		printk(BIOS_DEBUG, "Found CRAT\n");
		acpi_add_table(rsdp, data->hob_payload);
	}
	/* ALIB */
	memcpy(guid, "ALIB", 4);
	data = find_acpi_guid_hob(guid);
	if (data != NULL) {
		printk(BIOS_DEBUG, "Found ALIB\n");
		acpi_add_table(rsdp, data->hob_payload);
	}
	/* IVRS */
	memcpy(guid, "IVRS", 4);
	data = find_acpi_guid_hob(guid);
	if (data != NULL) {
		printk(BIOS_DEBUG, "Found IVRS\n");
		acpi_add_table(rsdp, data->hob_payload);
	}

	/* Add SRAT, MSCT, SLIT if needed in the future */

	return current;
}

static struct device_operations northbridge_operations = {
	.read_resources	  = read_resources,
	.enable_resources = pci_dev_enable_resources,
	.acpi_fill_ssdt_generator = northbridge_fill_ssdt_generator,
	.write_acpi_tables = agesa_write_acpi_tables,
	.enable		  = 0,
	.ops_pci	  = 0,
};

static const struct pci_driver family17_northbridge __pci_driver = {
	.ops	= &northbridge_operations,
	.vendor = PCI_VENDOR_ID_AMD,
	.device = PCI_DEVICE_ID_AMD_17H_MODEL_101F_NB,
};

static void disable_mmio_reg(int reg)
{
	pci_write_config32(SOC_DF_F0_DEVFN, NB_MMIO_CONTROL(reg),
			FABRIC_ID_IOMS0 << MMIO_DST_ID_SH);
	pci_write_config32(SOC_DF_F0_DEVFN, NB_MMIO_BASE(reg), 0);
	pci_write_config32(SOC_DF_F0_DEVFN, NB_MMIO_LIMIT(reg), 0);
}

static int is_mmio_reg_disabled(int reg)
{
	uint32_t val = pci_read_config32(SOC_DF_F0_DEVFN, NB_MMIO_CONTROL(reg));
	return !(val & ((MMIO_WE | MMIO_RE)));
}

static int find_unused_mmio_reg(void)
{
	int i;

	for (i = 0 ; i < NUM_NB_MMIO_REGS ; i++) {
		if (is_mmio_reg_disabled(i))
			return i;
	}
	return -1;
}

void northbridge_init(void)
{
	/*
	 * AGESA has already programmed the NB MMIO routing, however nothing
	 * is yet marked as non-posted.
	 *
	 * TODO: Remove the settings from AGESA and allow coreboot to own
	 * everything.  If not practical, consider erasing all settings and
	 * reprogram for coreboot.  At that time, make the following more
	 * flexible.
	 */

	int i;
	uint32_t base, limit, ctrl;
	const uint32_t adj_bot = HPET_BASE_ADDRESS >> D18F0_MMIO_SHIFT;
	const uint32_t adj_top = (LOCAL_APIC_ADDR - 1) >> D18F0_MMIO_SHIFT;

	 /* Adjust and mark from HPET-LAPIC or 0xfed00000-0xfee00000-1 as NP */
	for (i = 0 ; i < NUM_NB_MMIO_REGS ; i++) {
		/* Adjust all registers that overlap */
		ctrl = pci_read_config32(SOC_DF_F0_DEVFN, NB_MMIO_CONTROL(i));
		if (!(ctrl & (MMIO_WE | MMIO_RE)))
			continue; /* not enabled */

		base = pci_read_config32(SOC_DF_F0_DEVFN, NB_MMIO_BASE(i));
		limit = pci_read_config32(SOC_DF_F0_DEVFN, NB_MMIO_LIMIT(i));

		if (base > adj_top || limit < adj_bot)
			continue; /* no overlap */

		if (base >= adj_bot && limit <= adj_top) {
			disable_mmio_reg(i);
			continue; /* completely goes away */
		}

		if (base <= adj_bot)
			pci_write_config32(SOC_DF_F0_DEVFN, NB_MMIO_LIMIT(i),
					adj_bot - 1);
		else
			pci_write_config32(SOC_DF_F0_DEVFN, NB_MMIO_BASE(i),
					adj_top + 1);
	}
	i = find_unused_mmio_reg();
	if (i < 0) {
		printk(BIOS_ERR, "Error: cannot configure region as NP\n");
		return;
	}

	pci_write_config32(SOC_DF_F0_DEVFN, NB_MMIO_BASE(i), adj_bot);
	pci_write_config32(SOC_DF_F0_DEVFN, NB_MMIO_LIMIT(i), adj_top);
	pci_write_config32(SOC_DF_F0_DEVFN, NB_MMIO_CONTROL(i),
			FABRIC_ID_IOMS0 << MMIO_DST_ID_SH |
			MMIO_NP | MMIO_WE | MMIO_RE);
}

void domain_set_resources(struct device *dev)
{
	assign_resources(dev->link_list);
}

/*
 * Check for Zen2 video bios requirement
 */
void map_oprom_vendev_rev(u32 *vendev, u8 *rev)
{
	if (*vendev != PICASSO_VBIOS_VID_DID) {
		return;
	}

	switch (*rev) {
	case 0xc1:	/* Picasso - Ryzen 7 3700C */
	case 0xc2:	/* Picasso - Ryzen 5 3500C */
		printk(BIOS_SPEW, "Using Zen+ vBIOS.\n");
		*rev = PICASSO_VBIOS_REV;
		break;
	case 0xc4:	/* Dali - Ryzen 3 3250C */
	case 0xcd:	/* Dali - Athlon Silver 3050C */
	case 0xce:	/* Dali - Athlon Gold 3150C */
	case 0xe9:	/* Pollock - Samples */
	case 0xea:	/* Pollock - Production */
		printk(BIOS_SPEW, "Using Zen2 vBIOS.\n");
		*vendev = DALI_VBIOS_VID_DID;
		*rev = DALI_VBIOS_REV;
		break;
	default:
		printk(BIOS_WARNING, "Warning: Unknown device. Verify vBIOS.\n");
		break;
	}

	return;
}
