/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2019 Advanced Micro Devices, Inc.
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
#include <stddef.h>
#include <soc/romstage.h>
#include <amdblocks/lpc.h>
#include <superio/smsc/sio1036/sio1036.h>
#include <soc/espi.h>
#include <soc/gpio.h>
#include "gpio.h"

#define SERIAL_DEV PNP_DEV(0x4e, SIO1036_SP1)

static void enable_espi_early(void)
{
	const struct espi_config cfg = {
		.bus_width		= ESPI_IO_MODE_SINGLE,
		.espi_freq_mhz		= ESPI_OP_FREQ_33_MHZ,
		.enable_crc_checking	= 1,
		.alert_pin_on_io1	= 0,
		.peripheral_ch_en	= 0,
		.virtual_wire_ch_en	= 0,
		.out_of_band_ch_en	= 0,
		.flash_ch_en		= 0,
		.update_slave		= 0,
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

void romstage_mainboard_early_init(void)
{
	uint32_t decode;

	mainboard_program_early_gpios();

	if (CONFIG(SUPERIO_SMSC_SIO1036)) {
		lpc_enable_sio_decode(LPC_SELECT_SIO_4E4F);
		decode = DECODE_ENABLE_SERIAL_PORT0 << CONFIG_UART_FOR_CONSOLE;
		lpc_enable_decode(decode);
		sio1036_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
	}

	printk(BIOS_DEBUG, "Mandolin: Enable eSPI channel to EC\n");
	enable_espi_early();
}
