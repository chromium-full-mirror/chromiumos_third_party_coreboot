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

#include <console/console.h>
#include <device/pci_ops.h>
#include <soc/espi.h>
#include <soc/pci_devs.h>
#include <stdint.h>
#include <string.h>
#include <timer.h>


/*
 * Picasso-specific eSPI host registers
 */
#define SPI_BAR_ADDRESS_MASK	0xffffff00
#define ESPI_OFFSET_FROM_BAR	0x10000

#define ESPI_TX_HEADER				0x00
#define  ESPI_TX_CMD_TYPE			(7 << 0)
#define  ESPI_TX_CMD_SET_CONFIGURATION		(0 << 0)
#define  ESPI_TX_CMD_GET_CONFIGURATION		(1 << 0)
#define  ESPI_TX_CMD_RESET			(2 << 0)
#define  ESPI_TX_GO_STATUS			(1 << 3)

#define ESPI_DECODE				0x40
#define  ESPI_DECODE_MMIO_RANGE_EN(range)	(1 << (((range) & 3) + 12))
#define  ESPI_DECODE_IO_RANGE_EN(range)		(1 << (((range) & 3) + 8))
#define  ESPI_DECODE_IO_0X60_0X64_EN		(1 <<  1)
#define  ESPI_DECODE_IO_0X2E_0X2F_EN		(1 <<  0)

#define ESPI_IO_RANGE_BASE(range)		(0x44 + ((range) & 3) * 2)
#define ESPI_IO_RANGE_SIZE(range)		(0x4c + ((range) & 3))
#define ESPI_MMIO_RANGE_BASE(range)		(0x50 + ((range) & 3) * 4)
#define ESPI_MMIO_RANGE_SIZE(range)		(0x60 + ((range) & 3) * 2)

#define ESPI_SLAVE0_CONFIG			0x68

/*
 * Standard eSPI slave registers
 */
#define ESPI_SLAVE_REG_CFG			0x08
#define  ESPI_CRC_CHECKING_EN			(1 << 31)
#define  ESPI_RESP_MOD_EN			(1 << 30)
#define  ESPI_ALERT_MODE			(1 << 28)
#define  ESPI_IO_MODE_SINGLE			(0x0 << 26)
#define  ESPI_IO_MODE_DUAL			(0x1 << 26)
#define  ESPI_IO_MODE_QUAD			(0x2 << 26)
#define  ESPI_QUAD_IO_SUPPORTED			(1 << 25)
#define  ESPI_DUAL_IO_SUPPORTED			(1 << 24)
#define  ESPI_OPEN_DRAIN_ALERT			(1 << 23)
#define  ESPI_OP_FREQ_20_MHZ			(0x0 << 20)
#define  ESPI_OP_FREQ_25_MHZ			(0x1 << 20)
#define  ESPI_OP_FREQ_33_MHZ			(0x2 << 20)
#define  ESPI_OP_FREQ_50_MHZ			(0x3 << 20)
#define  ESPI_OP_FREQ_66_MHZ			(0x4 << 20)
#define  ESPI_SUPP_OPEN_DRAIN_ALERT		(1 << 19)
#define  ESPI_SUPP_FREQ_20_MHZ			(0x0 << 16)
#define  ESPI_SUPP_FREQ_25_MHZ			(0x1 << 16)
#define  ESPI_SUPP_FREQ_33_MHZ			(0x2 << 16)
#define  ESPI_SUPP_FREQ_50_MHZ			(0x3 << 16)
#define  ESPI_SUPP_FREQ_66_MHZ			(0x4 << 16)
#define  ESPI_MAX_WAIT_MASK			(0xf << 12)
#define  ESPI_MAX_WAIT_STATE(x)			(((x) << 12) & ESPI_MAX_WAIT_MASK)
#define  ESPI_SUPP_PERIPHERAL_CH		(0 << 0)
#define  ESPI_SUPP_VIRTUAL_WIRE_CH		(1 << 0)
#define  ESPI_SUPP_OOB_CH			(2 << 0)
#define  ESPI_SUPP_FLASH_CH			(3 << 0)


struct espi_resource_allocator {
	int num_io_ranges;
	int num_mmio_ranges;
	struct resource io_ranges[4];
	struct resource mmio_ranges[4];
	unsigned int enable_0x2e_0x2f : 1;
	unsigned int enable_0x60_0x64 : 1;
};


