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

#ifndef __BASEBOARD_GPIO_H__
#define __BASEBOARD_GPIO_H__

#ifndef __ACPI__
#include <soc/gpio.h>

/* CR50 interrupt pin */
#define H1_PCH_INT		GPIO_3		/* H1_INT */

/* SPI Write protect */
#define CROS_WP_GPIO		GPIO_137	/* BIOS_FLASH_WP_L */
#define GPIO_EC_IN_RW		GPIO_130	/* EC_IN_RW_OD */

/* PCIe reset pins */
#define PCIE_0_RST		GPIO_142	/* WIFI_AUX_RESET_L */
#define PCIE_1_RST		GPIO_142	/* SD_AUX_RESET_L */
#define PCIE_2_RST		0
#define PCIE_3_RST		0
#define PCIE_4_RST		GPIO_40		/* NVME_AUX_RESET_L */

#endif /* _ACPI__ */

/* These define the GPE, not the GPIO. */
#define EC_SCI_GPI		23	/* eSPI system event -> GPE 23 */
#define EC_WAKE_GPI		15	/* AGPIO 24 -> GPE 15 */

/* EC sync irq */
#define EC_SYNC_IRQ	31

#endif /* __BASEBOARD_GPIO_H__ */
