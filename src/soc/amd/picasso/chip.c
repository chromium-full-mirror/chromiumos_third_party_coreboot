/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2017 Advanced Micro Devices, Inc.
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

#include <amdblocks/spi.h>
#include <acpi/acpigen.h>
#include <bootstate.h>
#include <console/console.h>
#include <cpu/x86/mtrr.h>
#include <cpu/amd/mtrr.h>
#include <device/device.h>
#include <device/pci.h>
#include <drivers/i2c/designware/dw_i2c.h>
#include <romstage_handoff.h>
#include <soc/acpi.h>
#include <soc/cpu.h>
#include <soc/northbridge.h>
#include <soc/iomap.h>
#include <soc/pci_devs.h>
#include <soc/soc_util.h>
#include <soc/southbridge.h>
#include "chip.h"

/* Supplied by i2c.c */
extern struct device_operations picasso_i2c_mmio_ops;

struct device_operations cpu_bus_ops = {
	.read_resources	  = DEVICE_NOOP,
	.set_resources	  = DEVICE_NOOP,
	.enable_resources = DEVICE_NOOP,
	.init		  = picasso_init_cpus,
	.acpi_fill_ssdt_generator = generate_cpu_entries,
};

const char *soc_acpi_name(const struct device *dev)
{
	if (dev->path.type == DEVICE_PATH_DOMAIN)
		return "PCI0";

	if (dev->path.type == DEVICE_PATH_USB) {
		switch (dev->path.usb.port_type) {
		case 0:
			/* Root Hub */
			return "RHUB";
		case 2:
			/* USB2 ports */
			switch (dev->path.usb.port_id) {
			case 0: return "HS01";
			case 1: return "HS02";
			case 2: return "HS03";
			case 3: return "HS04";
			case 4: return "HS05";
			case 5: return "HS06";
			}
			break;
		case 3:
			/* USB3 ports */
			switch (dev->path.usb.port_id) {
			case 0: return "SS01";
			case 1: return "SS02";
			case 2: return "SS03";
			case 3: return "SS04";
			case 4: return "SS05";
			}
			break;
		}
		return NULL;
	}

	if (dev->path.type != DEVICE_PATH_PCI)
		return NULL;

	switch (dev->path.pci.devfn) {
	case GNB_DEVFN: 	// GNB Root Complex
		return "GNB";
	case IOMMU_DEVFN: 	// IOMMU
		return "IOMM";
	/* PCIe GPP Bridges PCIE_GPP_#_DEVFN*/
	case PCIE_GPP_0_DEVFN:
		return "PBR0";
	case PCIE_GPP_1_DEVFN:
		return "PBR1";
	case PCIE_GPP_2_DEVFN:
		return "PBR2";
	case PCIE_GPP_3_DEVFN:
		return "PBR3";
	case PCIE_GPP_4_DEVFN:
		return "PBR4";
	case PCIE_GPP_5_DEVFN:
		return "PBR5";
	case PCIE_GPP_6_DEVFN:
		return "PBR6";
	case PCIE_A_DEVFN:
		return "PBRA";
	case PCIE_B_DEVFN:
		return "PBRB";
	case HDA1_DEVFN: 	// HD Audio
		return "AZHD";
	case LPC_DEVFN: 	// LPC Bus
		return "LPCB";
	case SMBUS_DEVFN: 	// SMBUS
		return "SBUS";
	case DF_F0_DEVFN:
		return "DFD1";	// Data Fabric 0
	case DF_F1_DEVFN:
		return "DFD2";	// Data Fabric 1
	case DF_F2_DEVFN:
		return "DFD3";	// Data Fabric 2
	case DF_F3_DEVFN:
		return "DFD3";	// Data Fabric 3
	case DF_F4_DEVFN:
		return "DFD4";	// Data Fabric 4
	case DF_F5_DEVFN:
		return "DFD5";	// Data Fabric 5
	case DF_F6_DEVFN:
		return "DFD6";	// Data Fabric 6
	default:
		return NULL;
	}
};

struct device_operations pci_domain_ops = {
	.read_resources	  = pci_domain_read_resources,
	.set_resources	  = domain_set_resources,
	.scan_bus	  = pci_domain_scan_bus,
	.acpi_name	  = soc_acpi_name,
};

static void set_mmio_dev_ops(struct device *dev)
{
	switch (dev->path.mmio.addr) {
	case APU_I2C2_BASE:
	case APU_I2C3_BASE:
	case APU_I2C4_BASE:
		dev->ops = &picasso_i2c_mmio_ops;
		break;
	}
}

static void enable_dev(struct device *dev)
{
	/* Set the operations if it is a special bus type */
	if (dev->path.type == DEVICE_PATH_DOMAIN)
		dev->ops = &pci_domain_ops;
	else if (dev->path.type == DEVICE_PATH_CPU_CLUSTER)
		dev->ops = &cpu_bus_ops;
	else if (dev->path.type == DEVICE_PATH_PCI)
		sb_enable(dev);
	else if (dev->path.type == DEVICE_PATH_MMIO)
		set_mmio_dev_ops(dev);
}

void *vbt_get(void)
{
	return NULL;
}

static void soc_init(void *chip_info)
{
	/* Set the Fixed MTRRs to send cycles to DRAM.  AGESA will use just <1MB
	 * as an AP wake buffer */
	msr_t mtrr, syscfg, deftype;

	deftype = rdmsr(MTRR_DEF_TYPE_MSR);
	deftype.lo |= MTRR_DEF_TYPE_EN | MTRR_DEF_TYPE_FIX_EN;
	wrmsr(MTRR_DEF_TYPE_MSR, deftype);

	syscfg = rdmsr(SYSCFG_MSR);
	syscfg.lo |= SYSCFG_MSR_MtrrFixDramEn | SYSCFG_MSR_MtrrFixDramModEn;
	wrmsr(SYSCFG_MSR, syscfg);

	mtrr.hi = mtrr.lo = 0x1e1e1e1e; /* RdDram,WrDram,WB */
	wrmsr(MTRR_FIX_4K_F8000, mtrr);

	fsp_silicon_init(acpi_is_wakeup_s3());

	/*
	 * TODO(furquan): Get rid of this once FSP is fixed to not touch SPI
	 * registers(b/153506142). Currently, FSP-S reconfigures SPI frequency causing em100 to
	 * stop working. Thus, reconfigure SPI speeds here.
	 */
	fch_spi_config_modes();

	northbridge_init();
	southbridge_init(chip_info);
	setup_bsp_ramtop();
}

static void soc_final(void *chip_info)
{
	southbridge_final(chip_info);
}

struct chip_operations soc_amd_picasso_ops = {
	CHIP_NAME("AMD Picasso SOC")
	.enable_dev = enable_dev,
	.init = soc_init,
	.final = soc_final
};