static bool espi_wait_ready(uint8_t *espi)
{
	struct stopwatch sw;

	stopwatch_init_usecs_expire(&sw, 100);
	do {
		if (!(read32(espi + ESPI_TX_HEADER) & ESPI_TX_GO_STATUS))
			return true;
	} while (!stopwatch_expired(&sw));

	return false;
}

static void espi_set_configuration(uint8_t *espi, uint16_t addr, uint32_t val)
{
	uint32_t tx_ctl;

	/* How ironic that Intel designed eSPI to be big endian. */
	tx_ctl = addr & 0xff00;
	tx_ctl |= (addr & 0xff) << 16;
	tx_ctl |= (0x0 << 4);	/* Slave select -- always 0 */
	tx_ctl |= ESPI_TX_CMD_SET_CONFIGURATION;
	tx_ctl |= ESPI_TX_GO_STATUS;

	if (!espi_wait_ready(espi))
		return;

	write32(espi + ESPI_TX_HEADER + 4, val);
	write32(espi + ESPI_TX_HEADER, tx_ctl);
	espi_wait_ready(espi);
}

static void espi_setup_slave(uint8_t *espi, const struct espi_config *cfg)
{
	uint32_t slave_cfg_reg = 0;

	slave_cfg_reg |= cfg->enable_crc_checking ? ESPI_CRC_CHECKING_EN : 0;
	slave_cfg_reg |= cfg->alert_pin_on_io1	? 0 : ESPI_ALERT_MODE;

	slave_cfg_reg |= cfg->bus_width << 26;

	if (cfg->espi_freq_mhz <= 16)
		slave_cfg_reg |= ESPI_OP_FREQ_20_MHZ;
	else if (cfg->espi_freq_mhz <= 33)
		slave_cfg_reg |= ESPI_OP_FREQ_33_MHZ;
	else
		slave_cfg_reg |= ESPI_OP_FREQ_66_MHZ;

	espi_set_configuration(espi, ESPI_SLAVE_REG_CFG, slave_cfg_reg);
}

static bool is_0x2e_0x2f(uintptr_t base)
{
	return ((base & ~0x01) == 0x2e);
}

static bool is_0x60_0x64(uintptr_t base)
{
	return ((base & ~0x04) == 0x60);
}

/*
 * Contrary to the ESPI_BASE_ADDRESS macro in iomap.h,
 * this is a not a fixed resource.
 */
static void *espi_read_base_address(void)
{
	uintptr_t spi_espi_bar, espi;

	spi_espi_bar = pci_read_config32(SOC_LPC_DEV, 0xa0);
	espi = (spi_espi_bar & SPI_BAR_ADDRESS_MASK) + ESPI_OFFSET_FROM_BAR;
	return (void *)espi;
}

void espi_setup(const struct espi_config *cfg)
{
	uint32_t cfg_reg = 0;
	uint8_t *espi = espi_read_base_address();
	struct espi_config adjusted_cfg = *cfg;

	cfg_reg |= cfg->enable_crc_checking	? (1 << 31) : 0;
	cfg_reg |= cfg->alert_pin_on_io1	? 0 : (1 << 30);

	if (cfg->bus_width > ESPI_QUAD_IO)
		adjusted_cfg.bus_width = ESPI_SINGLE_IO;

	cfg_reg |= adjusted_cfg.bus_width << 28;

	if (cfg->espi_freq_mhz <= 16) {
		cfg_reg |= 0 << 25;
		adjusted_cfg.espi_freq_mhz = 16;
	} else if (cfg->espi_freq_mhz <= 33) {
		cfg_reg |= 1 << 25;
		adjusted_cfg.espi_freq_mhz = 33;
	} else {
		cfg_reg |= 2 << 25;
		adjusted_cfg.espi_freq_mhz = 66;
	}

	cfg_reg |= cfg->peripheral_ch_en	? (1 << 3) : 0;
	cfg_reg |= cfg->virtual_wire_ch_en	? (1 << 2) : 0;
	cfg_reg |= cfg->out_of_band_ch_en	? (1 << 1) : 0;
	cfg_reg |= cfg->flash_ch_en		? (1 << 0) : 0;

	write32(espi + ESPI_SLAVE0_CONFIG, cfg_reg);

	if (cfg->update_slave)
		espi_setup_slave(espi, &adjusted_cfg);
}

