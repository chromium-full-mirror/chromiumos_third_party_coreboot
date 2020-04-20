/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <commonlib/helpers.h>
#include <console/console.h>
#include <drivers/i2c/designware/dw_i2c.h>
#include <soc/i2c.h>
#include <soc/iomap.h>
#include <stdint.h>

static uintptr_t i2c_bus_address[I2C_MASTER_DEV_COUNT];

uintptr_t dw_i2c_base_address(unsigned int bus)
{
	if (bus >= ARRAY_SIZE(i2c_bus_address))
		return 0;

	return i2c_bus_address[bus];
}
void i2c_set_bar(unsigned int bus, uintptr_t bar)
{
	if (bus >= ARRAY_SIZE(i2c_bus_address)) {
		printk(BIOS_ERR, "Error: i2c index out of bounds: %u.", bus);
		return;
	}

	i2c_bus_address[bus] = bar;
}
