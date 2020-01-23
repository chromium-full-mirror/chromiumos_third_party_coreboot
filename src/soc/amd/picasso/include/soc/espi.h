/*
 * This file is part of the coreboot project.
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

#include <stdint.h>
/*
 * If 'update_slave' is set, then this will cause an eSPI bus transaction to
 * write to the slave's config registers. This is not needed if the setting
 * match the slave's defaults.
 */
struct espi_config {
	unsigned long espi_initial_mode;
	unsigned long bus_width;
	unsigned long espi_freq_mhz;
	unsigned int enable_crc_checking : 1;
	unsigned int alert_pin_on_io1 : 1;
	unsigned int peripheral_ch_en : 1;
	unsigned int virtual_wire_ch_en : 1;
	unsigned int out_of_band_ch_en : 1;
	unsigned int flash_ch_en : 1;
	unsigned int update_slave : 1;
};

struct espi_resource_allocator {
	int num_io_ranges;
	int num_mmio_ranges;
	struct resource io_ranges[4];
	struct resource mmio_ranges[4];
	unsigned int enable_0x2e_0x2f : 1;
	unsigned int enable_0x60_0x64 : 1;
	unsigned int enable_0x80 : 1;
};

struct espi_response {
	uint8_t hdata[7];
	uint8_t status : 1;
	uint8_t type : 2;
};

void espi_setup(const struct espi_config *cfg);
int espi_enable_resources(const struct resource *resource_linked_list);
void espi_enable_children_resources(struct device *espi);
void *espi_read_base_address(void);
void espi_show_configuration(void);
void dump_espi_regs(void);

/*
 * Picasso-specific eSPI host registers
 */
#define SPI_BAR_ADDRESS_MASK			0xffffff00
#define ESPI_OFFSET_FROM_BAR			0x10000

#define ESPI_DN_TXDR0				0x00
#define  ESPI_TX_CMD_SET_CONFIGURATION		(0 << 0)
#define  ESPI_TX_CMD_GET_CONFIGURATION		(1 << 0)
#define  ESPI_TX_CMD_RESET			(2 << 0)
#define  ESPI_VW_CMD				(5 << 0)
#define  ESPI_OOB_CMD				(6 << 0)
#define  ESPI_TX_GO_STATUS			(1 << 3)

#define ESPI_DN_TXDR1				0x04
#define ESPI_DN_TXDR2				0x08
#define ESPI_DN_TXDR3				0x0C

#define ESPI_DECODE				0x40
#define  ESPI_DECODE_MMIO_RANGE_EN(range)	(1 << (((range) & 3) + 12))
#define  ESPI_DECODE_IO_RANGE_EN(range)		(1 << (((range) & 3) + 8))
#define  ESPI_DECODE_IO_0x80_EN			BIT(2)
#define  ESPI_DECODE_IO_0X60_0X64_EN		BIT(1)
#define  ESPI_DECODE_IO_0X2E_0X2F_EN		BIT(0)

#define ESPI_IO_RANGE_BASE(range)		(0x44 + ((range) & 3) * 2)
#define ESPI_IO_RANGE_SIZE(range)		(0x4c + ((range) & 3))
#define ESPI_MMIO_RANGE_BASE(range)		(0x50 + ((range) & 3) * 4)
#define ESPI_MMIO_RANGE_SIZE(range)		(0x60 + ((range) & 3) * 2)

#define ESPI_SLAVE0_CONFIG			0x68
#define  ESPI_CRC_CHECKING_EN			BIT(31)
#define  ESPI_ALERT_MODE			BIT(30)
#define  ESPI_IO_MODE_SINGLE			(0x0 << 28)
#define  ESPI_IO_MODE_DUAL			(0x1 << 28)
#define  ESPI_IO_MODE_QUAD			(0x2 << 28)
#define  ESPI_OP_FREQ_16_MHZ			(0x0 << 25)
#define  ESPI_OP_FREQ_33_MHZ			(0x1 << 25)
#define  ESPI_OP_FREQ_66_MHZ			(0x2 << 25)
#define  ESPI_PR_EN				BIT(3)
#define  ESPI_VIRTUAL_WIRE_CH_EN		BIT(2)
#define  ESPI_OOB_CH_EN				BIT(1)
#define  ESPI_FLASH_CH_EN			BIT(0)