static int espi_allocate_io(struct espi_resource_allocator *allocation,
			    const struct resource *resource)
{
	if (is_0x2e_0x2f(resource->base) && resource->size == 1) {
		allocation->enable_0x2e_0x2f = true;
		return 1;
	}

	if (is_0x60_0x64(resource->base) && resource->size == 1) {
		allocation->enable_0x60_0x64 = true;
		return 1;
	}

	/* No more Mr. Nice bits. Will need to open a variable IO window. */
	if (allocation->num_io_ranges >= 4) {
		printk(BIOS_ERR, "ESPI: Out of IO ranges!!!\n");
		return 0;
	}

	allocation->io_ranges[allocation->num_io_ranges++] = *resource;
	return 1;
}

static int espi_allocate_mmio(struct espi_resource_allocator *allocation,
			      const struct resource *resource)
{
	if (allocation->num_mmio_ranges >= 4) {
		printk(BIOS_ERR, "ESPI: Out of MMIO ranges!!!\n");
		return 0;
	}

	allocation->mmio_ranges[allocation->num_mmio_ranges++] = *resource;
	return 1;
}

static void espi_write_resources(struct espi_resource_allocator *allocation)
{
	uint32_t espi_capabilities, decode_enable = 0;
	uint8_t *espi = espi_read_base_address();
	int i;

	espi_capabilities = read32(espi);
	if (espi_capabilities == ~0)
		printk(BIOS_ERR, "ESPI controller appears to be unpowered!\n");

	for (i = 0; i < allocation->num_io_ranges; i++) {
		decode_enable |= ESPI_DECODE_IO_RANGE_EN(i);
		write16(espi + ESPI_IO_RANGE_BASE(i),
			allocation->io_ranges[i].base);
		write8(espi + ESPI_IO_RANGE_SIZE(i),
			allocation->io_ranges[i].size - 1);
	}

	for (i = 0; i < allocation->num_mmio_ranges; i++) {
		decode_enable |= ESPI_DECODE_MMIO_RANGE_EN(i);
		write32(espi + ESPI_MMIO_RANGE_BASE(i),
			allocation->mmio_ranges[i].base);
		write16(espi + ESPI_MMIO_RANGE_SIZE(i),
			allocation->mmio_ranges[i].size - 1);
	}

	if (allocation->enable_0x2e_0x2f)
		decode_enable |= ESPI_DECODE_IO_0X2E_0X2F_EN;

	if (allocation->enable_0x60_0x64)
		decode_enable |= ESPI_DECODE_IO_0X60_0X64_EN;

	write32(espi + ESPI_DECODE, decode_enable);
}

static int espi_allocate(struct espi_resource_allocator *allocation,
			 const struct resource *resource_linked_list)
{
	int ret, num_total_ranges = 0;
	const struct resource *res;

	for (res = resource_linked_list; res; res = res->next) {
		ret = 0;

		if (res->flags & IORESOURCE_IO && res->size)
			ret = espi_allocate_io(allocation, res);
		else if (res->flags & IORESOURCE_MEM && res->size)
			ret = espi_allocate_mmio(allocation, res);

		num_total_ranges += ret;
	}

	return num_total_ranges;
}
/*
 * Enable resources specidied by resource_list.
 * 'resource_list is a linked' list describing resources to be enabled. Only IO
 * and MEM resources are decoded. This function is not additive. Any previously
 * enabled ranges will be nullified.
 * Intended to be used in early romstage to open up decoding windows to EC and
 * other eSPI devices needed during boot.
 *
 * Returns the total number of resources that have been enabled.
 */
int espi_enable_resources(const struct resource *resource_linked_list)
{
	struct espi_resource_allocator allocation;
	int num_total_ranges = 0;

	memset(&allocation, 0, sizeof(allocation));
	num_total_ranges = espi_allocate(&allocation, resource_linked_list);
	espi_write_resources(&allocation);
	return num_total_ranges;
}

/*
 * Same as above, but also opens IO windows for all probed subordinate devices.
 * Any previously enabled ranges will be nullified.
 * Intended to be used as the ops->enable_resources of the eSPI bridge device.
 */
void espi_enable_children_resources(struct device *espi)
{
	struct espi_resource_allocator allocation;
	const struct device *child;
	const struct bus *link;

	memset(&allocation, 0, sizeof(allocation));

	for (link = espi->link_list; link; link = link->next) {
		for (child = link->children; child; child = child->sibling) {
			if (!child->enabled)
				continue;

			espi_allocate(&allocation, child->resource_list);
		}
	}

	espi_write_resources(&allocation);
}
