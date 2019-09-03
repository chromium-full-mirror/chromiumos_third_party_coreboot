/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2015 Advanced Micro Devices, Inc.
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
#include <device/device.h>
#include <arch/acpi.h>
#include <amdblocks/amd_pci_util.h>
#include <soc/espi.h>
#include <soc/southbridge.h>
#include <soc/pci_devs.h>
#include "gpio.h"
//
//
//
// todo: check file for accuracy
//
//
//

/***********************************************************
 * These arrays set up the FCH PCI_INTR registers 0xC00/0xC01.
 * This table is responsible for physically routing the PIC and
 * IOAPIC IRQs to the different PCI devices on the system.  It
 * is read and written via registers 0xC00/0xC01 as an
 * Index/Data pair.  These values are chipset and mainboard
 * dependent and should be updated accordingly.
 *
 * These values are used by the PCI configuration space,
 * MP Tables.  TODO: Make ACPI use these values too.
 */
static uint8_t fch_pic_routing[0x80];
static uint8_t fch_apic_routing[0x80];

_Static_assert(sizeof(fch_pic_routing) == sizeof(fch_apic_routing),
	"PIC and APIC FCH interrupt tables must be the same size");

static const struct pirq_struct mainboard_pirq_data[] = {
	{ PCIE0_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE1_DEVFN,	{ PIRQ_E, PIRQ_F, PIRQ_G, PIRQ_H } },
	{ PCIE2_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE3_DEVFN,	{ PIRQ_E, PIRQ_F, PIRQ_G, PIRQ_H } },
	{ PCIE4_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE5_DEVFN,	{ PIRQ_E, PIRQ_F, PIRQ_G, PIRQ_H } },
	{ PCIE6_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE7_DEVFN,	{ PIRQ_E, PIRQ_F, PIRQ_G, PIRQ_H } },
	{ PCIE8_DEVFN,	{ PIRQ_G, PIRQ_H, PIRQ_E, PIRQ_F } },
	{ PCIE8_DEVFN,	{ PIRQ_G, PIRQ_H, PIRQ_E, PIRQ_F } },
	{ SMBUS_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
};

static const struct fch_apic_routing {
	uint8_t intr_index;
	uint8_t pic_irq_num;
	uint8_t apic_irq_num;
} mandolin_fch[] = {
	{ PIRQ_A,	 3,		16 },
	{ PIRQ_B,	 4,		17 },
	{ PIRQ_C,	 5,		18 },
	{ PIRQ_D,	 7,		19 },
	{ PIRQ_E,	11,		20 },
	{ PIRQ_F,	10,		21 },
	{ PIRQ_G,	 3,		22 },
	{ PIRQ_H,	 4,		23 },
	{ PIRQ_SIRQA,	PIRQ_NC,	 1 },
	{ PIRQ_SIRQB,	PIRQ_NC,	 2 },
	{ PIRQ_SIRQC,	PIRQ_NC,	 3 },
	{ PIRQ_SIRQD,	PIRQ_NC,	 4 },
	{ PIRQ_SCI,	PIRQ_NC,	 9 },
	{ PIRQ_SMBUS,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_ASF,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_PMON,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_SD,	PIRQ_NC,	16 },
	{ PIRQ_SDIO,	 0,		PIRQ_NC },
	{ PIRQ_CIR,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_GPIOA,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_GPIOB,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_GPIOC,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_SATA,	PIRQ_NC,	19 },
	{ PIRQ_EMMC,	 0,		 0 },
	{ PIRQ_GPP0,	 3,		PIRQ_NC },
	{ PIRQ_GPP1,	 4,		PIRQ_NC },
	{ PIRQ_GPP2,	 5,		PIRQ_NC },
	{ PIRQ_GPP3,	 7,		PIRQ_NC },
	{ PIRQ_GPIO,	 7,		 7 },
	{ PIRQ_I2C0,	 3,		 3 },
	{ PIRQ_I2C1,	15,		15 },
	{ PIRQ_I2C2,	 6,		 6 },
	{ PIRQ_I2C3,	14,		14 },
	{ PIRQ_UART0,	 4,		 4 },
	{ PIRQ_UART1,	 3,		 3 },
	{ PIRQ_I2C4,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_I2C5,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_UART2,	PIRQ_NC,	 0 },
	{ PIRQ_UART3,	PIRQ_NC,	 0 },

	/* The MISC registers are not interrupt numbers */
	{ PIRQ_MISC,	0xfa,		0x00 },
	{ PIRQ_MISC0,	0xf1,		0x00 },
	{ PIRQ_MISC1,	0x00,		0x00 },
	{ PIRQ_MISC2,	0x00,		0x00 },
};

static void init_tables(void)
{
	const struct fch_apic_routing *entry;
	int i;

	memset(fch_pic_routing, PIRQ_NC, sizeof(fch_pic_routing));
	memset(fch_apic_routing, PIRQ_NC, sizeof(fch_apic_routing));

	for (i = 0; i < ARRAY_SIZE(mandolin_fch); i++) {
		entry = mandolin_fch + i;
		fch_pic_routing[entry->intr_index] = entry->pic_irq_num;
		fch_apic_routing[entry->intr_index] = entry->apic_irq_num;
	}
}

/* PIRQ Setup */
static void pirq_setup(void)
{
	init_tables();

	pirq_data_ptr = mainboard_pirq_data;
	pirq_data_size = ARRAY_SIZE(mainboard_pirq_data);
	intr_data_ptr = fch_apic_routing;
	picr_data_ptr = fch_pic_routing;
}

static void enable_ec_io_ports(void)
{
	const struct espi_config cfg = {
		.bus_width		= ESPI_SINGLE_IO,
		.espi_freq_mhz		= 20,
		.enable_crc_checking	= 1,
		.alert_pin_on_io1	= 0,
		.peripheral_ch_en	= 0,
		.virtual_wire_ch_en	= 0,
		.out_of_band_ch_en	= 0,
		.flash_ch_en		= 0,
		.update_slave = 1,
	};

	struct resource ioports[] = { {
		.flags = IORESOURCE_IO,
		.base = 0x662,
		.size = 8,
		.next = ioports + 1,
	}, {
		.flags = IORESOURCE_IO,
		.base = 0x60,
		.size = 1,
		.next = ioports + 2,
	}, {
		.flags = IORESOURCE_IO,
		.base = 0x64,
		.size = 1,
		.next = NULL
	} };

	espi_setup(&cfg);
	espi_enable_resources(ioports);
}

static void mainboard_init(void *chip_info)
{
	mainboard_program_gpios();
}

void mainboard_fsp_silicon_init_params_cb(FSP_S_CONFIG *scfg)
{
	scfg->pcie_port0_topology = 0x8f610000;
	scfg->pcie_port1_topology = 0x44010000;
	scfg->pcie_port2_topology = 0x55110000;
	scfg->pcie_port3_topology = 0x66210000;
	scfg->pcie_port4_topology = 0x77310000;
	scfg->pcie_port5_topology = 0;
	scfg->pcie_port6_topology = 0x01510000;
	scfg->pcie_sata_topology  = 0x23020000;
	scfg->pcie_xgbe1_topology = 0;
	scfg->pcie_xgbe2_topology = 0;
	if (!CONFIG(PICASSO_LPC_IOMUX))
		scfg->emmc0_mode = 10;
	scfg->dp0_connector_type = 0;
	scfg->dp1_connector_type = 1;
}

/*************************************************
 * enable the dedicated function in mandolin board.
 *************************************************/
static void mandolin_enable(struct device *dev)
{
	printk(BIOS_INFO, "Mainboard "
				CONFIG_MAINBOARD_PART_NUMBER " Enable.\n");

	/* Initialize the PIRQ data structures for consumption */
	pirq_setup();
	enable_ec_io_ports();
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
	.enable_dev = mandolin_enable,
};
