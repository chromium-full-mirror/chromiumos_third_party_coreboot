/*
 * This file is part of the coreboot project.
 *
 * Copyright 2020 Google, Inc.
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

#ifndef __AMDBLOCKS_I2C_H__
#define __AMDBLOCKS_I2C_H__

#include <device/resource.h>

/* Some systems can only determine the number of i2c controllers at runtime. */
void soc_update_i2c_resource(struct resource *res);

#endif /* __AMDBLOCKS_I2C_H__ */
