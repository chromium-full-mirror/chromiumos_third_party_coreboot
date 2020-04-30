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

#include <types.h>
#include <baseboard/espi.h>
#include <soc/espi.h>
#include <ec/google/chromeec/ec_commands.h>

void enable_espi_early(void)
{
	/*
	 * AP supports eSPI speeds of 16, 33, and 66MHz.
	 * Trembyle EC supports standard eSPI speeds of 20MHz up to 50MHz.
	 */
	const struct espi_config cfg = {
		/* normal operating modes */
		.bus_width		= ESPI_IO_MODE_QUAD,
		.espi_freq_mhz		= ESPI_OP_FREQ_33_MHZ,
		.enable_crc_checking	= 1,
		.alert_pin_on_io1	= 0,
		.peripheral_ch_en	= 1,
		.virtual_wire_ch_en	= 1,
		.out_of_band_ch_en	= 0,
		.flash_ch_en		= 0,
		/* Set IRQ 1 and IRQ 12 to active high. */
		.irq_polarity		= BIT(1) | BIT(12),
	};

	struct resource ioports[] = { {
		.flags = IORESOURCE_IO,
		.base = 0x62,
		.size = 5,
		.next = ioports + 1,
	}, {
		.flags = IORESOURCE_IO,
		.base = EC_HOST_CMD_REGION0,
		.size = EC_HOST_CMD_REGION_SIZE * 2,
		.next = ioports + 2
	}, {
		.flags = IORESOURCE_IO,
		.base = EC_LPC_ADDR_MEMMAP,
		.size = EC_MEMMAP_SIZE + 1,
		.next = ioports + 3
	}, {
		.flags = IORESOURCE_IO,
		.base = EC_LPC_ADDR_HOST_DATA,
		.size = 8,
		.next = ioports + 4
	}, {
		.flags = IORESOURCE_IO,
		.base = 0x80,
		.size = 1,
		.next = ioports + 5
	}, {
		.flags = IORESOURCE_IO,
		.base = 0x60,
		.size = 1,
		.next = NULL
	} };

	espi_setup(&cfg);
	espi_enable_resources(ioports);
}
