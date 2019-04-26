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

#ifndef __SRC_DRIVERS_VIDEO_LED_LP5562__H__
#define __SRC_DRIVERS_VIDEO_LED_LP5562__H__

/* Key lp5562 registers. */
#define LP5562_ENABLE_REG       0x00
#define LP5562_OPMODE_REG       0x01
#define LP5562_PWMREG_B         0x02
#define LP5562_PWMREG_G         0x03
#define LP5562_PWMREG_R         0x04
#define LP5562_CURRENT_B        0x05
#define LP5562_CURRENT_G        0x06
#define LP5562_CURRENT_R        0x07
#define LP5562_CONFIG_REG       0x08
#define LP5562_ENGINE1_PC       0x09
#define LP5562_ENGINE2_PC       0x0a
#define LP5562_ENGINE3_PC       0x0b
#define LP5562_STATUS_REG       0x0c
#define LP5562_RESET_REG        0x0d
#define LP5562_PWMREG_W         0x0e
#define LP5562_CURRENT_W        0x0f
#define LP5562_LED_MAP_REG      0x70
#define LP5562_PROG_MEM_ENG1    0x10
#define LP5562_PROG_MEM_ENG2    0x30
#define LP5562_PROG_MEM_ENG3    0x50

/* LP5562 ENABLE REG fields. */
#define LP5562_ENABLE_LOG_EN         0x80
#define LP5562_ENABLE_CHIP_EN        0x40
#define LP5562_ENABLE_EXEC_HOLD      0x00
#define LP5562_ENABLE_EXEC_STEP      0x01
#define LP5562_ENABLE_EXEC_RUN       0x02
#define LP5562_ENABLE_EXEC_MASK      0x03
#define LP5562_ENABLE_ENG1_SHIFT     4
#define LP5562_ENABLE_ENG2_SHIFT     2
#define LP5562_ENABLE_ENG3_SHIFT     0
#define LP5562_ENABLE_ALL_MASK       0x3f
#define LP5562_ENABLE_ALL_HOLD       0x00
#define LP5562_ENABLE_ALL_RUN        0x2a

/* LP5562 OPMODE REG fields. */
#define LP5562_OPMODE_DISABLED       0x00
#define LP5562_OPMODE_LOAD           0x01
#define LP5562_OPMODE_RUN            0x02
#define LP5562_OPMODE_DIRECT         0x03
#define LP5562_OPMODE_MASK           0x03
#define LP5562_OPMODE_ENG1_SHIFT     4
#define LP5562_OPMODE_ENG2_SHIFT     2
#define LP5562_OPMODE_ENG3_SHIFT     0
#define LP5562_OPMODE_ALL_DISABLE    0x00
#define LP5562_OPMODE_ALL_LOAD       0x15
#define LP5562_OPMODE_ALL_RUN        0x2a

/*
 * LP5562_ENABLE_REG, default value
 */
#define LP5562_ENABLE_REG_DEFAULT    (LP5562_ENABLE_CHIP_EN)

/*
 * LP5562_CONFIG_REG, default value
 * PWM_HF=0(256Hz)
 * PWRSAVE_EN=0 (Disable)
 * CLKDET_EN/INT_CLK_EN=1 (Internal Clock)
 */
#define LP5562_CONFIG_REG_DEFAULT    0x03

/*
 * LP5562_LED_MAP_REG, default value
 * B:ENG1, G:ENG2, R:ENG3
 */
#define LP5562_LED_MAP_REG_DEFAULT   0x39

/* LP5562 Current default value, applies to all four of them */
#define LP5562_CRT_CTRL_DEFAULT      0xaf

/* Goes into LP5562_RESET_REG to reset the chip. */
#define LP5562_RESET_VALUE           0xff

/*
 * The controller has 96 bytes of SRAM for code/data, available as three 32 byte
 * pages.
 */
#define LP5562_PROG_PAGE_SIZE   32
#define LP5562_PROG_PAGES       3
#define LP5562_MAX_PROG_SIZE    LP5562_PROG_PAGE_SIZE

/*
 * Different types of display patterns to be shown by the LED while
 * controlled by coreboot.
 */
enum display_pattern {
	LED_ALL_OFF,		/* Turn the LEDs off. */
	LED_RECOVERY_PUSHED,	/* Recovery button push detected on start up. */
	LED_WIPEOUT_REQUEST,	/* Held long enough for wipout request. */
	LED_RECOVERY_REQUEST,	/* Held long enough for recovery request. */
	LED_NORMAL_BOOT		/* No buttons pressed, normal boot sequence. */
};
/*
 * led_lp5562_display_pattern
 *
 * Display pattern on the ring LEDs.
 */
int led_lp5562_display_pattern(unsigned int i2c_bus, enum display_pattern pattern);

#endif
