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
#include <acpi/acpi.h>
#include <amdblocks/amd_pci_util.h>
#include <soc/cpu.h>
#include <soc/southbridge.h>
#include <soc/pci_devs.h>
#include <soc/platform_descriptors.h>
#include <soc/soc_util.h>
#include <commonlib/helpers.h>
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
	{ PCIE1_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE2_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE3_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE4_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE5_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE6_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE7_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE8_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ PCIE8_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
	{ SMBUS_DEVFN,	{ PIRQ_A, PIRQ_B, PIRQ_C, PIRQ_D } },
};

static const struct fch_apic_routing {
	uint8_t intr_index;
	uint8_t pic_irq_num;
	uint8_t apic_irq_num;
} mandolin_fch[] = {
	{ PIRQ_A,	8,		16 },
	{ PIRQ_B,	10,		17 },
	{ PIRQ_C,	11,		18 },
	{ PIRQ_D,	12,		19 },
	{ PIRQ_E,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_F,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_G,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_H,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_SIRQA,	PIRQ_NC,	 1 },
	{ PIRQ_SIRQB,	PIRQ_NC,	 2 },
	{ PIRQ_SIRQC,	PIRQ_NC,	 3 },
	{ PIRQ_SIRQD,	PIRQ_NC,	 4 },
	{ PIRQ_SCI,	9,		 9 },
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
	{ PIRQ_EMMC,	PIRQ_NC,	17 },
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

static void mainboard_init(void *chip_info)
{
	struct soc_amd_picasso_config *cfg = config_of_soc();

	if (!CONFIG(PICASSO_LPC_IOMUX)) {
		cfg->sd_emmc_config = SD_EMMC_EMMC_HS400;
	}

	mainboard_program_gpios();
}

static const picasso_fsp_pcie_descriptor pco_pcie_descriptors[] =
{
	{ // MXM
		.port_present = true,
		.engine_type = PCIE_ENGINE,
		.start_lane = 8,
		.end_lane = 15,
		.device_number = 1,
		.function_number = 1,
		.link_aspm = ASPM_L1,
		.link_aspm_L1_1 = true,
		.link_aspm_L1_2 = true,
		.turn_off_unused_lanes = true,
		.clk_req = CLK_REQ6
	},
	{ // SSD
		.port_present = true,
		.engine_type = PCIE_ENGINE,
		.start_lane = 0,
		.end_lane = 1,
		.device_number = 1,
		.function_number = 7,
		.link_aspm = ASPM_L1,
		.link_aspm_L1_1 = true,
		.link_aspm_L1_2 = true,
		.turn_off_unused_lanes = true,
		.clk_req = CLK_REQ5
	},
	{ // WLAN
		.port_present = true,
		.engine_type = PCIE_ENGINE,
		.start_lane = 4,
		.end_lane = 4,
		.device_number = 1,
		.function_number = 2,
		.link_aspm = ASPM_L1,
		.link_aspm_L1_1 = true,
		.link_aspm_L1_2 = true,
		.turn_off_unused_lanes = true,
		.clk_req = CLK_REQ0
	},
	{ // LAN
		.port_present = true,
		.engine_type = PCIE_ENGINE,
		.start_lane = 5,
		.end_lane = 5,
		.device_number = 1,
		.function_number = 3,
		.link_aspm = ASPM_L1,
		.link_aspm_L1_1 = true,
		.link_aspm_L1_2 = true,
		.turn_off_unused_lanes = true,
		.clk_req = CLK_REQ1
	},
	{ // WWAN
		.port_present = true,
		.engine_type = PCIE_ENGINE,
		.start_lane = 6,
		.end_lane = 6,
		.device_number = 1,
		.function_number = 4,
		.link_aspm = ASPM_L1,
		.link_aspm_L1_1 = true,
		.link_aspm_L1_2 = true,
		.turn_off_unused_lanes = true,
		.clk_req = CLK_REQ2
	},
	{ // WIFI
		.port_present = true,
		.engine_type = PCIE_ENGINE,
		.start_lane = 7,
		.end_lane = 7,
		.gpio_group_id = 1,
		.device_number = 1,
		.function_number = 5,
		.link_aspm = ASPM_L1,
		.link_aspm_L1_1 = true,
		.link_aspm_L1_2 = true,
		.turn_off_unused_lanes = true,
		.clk_req = CLK_REQ3
	},
	{ // SATA EXPRESS
		.port_present = true,
		.engine_type = SATA_ENGINE,
		.start_lane = 2,
		.end_lane = 3,
		.gpio_group_id = 1,
		.channel_type = SATA_CHANNEL_LONG,
	}
};

static const picasso_fsp_pcie_descriptor dali_pcie_descriptors[] =
{
	{ // MXM
		.port_present = true,
		.engine_type = PCIE_ENGINE,
		.start_lane = 8,
		.end_lane = 11,
		.device_number = 1,
		.function_number = 1,
		.link_aspm = ASPM_L1,
		.link_aspm_L1_1 = true,
		.link_aspm_L1_2 = true,
		.turn_off_unused_lanes = true,
		.clk_req = CLK_REQ6
	},
	{ // SSD
		.port_present = true,
		.engine_type = PCIE_ENGINE,
		.start_lane = 0,
		.end_lane = 1,
		.device_number = 1,
		.function_number = 7,
		.link_aspm = ASPM_L1,
		.link_aspm_L1_1 = true,
		.link_aspm_L1_2 = true,
		.turn_off_unused_lanes = true,
		.clk_req = CLK_REQ5
	},
	{ // WLAN
		.port_present = true,
		.engine_type = PCIE_ENGINE,
		.start_lane = 4,
		.end_lane = 4,
		.device_number = 1,
		.function_number = 2,
		.link_aspm = ASPM_L1,
		.link_aspm_L1_1 = true,
		.link_aspm_L1_2 = true,
		.turn_off_unused_lanes = true,
		.clk_req = CLK_REQ0
	},
	{ // LAN
		.port_present = true,
		.engine_type = PCIE_ENGINE,
		.start_lane = 5,
		.end_lane = 5,
		.device_number = 1,
		.function_number = 3,
		.link_aspm = ASPM_L1,
		.link_aspm_L1_1 = true,
		.link_aspm_L1_2 = true,
		.turn_off_unused_lanes = true,
		.clk_req = CLK_REQ1
	}
};

static const picasso_fsp_ddi_descriptor pco_ddi_descriptors[] =
{
	{ // DDI0 - DP
		.connector_type = DP,
		.aux_index = AUX1,
		.hdp_index = HDP1
	},
	{ // DDI1 - eDP
		.connector_type = EDP,
		.aux_index = AUX2,
		.hdp_index = HDP2
	},
	{ // DDI2 - DP
		.connector_type = DP,
		.aux_index = AUX3,
		.hdp_index = HDP3,
	},
	{ // DDI3 - DP
		.connector_type = DP,
		.aux_index = AUX4,
		.hdp_index = HDP4,
	}
};

static const picasso_fsp_ddi_descriptor dali_ddi_descriptors[] =
{
	{ // DDI0 - DP
		.connector_type = DP,
		.aux_index = AUX1,
		.hdp_index = HDP1
	},
	{ // DDI1 - eDP
		.connector_type = EDP,
		.aux_index = AUX2,
		.hdp_index = HDP2
	},
	{ // DDI2 - DP
		.connector_type = DP,
		.aux_index = AUX3,
		.hdp_index = HDP3,
	}
};

void mainboard_get_pcie_ddi_descriptors(
		const picasso_fsp_pcie_descriptor **pcie_descs, size_t *pcie_num,
		const picasso_fsp_ddi_descriptor **ddi_descs, size_t *ddi_num)
{
	// Dali
	if (soc_is_dali()) {
		*pcie_descs = dali_pcie_descriptors;
		*pcie_num = ARRAY_SIZE(dali_pcie_descriptors);
		*ddi_descs = dali_ddi_descriptors;
		*ddi_num = ARRAY_SIZE(dali_ddi_descriptors);
	}
	// Picasso and default
	else {
		if (!soc_is_picasso())
			printk(BIOS_WARNING, "Warning: Unrecognized Processor in %s", __FILE__);

		*pcie_descs = pco_pcie_descriptors;
		*pcie_num = ARRAY_SIZE(pco_pcie_descriptors);
		*ddi_descs = pco_ddi_descriptors;
		*ddi_num = ARRAY_SIZE(pco_ddi_descriptors);
	}
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
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
	.enable_dev = mandolin_enable,
};
