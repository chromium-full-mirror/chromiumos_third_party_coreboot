/*
 * Copyright (C) 2019 Google, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * This is a driver for the TI LP5562 (http://www.ti.com/product/lp5562),
 * driving a tri-color LED.
 *
 * The only connection between the LED and the main board is an i2c bus.
 *
 * This driver imitates a depthcharge display device. On initialization the
 * driver sets up the controllers to prepare them to accept programs to run.
 *
 * When a certain vboot state needs to be indicated, the program for that
 * state is loaded into the controllers, resulting in the state appropriate
 * LED behavior.
 */

#include <stddef.h>
#include "drivers/i2c/lp5562/led_lp5562_programs.h"
#include "drivers/i2c/lp5562/led_lp5562_calibration.h"

/****************************************************************
 *   LED ring program definitions for different patterns.
 *
 * Comments below are real lp5562 source code, they are compiled using
 * lasm.exe, the TI tool available from their Web site (search for lp5562)
 * and running only on Windows :P.
 *
 * Different hex dumps are results of tweaking the source code parameters to
 * achieve desirable LED ring behavior. It is possible to use just one code
 * dump and replace certain values in the body to achieve different behaviour
 * with the same basic dump, but keeping track of location to tweak with every
 * code change would be quite tedious.
 */

/*
 * Solid LED display, the arguments of the set_pwm commands set intensity and
 * color of the display:

  1 00		.ENGINE1(R)
  2 00 4080		set_pwm 128
  3 01 C000		end
  4
  5 10		.ENGINE2(G)
  6 10 4080		set_pwm 128
  7 11 C000		end
  8
  9 20		.ENGINE3(B)
 10 20 4080		set_pwm 128
 11 21 C000		end
*/

/* RGB set to 000000, resulting in all LEDs off. */
static uint8_t solid_00_text[] = {
	0x40,    0, 0xc0, 0x00
};

