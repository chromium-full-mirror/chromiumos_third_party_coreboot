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

#ifndef __PICASSO_PSP_H__
#define __PICASSO_PSP_H__

#if !CONFIG_PSP_APOB_DRAM_ADDRESS || !CONFIG_MRC_SETTINGS_CACHE_SIZE
#error Incorrect APOB configuration setting(s)
#endif

#define APOB_SIGNATURE 0x424F5041	/* 'APOB' */

/* APOB_BASE_HEADER from AGESA */
struct apob_base_header {
	uint32_t   signature;			/* APOB signature */
	uint32_t   version;			/* Version */
	uint32_t   size;			/* APOB Size */
	uint32_t   offset_of_first_entry;	/* APOB Header Size */
};

#endif /* __PICASSO_PSP_H__ */