#define SLAVE0_INT_STS				0x70
#define  ESPI_STATUS_RXMSG				BIT(29)
#define  ESPI_STATUS_DNCMD				BIT(28)
#define  ESPI_STATUS_NON_FATAL_ERROR			BIT(6)
#define  ESPI_STATUS_FATAL_ERROR			BIT(5)
#define  ESPI_STATUS_NO_RESPONSE			BIT(4)
#define  ESPI_STATUS_CRC_ERR				BIT(2)
#define  ESPI_STATUS_WAIT_TIMEOUT			BIT(1)
#define  ESPI_STATUS_BUS_ERROR				BIT(0)

#define ESPI_RXVW_				0x9C
#define ESPI_RXVW_DATA				0xA0
#define ESPI_RXVW_INDEX				0xA4
#define ESPI_RXVW_MISC_CNTL			0xA8
#define ESPI_RXVW_POLARITY			0xAC

/*
- * Standard eSPI slave registers
- */
#define ESPI_SLAVE_GENERAL_CFG			0x08
#define  ESPI_SLAVE_CRC_CHECKING_EN			BIT(31)
#define  ESPI_SLAVE_RESP_MOD_EN				BIT(30)
#define  ESPI_SLAVE_ALERT_MODE				BIT(28)
#define  ESPI_SLAVE_IO_MODE_SINGLE			(0x0 << 26)
#define  ESPI_SLAVE_IO_MODE_DUAL			(0x1 << 26)
#define  ESPI_SLAVE_IO_MODE_QUAD			(0x2 << 26)
#define  ESPI_SLAVE_QUAD_IO_SUPPORTED			BIT(25)
#define  ESPI_SLAVE_DUAL_IO_SUPPORTED			BIT(24)
#define  ESPI_SLAVE_OPEN_DRAIN_ALERT			BIT(23)
#define  ESPI_SLAVE_OP_FREQ_20_MHZ			(0x0 << 20)
#define  ESPI_SLAVE_OP_FREQ_25_MHZ			(0x1 << 20)
#define  ESPI_SLAVE_OP_FREQ_33_MHZ			(0x2 << 20)
#define  ESPI_SLAVE_OP_FREQ_50_MHZ			(0x3 << 20)
#define  ESPI_SLAVE_OP_FREQ_66_MHZ			(0x4 << 20)
#define  ESPI_SLAVE_SUPP_OPEN_DRAIN_ALERT		BIT(19)
#define  ESPI_SLAVE_SUPP_FREQ_20_MHZ			(0x0 << 16)
#define  ESPI_SLAVE_SUPP_FREQ_25_MHZ			(0x1 << 16)
#define  ESPI_SLAVE_SUPP_FREQ_33_MHZ			(0x2 << 16)
#define  ESPI_SLAVE_SUPP_FREQ_50_MHZ			(0x3 << 16)
#define  ESPI_SLAVE_SUPP_FREQ_66_MHZ			(0x4 << 16)
#define  ESPI_SLAVE_MAX_WAIT_MASK			(0xf << 12)
#define  ESPI_SLAVE_MAX_WAIT_STATE(x)			(((x) << 12) & ESPI_MAX_WAIT_MASK)
#define  ESPI_SLAVE_SUPP_PERIPHERAL_CH			BIT(0)
#define  ESPI_SLAVE_SUPP_VIRTUAL_WIRE_CH		BIT(1)
#define  ESPI_SLAVE_SUPP_OOB_CH				BIT(2)
#define  ESPI_SLAVE_SUPP_FLASH_CH			BIT(3)

#define ESPI_SLAVE_CHAN0_CFG				0x10	/* Peripheral channel */
#define ESPI_SLAVE_CHAN1_CFG				0x20	/* Virtual Wire channel */
#define ESPI_SLAVE_CHAN2_CFG				0x30	/* OOB Channel */
#define ESPI_SLAVE_CHAN3_CFG				0x40	/* Flash Access channel */
#define  ESPI_SLAVE_CHANNEL_ENABLE			BIT(0)
#define  ESPI_SLAVE_CHANNEL_READY			BIT(1)
#define  ESPI_SLAVE_PERIPH_BM_ENABLE			BIT(2)

/* Virtual Wire definitions */

struct vw_config_def {
	uint8_t		index;
	uint8_t		valid_bit;
	uint8_t		enable_bit;
	const char	*configname;
};

//					index,valid,enable,name
#define VW_PLTRST_CONFIG		0x03, 0x05, 0x01, "PLTRST"


#endif /* AMD_PICASSO_ESPI_H */
