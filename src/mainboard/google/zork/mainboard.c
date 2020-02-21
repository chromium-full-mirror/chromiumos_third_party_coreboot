/*
 * This file is part of the coreboot project.
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

#include <string.h>
#include <console/console.h>
#include <device/device.h>
#include <device/mmio.h>
#include <arch/acpi.h>
#include <amdblocks/amd_pci_util.h>
#include <amdblocks/gpio_banks.h>
#include <cbmem.h>
#include <baseboard/variants.h>
#include <boardid.h>
#include <gpio.h>
#include <smbios.h>
#include <soc/cpu.h>
#include <soc/gpio.h>
#include <soc/nvs.h>
#include <soc/pci_devs.h>
#include <soc/southbridge.h>
#include <soc/smi.h>
#include <soc/soc_util.h>
#include <amdblocks/acpimmio.h>
#include <variant/ec.h>
#include <variant/thermal.h>
#include <vendorcode/google/chromeos/chromeos.h>
#include <commonlib/helpers.h>
#include <platform_descriptors.h>

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
} fch_pirq[] = {
	{ PIRQ_A,	3,		16 },
	{ PIRQ_B,	4,		17 },
	{ PIRQ_C,	5,		18 },
	{ PIRQ_D,	7,		19 },
	{ PIRQ_E,	11,		20 },
	{ PIRQ_F,	10,		21 },
	{ PIRQ_G,	3,		22 },
	{ PIRQ_H,	4,		23 },
	{ PIRQ_SIRQA,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_SIRQB,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_SIRQC,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_SIRQD,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_SCI,	PIRQ_NC,	9 },
	{ PIRQ_SMBUS,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_ASF,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_PMON,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_SD,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_SDIO,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_CIR,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_GPIOA,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_GPIOB,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_GPIOC,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_SATA,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_EMMC,	5,		5 },
	{ PIRQ_GPP0,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_GPP1,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_GPP2,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_GPP3,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_GPIO,	7,		7 },
	{ PIRQ_I2C0,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_I2C1,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_I2C2,	10,		10 },
	{ PIRQ_I2C3,	11,		11 },
	{ PIRQ_UART0,	4,		4 },
	{ PIRQ_UART1,	3,		3 },
	{ PIRQ_I2C4,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_I2C5,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_UART2,	PIRQ_NC,	PIRQ_NC },
	{ PIRQ_UART3,	PIRQ_NC,	PIRQ_NC },

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

	for (i = 0; i < ARRAY_SIZE(fch_pirq); i++) {
		entry = fch_pirq + i;
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
	const struct sci_source *gpes;
	size_t num;
	int boardid;
	size_t num_gpios;
	const struct soc_amd_gpio *gpios;

	mainboard_ec_init();
	boardid = board_id();
	printk(BIOS_INFO, "Board ID: %d\n", boardid);

	gpios = variant_gpio_table(&num_gpios);
	program_gpios(gpios, num_gpios);

	/*
	 * Some platforms use SCI not generated by a GPIO pin (event above 23).
	 * For these boards, gpe_configure_sci() is still needed, but all GPIO
	 * generated events (23-0) must be removed from gpe_table[].
	 * For boards that only have GPIO generated events, table gpe_table[]
	 * must be removed, and get_gpe_table() should return NULL.
	 */
	gpes = get_gpe_table(&num);
	if (gpes != NULL)
		gpe_configure_sci(gpes, num);

	/* Initialize i2c busses that were not initialized in bootblock */
	i2c_soc_init();
}

static const picasso_fsp_pcie_descriptor pco_pcie_descriptors[] =
{
	{ // NVME SSD
		.port_present = true,
		.engine_type = PCIE_ENGINE,
		.start_lane = 0,
		.end_lane = 3,
		.device_number = 1,
		.function_number = 7,
		.link_aspm = ASPM_L1,
		.link_aspm_L1_1 = true,
		.link_aspm_L1_2 = true,
		.turn_off_unused_lanes = true,
		.clk_req = NVME_CLKREQ,
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
		.clk_req = WLAN_CLKREQ,
	},
	{ // SD Reader
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
		.clk_req = SD_CLKREQ,
	}
};

static const picasso_fsp_pcie_descriptor dali_pcie_descriptors[] =
{
	{ // NVME SSD
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
		.clk_req = NVME_CLKREQ,
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
		.clk_req = WLAN_CLKREQ,
	},
	{ // SD Reader
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
		.clk_req = SD_CLKREQ,
	}
};

picasso_fsp_ddi_descriptor pco_ddi_descriptors[] =
{
	{ // DDI0, DP0, eDP
		.connector_type = EDP,
		.aux_index = AUX1,
		.hdp_index = HDP1
	},
	{ // DDI1, DP1, DB OPT1 HDMI
		.connector_type = HDMI,
		.aux_index = AUX2,
		.hdp_index = HDP2
	},
	{ // DDI2, DP2, DB OPT1 USB-C1
		.connector_type = DP,
		.aux_index = AUX3,
		.hdp_index = HDP3,
	},
	{ // DDI3, DP3, USB-C0
		.connector_type = DP,
		.aux_index = AUX4,
		.hdp_index = HDP4,
	}
};

