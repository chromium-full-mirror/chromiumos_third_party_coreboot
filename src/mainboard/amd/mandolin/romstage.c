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

#include <stddef.h>
#include <soc/romstage.h>
#include <amdblocks/lpc.h>
#include <superio/smsc/sio1036/sio1036.h>
#include <soc/gpio.h>
#include "gpio.h"

#define SERIAL_DEV PNP_DEV(0x4e, SIO1036_SP1)

void mainboard_romstage_early_init(void)
{
	uint32_t decode;

	mainboard_program_early_gpios();

	if (CONFIG(SUPERIO_SMSC_SIO1036)) {
		lpc_enable_sio_decode(LPC_SELECT_SIO_4E4F);
		decode = DECODE_ENABLE_SERIAL_PORT0 << CONFIG_UART_FOR_CONSOLE;
		lpc_enable_decode(decode);
		sio1036_enable_serial(SERIAL_DEV, CONFIG_TTYS0_BASE);
	}
}

void mainboard_fsp_memory_init_params_cb(FSP_M_CONFIG *mcfg, uint32_t version)
{
}
