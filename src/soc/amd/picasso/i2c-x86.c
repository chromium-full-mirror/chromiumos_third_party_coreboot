/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <stdint.h>
#include <soc/iomap.h>
#include <commonlib/helpers.h>
#include <drivers/i2c/designware/dw_i2c.h>

/*
 * We don't have addresses for I2C0-1.
 */
static const uintptr_t i2c_bus_address[] = {
	0,
	0,
	APU_I2C2_BASE,
	APU_I2C3_BASE,
	APU_I2C4_BASE, /* Can only be used in slave mode */
};

_Static_assert(
	ARRAY_SIZE(i2c_bus_address) == I2C_MASTER_DEV_COUNT + I2C_SLAVE_DEV_COUNT,
	"ARRAY_SIZE(i2c_bus_address) must equal I2C_MASTER_DEV_COUNT + I2C_SLAVE_DEV_COUNT");

uintptr_t dw_i2c_base_address(unsigned int bus)
{
	if (bus >= ARRAY_SIZE(i2c_bus_address))
		return 0;

	return i2c_bus_address[bus];
}