picasso_fsp_ddi_descriptor dali_ddi_descriptors[] =
{
	{ // DDI0, DP0, eDP
		.connector_type = EDP,
		.aux_index = AUX1,
		.hdp_index = HDP1
	},
	{ // DDI1, DP1, DB OPT2 USB-C1 / DB OPT3 MST hub
		.connector_type = DP,
		.aux_index = AUX2,
		.hdp_index = HDP2
	},
	// DP2 pins not connected on Dali
	{ // DDI2, DP3, USB-C0
		.connector_type = DP,
		.aux_index = AUX4,
		.hdp_index = HDP4,
	}
};

void mainboard_fsp_silicon_init_params_cb(FSP_S_CONFIG *scfg)
{

	picasso_fsp_ddi_descriptor     *fsp_ddi;
	picasso_fsp_pcie_descriptor    *fsp_pcie;
	uint8_t                        counter;

	fsp_pcie = (picasso_fsp_pcie_descriptor *)(scfg->dxio_descriptor0);
	fsp_ddi = (picasso_fsp_ddi_descriptor *)&(scfg->ddi_descriptor0);

	// Dali and Pollock
	if (soc_is_raven2()) {
		for (counter = 0; counter < ARRAY_SIZE(dali_pcie_descriptors); counter++) {
			fsp_pcie[counter] = dali_pcie_descriptors[counter];
		}

		for (counter = 0; counter < ARRAY_SIZE(dali_ddi_descriptors); counter++) {
			fsp_ddi[counter] = dali_ddi_descriptors[counter];
		}
	}
	// Picasso and default
	else {
		if (!soc_is_picasso())
			printk(BIOS_WARNING, "Warning: Unrecognized Processor in %s", __FILE__);

		for (counter = 0; counter < ARRAY_SIZE(pco_pcie_descriptors); counter++) {
			fsp_pcie[counter] = pco_pcie_descriptors[counter];
		}

		for (counter = 0; counter < ARRAY_SIZE(pco_ddi_descriptors); counter++) {
			fsp_ddi[counter] = pco_ddi_descriptors[counter];
		}
	}

// Possible definition of the SD/EMMC values
// TODO: Remove when we get official definitions
#define SD_DISABLE		0
#define SD_LOW_SPEED		1
#define SD_HIGH_SPEED		2
#define SD_UHS_I_SDR_50		3
#define SD_UHS_I_DDR_50		4
#define SD_UHS_I_SDR_104	5
#define EMMC_SDR_26		6
#define EMMC_SDR_52		7
#define EMMC_DDR_52		8
#define EMMC_HS200		9
#define EMMC_HS400		10
#define EMMC_HS300		11

	if (variant_has_emmc(variant_board_sku())) {
		if (CONFIG(AMD_FT5)) {
			printk(BIOS_WARNING, "Warning: using low speed eMMC on Dalboz!\n");
			scfg->emmc0_mode = EMMC_DDR_52;
		} else {
			scfg->emmc0_mode = EMMC_HS400;
		}
	} else {
		scfg->emmc0_mode = SD_DISABLE;
	}
}

/*************************************************
 * Dedicated mainboard function
 *************************************************/
static void zork_enable(struct device *dev)
{
	printk(BIOS_INFO, "Mainboard "
				CONFIG_MAINBOARD_PART_NUMBER " Enable.\n");

	/* Initialize the PIRQ data structures for consumption */
	pirq_setup();

	dev->ops->acpi_inject_dsdt_generator = chromeos_dsdt_generator;
}

static const struct soc_amd_gpio gpio_set_bl[] = {
	PAD_GPO(GPIO_85, LOW),
};

static void mainboard_final(void *chip_info)
{
	struct global_nvs_t *gnvs;

	gnvs = cbmem_find(CBMEM_ID_ACPI_GNVS);

	/* Re-Enable backlight - GPIO 85 active low */
	/* TODO: Remove this after AGESA stops enabling the fan */
	program_gpios(gpio_set_bl, ARRAY_SIZE(gpio_set_bl)); /*  APU_EDP_BL_DISABLE */

	if (gnvs) {
		gnvs->tmps = CTL_TDP_SENSOR_ID;
		gnvs->tcrt = CRITICAL_TEMPERATURE;
		gnvs->tpsv = PASSIVE_TEMPERATURE;
	}
}

void mainboard_suspend_resume(void)
{
	variant_mainboard_suspend_resume();
}

struct chip_operations mainboard_ops = {
	.init = mainboard_init,
	.enable_dev = zork_enable,
	.final = mainboard_final,
};

const char *smbios_system_sku(void)
{
	static char sku_str[3 + 10 + 1];

	snprintf(sku_str, sizeof(sku_str), "sku%u", variant_board_sku());

	return sku_str;
}

/* Variants may override these functions so see definitions in variants */
uint32_t __weak variant_board_sku(void)
{
	/* Magic value meaning not provisioned. */
	return 0x7fffffff;
}

void __weak variant_mainboard_suspend_resume(void)
{
}

int __weak variant_has_emmc(uint8_t sku)
{
	/* Default to EMMC enabled */
	return 1;
}
