/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Intel Corp.
 * (Written by Lance Zhao <lijian.zhao@intel.com> for Intel Corp.)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __SOC_PICASSO_ACPI_H__
#define __SOC_PICASSO_ACPI_H__

#include <arch/acpi.h>
#include <fsp/util.h>

#define AMD_FSP_ACPI_HOB_BASE_GUID { \
	0x00, 0x00, 0x00, 0x00, \
	0x02, 0x40, 0x3B, 0x40, \
	0x87, 0xE1, 0x3F, 0xEB, \
	0x13, 0xC5, 0x66, 0x9A \
	}

struct amd_fsp_acpi_hob_info {
	struct hob_guid_extension guid_extension_header;
	uint32_t table_size_in_bytes;
	uint8_t total_hobs_for_table;
	uint8_t sequence_number;
	uint16_t reserved;
	uint16_t hob_payload[0xffc8];
} __packed;

unsigned long southbridge_write_acpi_tables(struct device *device,
		unsigned long current, struct acpi_rsdp *rsdp);

void southbridge_inject_dsdt(struct device *device);

const char *soc_acpi_name(const struct device *dev);

#endif /* __SOC_PICASSO_ACPI_H__ */
