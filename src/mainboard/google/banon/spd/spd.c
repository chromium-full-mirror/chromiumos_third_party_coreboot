/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013 Google Inc.
 * Copyright (C) 2015 Intel Corp.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc.
 */

#include <cbfs.h>
#include <cbmem.h>
#include <console/console.h>
#include <gpio.h>
#include <lib.h>
#include <mainboard/google/banon/spd/spd.h>
#include <memory_info.h>
#include <smbios.h>
#include <spd.h>
#include <soc/gpio.h>
#include <soc/romstage.h>
#include <string.h>

#define SPD_SIZE 256

/*
 * 0b0000 - 4GiB total - 2 x 2GiB Samsung K4E8E304EE-EGCE
 * 0b0001 - 4GiB total - 2 x 2GiB Samsung K4E8E324EB-EGCF
 * 0b0011 - 4GiB total - 2 x 2GiB Hynix H9CCNNN8JTBLAR
 * 0b0100 - 4GiB total - 2 x 2GiB Micron MT52L256M32D1PF
 * 0b0101 - 4GiB total - 2 x 2GiB Hynix H9CCNNN8GTALAR
 * 0b1000 - 2GiB total - 1 x 2GiB Samsung K4E8E304EE-EGCE
 * 0b1001 - 2GiB total - 1 x 2GiB Samsung K4E8E324EB-EGCF
 * 0b1011 - 2GiB total - 1 x 2GiB Hynix H9CCNNN8JTBLAR
 * 0b1100 - 2GiB total - 1 x 2GiB Micron MT52L256M32D1PF
 * 0b1101 - 2GiB total - 1 x 2GiB Hynix H9CCNNN8GTALAR
 */

/* Copy SPD data for on-board memory */
static void *get_spd_pointer(char *spd_file_content, int total_spds
				, int *single)
{
	int ram_id = 0;

	gpio_t spd_gpios[] = {
		GP_SW_80,	/* SATA_GP3,RAMID0 */
		GP_SW_67,	/* I2C3_SCL,RAMID1 */
		GP_SE_02,	/* MF_PLT_CLK1, RAMID2 */
		GP_SW_64,	/* I2C3_SDA RAMID3 */
	};

	ram_id = (gpio_base2_value(spd_gpios, ARRAY_SIZE(spd_gpios))) & 0x07;
	*single = gpio_get(spd_gpios[3]);

	if (ram_id >= total_spds)
		return NULL;

	/* Display the RAM type */
	switch (ram_id) {
	case 0:
		printk(BIOS_DEBUG, "Samsung K4E8E304EE-EGCE\n");
		break;
	case 1:
		printk(BIOS_DEBUG, "Samsung K4E8E324EB-EGCF\n");
		break;
	case 2:
		printk(BIOS_DEBUG, "Unknown future LPDDR3\n");
		break;
	case 3:
		printk(BIOS_DEBUG, "Hynix H9CCNNN8JTBLAR\n");
		break;
	case 4:
		printk(BIOS_DEBUG, "Micron MT52L256M32D1PF\n");
		break;
	case 5:
		printk(BIOS_DEBUG, "Hynix H9CCNNN8GTALAR\n");
		break;
	}

	return &spd_file_content[SPD_SIZE * ram_id];
}



/* Copy SPD data for on-board memory */
void mainboard_fill_spd_data(struct pei_data *ps)
{
	struct cbfs_file *spd_file;
	void *spd_content;
	int single_channel = 0;

	/* Find the SPD data in CBFS. */
	spd_file = cbfs_get_file(CBFS_DEFAULT_MEDIA, "spd.bin");
	if (!spd_file)
		die("SPD data not found.");

	spd_content = get_spd_pointer(CBFS_SUBHEADER(spd_file),
				      ntohl(spd_file->len) / SPD_SIZE,
					&single_channel);

	if (IS_ENABLED(CONFIG_DISPLAY_SPD_DATA) && spd_content != NULL) {
		printk(BIOS_DEBUG, "SPD Data:\n");
		hexdump(spd_content, SPD_SIZE);
		printk(BIOS_DEBUG, "\n");
	}

	if (spd_content != NULL) {
		ps->spd_data_ch0 = spd_content;
		ps->spd_ch0_config = 1;
		if (single_channel) {
			ps->spd_ch1_config = 2;
		} else {
			ps->spd_data_ch1 = spd_content;
			ps->spd_ch1_config = 1;
		}
	}
}


static void set_dimm_info(uint8_t *spd, struct dimm_info *dimm)
{
	const int spd_capmb[8] = {  1,  2,  4,  8, 16, 32, 64,  0 };
	const int spd_ranks[8] = {  1,  2,  3,  4, -1, -1, -1, -1 };
	const int spd_devw[8]  = {  4,  8, 16, 32, -1, -1, -1, -1 };
	const int spd_busw[8]  = {  8, 16, 32, 64, -1, -1, -1, -1 };
	uint16_t clock_frequency;

	int capmb = spd_capmb[spd[SPD_DENSITY_BANKS] & 7] * 256;
	int ranks = spd_ranks[(spd[SPD_ORGANIZATION] >> 3) & 7];
	int devw  = spd_devw[spd[SPD_ORGANIZATION] & 7];
	int busw  = spd_busw[spd[SPD_BUS_DEV_WIDTH] & 7];

	/* Parse the SPD data to determine the DIMM information */
	dimm->ddr_type = MEMORY_DEVICE_LPDDR3;
	dimm->dimm_size = capmb / 8 * busw / devw * ranks;  /* MiB */
	clock_frequency = 1000 * spd[11] / (spd[10] * spd[12]);	/* MHz */
	dimm->ddr_frequency = 2 * clock_frequency;	/* Double Data Rate */
	dimm->mod_type = spd[3] & 0xf;
	memcpy((char *)&dimm->module_part_number[0], &spd[0x80],
		sizeof(dimm->module_part_number) - 1);
	dimm->mod_id = *(uint16_t *)&spd[0x94];

	switch (busw) {
	default:
	case 8:
		dimm->bus_width = MEMORY_BUS_WIDTH_8;
		break;

	case 16:
		dimm->bus_width = MEMORY_BUS_WIDTH_16;
		break;

	case 32:
		dimm->bus_width = MEMORY_BUS_WIDTH_32;
		break;

	case 64:
		dimm->bus_width = MEMORY_BUS_WIDTH_64;
		break;
	}
}

void mainboard_save_dimm_info(struct romstage_params *params)
{
	struct dimm_info *dimm;
	struct memory_info *mem_info;

	/*
	 * Allocate CBMEM area for DIMM information used to populate SMBIOS
	 * table 17
	 */
	mem_info = cbmem_add(CBMEM_ID_MEMINFO, sizeof(*mem_info));
	printk(BIOS_DEBUG, "CBMEM entry for DIMM info: 0x%p\n", mem_info);
	if (mem_info == NULL)
		return;
	memset(mem_info, 0, sizeof(*mem_info));

	/* Describe the first channel memory */
	dimm = &mem_info->dimm[0];
	set_dimm_info(params->pei_data->spd_data_ch0, dimm);
	mem_info->dimm_cnt = 1;

	/* Describe the second channel memory */
	if (params->pei_data->spd_ch1_config == 1) {
		dimm = &mem_info->dimm[1];
		set_dimm_info(params->pei_data->spd_data_ch1, dimm);
		dimm->channel_num = 1;
		mem_info->dimm_cnt = 2;
	}
}
