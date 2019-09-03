/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2019 Advanced Micro Devices, Inc.
 * (Written by Hugh Edward Richard <hugh.e.dick@silverbackltd.com> for AMD Inc.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef AMD_PICASSO_ESPI_H
#define AMD_PICASSO_ESPI_H

enum espi_io_width {
	ESPI_SINGLE_IO,
	ESPI_DUAL_IO,
	ESPI_QUAD_IO,
};

/*
 * If 'update_slave' is set, then this will cause an eSPI bus transaction to
 * write to the slave's config registers. This is not needed if the setting
 * match the slave's defaults.
 */
struct espi_config {
	enum espi_io_width bus_width;
	unsigned int espi_freq_mhz;
	unsigned int enable_crc_checking : 1;
	unsigned int alert_pin_on_io1 : 1;
	unsigned int peripheral_ch_en : 1;
	unsigned int virtual_wire_ch_en : 1;
	unsigned int out_of_band_ch_en : 1;
	unsigned int flash_ch_en : 1;
	unsigned int update_slave : 1;
};

void espi_setup(const struct espi_config *cfg);
int espi_enable_resources(const struct resource *resource_linked_list);
void espi_enable_children_resources(struct device *espi);

#endif /* AMD_PICASSO_ESPI_H */