static ti_lp5562_program solid_000000_program = {
	{
		{ /* Engine1:Blue */
			solid_00_text,
			sizeof(solid_00_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
		{ /* Engine2:Green */
			solid_00_text,
			sizeof(solid_00_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
		{ /* Engine3:Red */
			solid_00_text,
			sizeof(solid_00_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
	}
};

/*
 * fdr_press1.src
 *
  1 00		.ENGINE1(B)
  2 00 4000		set_pwm 0
  3		loop1_1:
  4 01 E208		trigger w3,s3
  5 02 4000		set_pwm 0
  6 03 4000		set_pwm 0
  7 04 E208		trigger w3,s3
  8 05 4000		set_pwm 0
  9 06 4000		set_pwm 0
 10 07 A001		branch 0, loop1_1
 11
 12 10		.ENGINE2(G)
 13 10 4000		set_pwm 0
 14		loop2_1:
 15 11 E208		trigger w3,s3
 16 12 0565		ramp 250, 102
 17 13 0565		ramp 250, 102
 18 14 E208		trigger w3,s3
 19 15 05E5		ramp 250, -102
 20 16 05E5		ramp 250, -102
 21 17 A011		branch 0,loop2_1
 22
 23 20		.ENGINE3(R)
 24 20 4000		set_pwm 0
 25		loop3_1:
 26 21 E186		trigger s21,w21
 27 22 047E		ramp 250, 127
 28 23 047F		ramp 250, 128
 29 24 E186		trigger s21,w21
 30 25 04FE		ramp 250, -127
 31 26 04FF		ramp 250, -128
 32 27 A021		branch 0,loop3_1
*/

/* WWR_RECOVERY_PUSHED */
/* Fast Blinking Yellow */
/* Ramp up to (255,204,0) in 0.5sec, ramp down to (0,0,0) in 0.5sec */
static uint8_t fdr_press1_b_text[] = {
	0x40,  0x00,  0xE2,  0x08,  0x40,  0x00,  0x40,  0x00,
	0xE2,  0x08,  0x40,  0x00,  0x40,  0x00,  0xA0,  0x01,
};

static uint8_t fdr_press1_g_text[] = {
	0x40,  0x00,  0xE2,  0x08,  0x05,  0x65,  0x05,  0x65,
	0xE2,  0x08,  0x05,  0xE5,  0x05,  0xE5,  0xA0,  0x11,
};

static uint8_t fdr_press1_r_text[] = {
	0x40,  0x00,  0xE1,  0x86,  0x04,  0x7E,  0x04,  0x7F,
	0xE1,  0x86,  0x04,  0xFE,  0x04,  0xFF,  0xA0,  0x21,
};

static ti_lp5562_program fdr_press1_program = {
	{
		{
			fdr_press1_b_text,
			sizeof(fdr_press1_b_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
		{
			fdr_press1_g_text,
			sizeof(fdr_press1_g_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
		{
			fdr_press1_r_text,
			sizeof(fdr_press1_r_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
	}
};

/*
 * wipeout_request1.src
 *
  1 00		.ENGINE1(B)
  2 00 4000		set_pwm 0
  3		loop1_1:
  4 01 E208		trigger w3,s3
  5 02 4000		set_pwm 0
  6 03 4000		set_pwm 0
  7 04 E208		trigger w3,s3
  8 05 4000		set_pwm 0
  9 06 4000		set_pwm 0
 10 07 A001		branch 0, loop1_1
 11
 12 10		.ENGINE2(G)
 13 10 4000             set_pwm 0
 14		loop2_1:
 15 11 E208		trigger w3,s3
 16 12 0565		ramp 250, 102
 17 13 0565		ramp 250, 102
 18 14 E208		trigger w3,s3
 19 15 05E5		ramp 250, -102
 20 16 05E5		ramp 250, -102
 21 17 A011		branch 0,loop2_1
 22
 23 20		.ENGINE3(R)
 24 20 4000             set_pwm 0
 25		loop3_2:
 26 21 6000             wait 500
 27 22 A321             branch 6,loop3_2
 28		loop3_1:
 29 23 E186		trigger s21,w21
 30 24 047E		ramp 250, 127
 31 25 047F		ramp 250, 128
 32 26 E186		trigger s21,w21
 33 27 04FE		ramp 250, -127
 34 28 04FF		ramp 250, -128
*/

/* WWR_WIPEOUT_REQUEST */
/* Fast Blinking Yellow with 3sec delay */
/* Blank for 3sec */
/* Ramp up to (255,204,0) in 0.5sec, ramp down to (0,0,0) in 0.5sec */
static uint8_t wipeout_request1_b_text[] = {
	0x40,  0x00,  0xE2,  0x08,  0x40,  0x00,  0x40,  0x00,
	0xE2,  0x08,  0x40,  0x00,  0x40,  0x00,  0xA0,  0x01,
};

static uint8_t wipeout_request1_g_text[] = {
	0x40,  0x00,  0xE2,  0x08,  0x05,  0x65,  0x05,  0x65,
	0xE2,  0x08,  0x05,  0xE5,  0x05,  0xE5,  0xA0,  0x11,
};

static uint8_t wipeout_request1_r_text[] = {
	0x40,  0x00,  0x60,  0x00,  0xA3,  0x21,  0xE1,  0x86,
	0x04,  0x7E,  0x04,  0x7F,  0xE1,  0x86,  0x04,  0xFE,
	0x04,  0xFF,  0xA0,  0x23,
};

static ti_lp5562_program wipeout_request1_program = {
	{
		{
			wipeout_request1_b_text,
			sizeof(wipeout_request1_b_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
		{
			wipeout_request1_g_text,
			sizeof(wipeout_request1_g_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
		{
			wipeout_request1_r_text,
			sizeof(wipeout_request1_r_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
	}
};

/*
 * blink_recovery1.src
 *
  1 00		.ENGINE1(B)
  2 00 E200		trigger w3
  3 01 4000		set_pwm 0
  4 02 E200		trigger w3
  5 03 4000		set_pwm 0
  6 04 0000		gotostart
  7
  8 10		.ENGINE2(G)
  9 10 E200		trigger w3
 10 11 40CC		set_pwm 0
 11 12 E200		trigger w3
 12 13 4000		set_pwm 0
 13 14 0000		gotostart
 14
 15 20		.ENGINE3(R)
 16 20 E006		trigger s21
 17 21 40FF		set_pwm 255
 18 22 5300		wait 300
 19 23 E006		trigger s21
 20 24 4000		set_pwm 0
 21 25 5300		wait 300
 22 26 0000		gotostart
*/

/* WWR_RECOVERY_REQUEST */
/* Blinking Red, (255,0,0) for 0.3sec, (0,0,0) for 0.3sec */
static uint8_t blink_recovery1_b_text[] = {
	0xe2,  0x00,  0x40,     0,  0xe2,  0x00,  0x40,     0,
	0x00,  0x00,
};

static uint8_t blink_recovery1_g_text[] = {
	0xe2,  0x00,  0x40,     0,  0xe2,  0x00,  0x40,     0,
	0x00,  0x00,
};

static uint8_t blink_recovery1_r_text[] = {
	0xe0,  0x06,  0x40,   255,  0x53,  0x00,  0xe0,  0x06,
	0x40,     0,  0x53,  0x00,  0x00,  0x00
};

static ti_lp5562_program blink_recovery1_program = {
	{
		{
			blink_recovery1_b_text,
			sizeof(blink_recovery1_b_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
		{
			blink_recovery1_g_text,
			sizeof(blink_recovery1_g_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
		{
			blink_recovery1_r_text,
			sizeof(blink_recovery1_r_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
	}
};

/*
 * preboot1.src
 *
  1 00		.ENGINE1(B)
  2 00 4000		set_pwm 0
  3 01 E200		trigger w3
  4 02 0D4C		ramp 500, 77
  5 03 C000		end
  6
  7 10		.ENGINE2(G)
  8 10 4000		set_pwm 0
  9 11 E200		trigger w3
 10 12 0D4C		ramp 500, 77
 11 13 C000		end
 12
 13 20		.ENGINE3(R)
 14 20 4000		set_pwm 0
 15 21 E006		trigger s21
 16 22 0D4C		ramp 500, 77
 17 23 C000		end
*/

/* WWR_NORMAL_BOOT */
/* Ramp up to (77,77,77) in 0.5sec */
static uint8_t preboot1_b_text[] = {
	0x40,  0x00,  0xe2,  0x00,  0x0d,  0x4c,  0xc0,  0x00
};
static uint8_t preboot1_g_text[] = {
	0x40,  0x00,  0xe2,  0x00,  0x0d,  0x4c,  0xc0,  0x00
};
static uint8_t preboot1_r_text[] = {
	0x40,  0x00,  0xe0,  0x06,  0x0d,  0x4c,  0xc0,  0x00
};

static ti_lp5562_program preboot1_program = {
	{
		{
			preboot1_b_text,
			sizeof(preboot1_b_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
		{
			preboot1_g_text,
			sizeof(preboot1_g_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
		{
			preboot1_r_text,
			sizeof(preboot1_r_text),
			0,
			LED_LP5562_DEFAULT_CURRENT
		},
	}
};

const led5562_state_prog led_lp5562_state_programs[] = {
	{LED_ALL_OFF, {&solid_000000_program} },
	{LED_RECOVERY_PUSHED, {&fdr_press1_program} },
	{LED_WIPEOUT_REQUEST, {&wipeout_request1_program} },
	{LED_RECOVERY_REQUEST, {&blink_recovery1_program} },
	{LED_NORMAL_BOOT, {&preboot1_program} },
	{}, /* Empty record to mark the end of the table. */
};

/*
 * Calibration code map for "solid" pattern.
 * Set PWM values then stop.
 */
const struct lp5562_calibration_code_map mistral_code_map_solid[] = {
	{
		blue,
		set_pwm,
		0x00,
		0,
		{ }
	},
	{
		green,
		set_pwm,
		0x10,
		0,
		{ }
	},
	{
		red,
		set_pwm,
		0x20,
		0,
		{ }
	},
	{
		0,
		invalid,
		0x00,
		0,
		{ }
	}
};

/*
 * Calibration code map for "blink" pattern.
 * Set PWM values, wait, set PWMs to 0, wait, loop.
 */
const struct lp5562_calibration_code_map mistral_code_map_blink[] = {
	{
		blue,
		set_pwm,
		0x01,
		0,
		{ }
	},
	{
		green,
		set_pwm,
		0x11,
		0,
		{ }
	},
	{
		red,
		set_pwm,
		0x21,
		0,
		{ }
	},
	{
		0,
		invalid,
		0x00,
		0,
		{ }
	}
};

/*
 * Calibration code map for "ramp up" pattern.
 * Start from OFF, ramp up in 0.5sec
 */
const struct lp5562_calibration_code_map mistral_code_map_ramp_up[] = {
	{
		blue,
		ramp,
		0x02,
		1,
		{ {500, 256}, }
	},
	{
		green,
		ramp,
		0x12,
		1,
		{ {500, 256}, }
	},
	{
		red,
		ramp,
		0x22,
		1,
		{ {500, 256}, }
	},
	{
		0,
		invalid,
		0x00,
		0,
		{ }
	}
};

// Calibration code map for "fast blink" pattern.
// Start from OFF,
// ramp up in 0.5sec, ramp down in 0.5sec, repeat forever.
const struct lp5562_calibration_code_map mistral_code_map_fast_blink[] = {
	{
		blue,
		ramp,
		0x02,
		2,
		{ {250, 128}, {250, 128} }
	},
	{
		blue,
		ramp,
		0x05,
		2,
		{ {250, -128}, {250, -128} }
	},
	{
		green,
		ramp,
		0x12,
		2,
		{ {250, 128}, {250, 128} }
	},
	{
		green,
		ramp,
		0x15,
		2,
		{ {250, -128}, {250, -128} }
	},
	{
		red,
		ramp,
		0x22,
		2,
		{ {250, 128}, {250, 128} }
	},
	{
		red,
		ramp,
		0x25,
		2,
		{ {250, -128}, {250, -128} }
	},
	{
		0,
		invalid,
		0x00,
		0,
		{ }
	}
};

// Calibration code map for "wipeout request" pattern.
// Start from OFF, wait for 3 seconds
// ramp up in 0.5sec, ramp down in 0.5sec, repeat forever.
const struct lp5562_calibration_code_map mistral_code_map_wipeout_request1[] = {
	{
		blue,
		ramp,
		0x02,
		2,
		{ {250, 128}, {250, 128} }
	},
	{
		blue,
		ramp,
		0x05,
		2,
		{ {250, -128}, {250, -128} }
	},
	{
		green,
		ramp,
		0x12,
		2,
		{ {250, 128}, {250, 128} }
	},
	{
		green,
		ramp,
		0x15,
		2,
		{ {250, -128}, {250, -128} }
	},
	{
		red,
		ramp,
		0x24,
		2,
		{ {250, 128}, {250, 128} }
	},
	{
		red,
		ramp,
		0x27,
		2,
		{ {250, -128}, {250, -128} }
	},
	{
		0,
		invalid,
		0x00,
		0,
		{ }
	}
};

const struct lp5562_calibration_data mistral_calibration_database[] = {
	{
		&fdr_press1_program,
		1, /* Yellow */
		100, /* 100% brigheness */
		mistral_code_map_fast_blink,
	},
	{
		&preboot1_program,
		0, /* White */
		30, /* 30% brigheness */
		mistral_code_map_ramp_up,
	},
	{
		&blink_recovery1_program,
		2, /* Red */
		100, /* 100% brightness */
		mistral_code_map_blink,
	},
	{
		&wipeout_request1_program,
		1, /* Yellow */
		100, /* 100% brightness */
		mistral_code_map_wipeout_request1,
	},
	{
		NULL,
		0,
		0,
		NULL
	}
};
