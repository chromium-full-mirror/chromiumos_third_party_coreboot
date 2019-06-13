/*
 * This file is part of the coreboot project.
 *
 * Copyright 2019 Google LLC
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
#include <string.h>
#include <fmap.h>
#include "drivers/i2c/lp5562/led_lp5562_programs.h"
#include "drivers/i2c/lp5562/led_lp5562_calibration.h"
#include "led_calibration.h"

/* Number of predefined colors */
#define NDEFCOLORS 3

const char *color_names[NDEFCOLORS] = {
	"white",
	"yellow",
	"red"
};

struct lp5562_calibrated_color calibrated_colors[NDEFCOLORS] = {
	{ {255, 255, 255}, {120, 120, 120} },
	{ {255, 204,   0}, {120, 120, 120} },
	{ {255,   0,   0}, {120, 120, 120} },
};

static const char key_name[] = "led_calibration";

/* Some constants of VPD2.0 format */
enum {
	GOOGLE_VPD_2_0_OFFSET = 0x600,
	VPD_TYPE_STRING = 0x01,
	VPD_TYPE_INFO = 0xfe,
};

static int decode_vpd_length(uint8_t *vpd_ptr, int *offset_p, int max_len);
static int load_caldata_from_vpd(char *caldata_buff, int caldata_size);

/* Decode VPD length from given buffer */
static int decode_vpd_length(uint8_t *vpd_ptr, int *offset_p, int max_len)
{
	int offset = *offset_p;
	int len = 0;
	uint8_t d;

	do {
		d = vpd_ptr[offset++];
		len *= 128;
		len += (d & 0x7f);
	} while ((d & 0x80) && (offset < max_len));

	if (d & 0x80)
		len = -1;
	*offset_p  = offset;
	return len;
}

/* Find and read LED calibration data from VPD */
static int load_caldata_from_vpd(char *caldata_buff, int caldata_size)
{
	struct region_device rdev;
	size_t offset;
	size_t size;
	ssize_t read_len;
	int buff_offset;
	int len;
	uint8_t buff[256];
	int led_caldata_found = 0;

	if (fmap_locate_area_as_rdev("RO_VPD", &rdev))
		return 0;

	/* Search from VPD2.0 fixed offset */
	offset = GOOGLE_VPD_2_0_OFFSET;
	size = rdev.region.size;
	while (!led_caldata_found && offset < size) {
		uint8_t type;

		/* Read key length */
		read_len = rdev_readat(&rdev, buff, offset, 4);
		if (read_len < 0)
			break;
		type = buff[0];
		if ((type != VPD_TYPE_STRING) && (type != VPD_TYPE_INFO))
			break;
		buff_offset = 1;
		len = decode_vpd_length(buff, &buff_offset, (size - offset));
		if (len < 0)
			break;

		offset += buff_offset;
		if (len < sizeof(buff)) {
			/* Read key */
			read_len = rdev_readat(&rdev, buff, offset, len);
			if (read_len < 0)
				break;
			buff[len] = 0;
			if ((type == VPD_TYPE_STRING) &&
			    (strcmp((char *)buff, key_name) == 0)) {
				led_caldata_found = 1;
			}
		}
		offset += len;

		/* Read value length */
		read_len = rdev_readat(&rdev, buff, offset, 4);
		if (read_len < 0)
			break;
		buff_offset = 0;
		len = decode_vpd_length(buff, &buff_offset, (size - offset));
		if (len < 0)
			break;

		offset += buff_offset;
		if (led_caldata_found) {
			if (len >= caldata_size) {
				printk(BIOS_ERR,
					"LED_LP5562: Caldata too large(%d)\n",
					len);
				break;
			}

			/* Read value */
			read_len =
				rdev_readat(&rdev, caldata_buff, offset, len);
			if (read_len < 0)
				break;
			led_caldata_found = 2;
		}
		offset += len;
	}
	return (led_caldata_found == 2);
}

int calibrate_led(void)
{
	const char *separators = " ,;";
	char buff[256];
	char *p = buff;
	int idx;

	if (!load_caldata_from_vpd(buff, sizeof(buff))) {
		printk(BIOS_INFO, "LED_LP5562: Valid caldata not found\n");
		return 0;
	}

	/*
	 * Parse calibration data
	 * Calibration data is list of entries separated by semicolon(;)
	 * An entry is list of values separated by commna(,)
	 * first value is ASCII string color name,
	 * second value is ASCII decimal string of current,
	 * 3rd, 4th, 5th values are brightness of Red, Green and Blue.
	 */
	while (*p != 0) {
		int color;
		int rgb;
		int len;

		len = strcspn(p, separators);
		if (len < 1)
			break;
		for (color = 0; color < NDEFCOLORS; color++) {
			if (strncmp(color_names[color], p, len) == 0)
				break;
		}
		if (color >= NDEFCOLORS) {
			len = strcspn(p, ";");
			if (len < 0)
				break;
			p += (len + 1);
			continue;
		}

		p += len;
		p += strspn(p, separators);
		calibrated_colors[color].current_values[0] =
		calibrated_colors[color].current_values[1] =
		calibrated_colors[color].current_values[2] = atol(p);

		p += strcspn(p, separators);
		p += strspn(p, separators);
		for (rgb = 0; rgb < 3; rgb++) {
			calibrated_colors[color].pwm_values[rgb] = atol(p);
			p += strcspn(p, separators);
			p += strspn(p, separators);
		}
	}

	/*
	 * Apply calibration data
	 */
	idx = 0;
	while (mistral_calibration_database[idx].pattern != NULL) {
		led_lp5562_calibrate(
			&mistral_calibration_database[idx],
			calibrated_colors);
		idx++;
	}
	return 0;
}
