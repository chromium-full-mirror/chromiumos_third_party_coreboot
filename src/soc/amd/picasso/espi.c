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

#include <console/console.h>
#include <device/pci_ops.h>
#include <soc/espi.h>
#include <soc/pci_devs.h>
#include <stdint.h>
#include <string.h>
#include <timer.h>
#include <amdblocks/lpc.h>
#include <amdblocks/lpc_espi_checker.h>
#include <lib.h>
#include <assert.h>

#if CONFIG(DEBUG_ESPI_INIT)
# define ESPI_DBG BIOS_DEBUG
# define ESPI_EXT_DBG BIOS_SPEW
#else
# define ESPI_DBG BIOS_NEVER
# define ESPI_EXT_DBG BIOS_NEVER
#endif

static void write32_espi(uint8_t *espi, uint8_t offset, uint32_t val)
{
	printk(ESPI_EXT_DBG, "Writing 0x%08x to espi offset 0x%02x\n",
			val, offset);
	write32(espi + offset, val);
}

static uint32_t read32_espi(uint8_t *espi, uint8_t offset)
{
	uint32_t val = read32(espi + offset);
	printk(ESPI_EXT_DBG, "Read 0x%08x from espi offset 0x%02x\n",
			val, offset);
	return val;
}

void dump_espi_regs(void)
{
	/* Show the entire address space of 64 dwords*/
	hexdump32(ESPI_EXT_DBG, espi_read_base_address(), 0x40);
}

static void espi_show_slave_configuration(uint32_t config)
{
	if ((ESPI_DBG < (int)BIOS_EMERG) || (ESPI_DBG >= (int)BIOS_NEVER))
		return;

	printk(ESPI_DBG, "eSPI Slave configuration:\n");

	if (config & ESPI_SLAVE_CRC_CHECKING_EN)
		printk(ESPI_DBG, "    CRC checking enabled\n");

	if (config & ESPI_SLAVE_RESP_MOD_EN)
		printk(ESPI_DBG, "    Response modifier enabled\n");

	if (config & ESPI_SLAVE_ALERT_MODE)
		printk(ESPI_DBG, "    Dedicated Alert# used to signal alert event\n");
	else
		printk(ESPI_DBG, "    IO bit1 pin used to signal alert event\n");

	if ((config & (0x03 << 26)) == ESPI_SLAVE_IO_MODE_SINGLE)
		printk(ESPI_DBG, "    eSPI single IO mode selected\n");
	else if ((config & (0x03 << 26)) == ESPI_SLAVE_IO_MODE_DUAL)
		printk(ESPI_DBG, "    eSPI dual IO mode selected\n");
	else if ((config & (0x03 << 26)) == ESPI_SLAVE_IO_MODE_QUAD)
		printk(ESPI_DBG, "    eSPI quad IO mode selected\n");
	else
		printk(ESPI_DBG, "    Error: Invalid eSPI IO mode selected\n");

	if ((config & (0x03 << 24)) == ESPI_SLAVE_QUAD_IO_SUPPORTED)
		printk(ESPI_DBG, "    eSPI quad and single IO modes supported\n");
	else if ((config & (0x03 << 24)) == ESPI_SLAVE_DUAL_IO_SUPPORTED)
		printk(ESPI_DBG, "    eSPI dual and single IO mode supported\n");
	else if ((config & (0x03 << 24)) == (ESPI_SLAVE_DUAL_IO_SUPPORTED |
			ESPI_SLAVE_QUAD_IO_SUPPORTED))
		printk(ESPI_DBG, "    eSPI quad, dual, and single IO modes supported\n");
	else
		printk(ESPI_DBG, "    Only eSPI single IO mode supported\n");

	if (config & ESPI_SLAVE_OPEN_DRAIN_ALERT)
		printk(ESPI_DBG, "    Alert# pin is open-drain\n");
	else
		printk(ESPI_DBG, "    Alert# pin is driven\n");

	if ((config & (0x07 << 20)) == ESPI_SLAVE_OP_FREQ_20_MHZ)
		printk(ESPI_DBG, "    eSPI 20MHz selected\n");
	else if ((config & (0x07 << 20)) == ESPI_SLAVE_OP_FREQ_25_MHZ)
		printk(ESPI_DBG, "    eSPI 25MHz selected\n");
	else if ((config & (0x07 << 20)) == ESPI_SLAVE_OP_FREQ_33_MHZ)
		printk(ESPI_DBG, "    eSPI 33MHz selected\n");
	else if ((config & (0x07 << 20)) == ESPI_SLAVE_OP_FREQ_50_MHZ)
		printk(ESPI_DBG, "    eSPI 50MHz selected\n");
	else if ((config & (0x07 << 20)) == ESPI_SLAVE_OP_FREQ_66_MHZ)
		printk(ESPI_DBG, "    eSPI 66MHz selected\n");
	else
		printk(ESPI_DBG, "    Error: Invalid eSPI frequency\n");

	if (config & ESPI_SLAVE_SUPP_OPEN_DRAIN_ALERT)
		printk(ESPI_DBG, "    Open-drain Alert# pin supported\n");

	if ((config & (0x07 << 16)) == ESPI_SLAVE_SUPP_FREQ_20_MHZ)
		printk(ESPI_DBG, "    eSPI 20MHz supported\n");
	else if ((config & (0x07 << 16)) == ESPI_SLAVE_SUPP_FREQ_25_MHZ)
		printk(ESPI_DBG, "    eSPI 25MHz supported\n");
	else if ((config & (0x07 << 16)) == ESPI_SLAVE_SUPP_FREQ_33_MHZ)
		printk(ESPI_DBG, "    eSPI 33MHz supported\n");
	else if ((config & (0x07 << 16)) == ESPI_SLAVE_SUPP_FREQ_50_MHZ)
		printk(ESPI_DBG, "    eSPI 50MHz supported\n");
	else if ((config & (0x07 << 16)) == ESPI_SLAVE_SUPP_FREQ_66_MHZ)
		printk(ESPI_DBG, "    eSPI 66MHz supported\n");
	else
		printk(ESPI_DBG, "    Error: Invalid eSPI frequency\n");

	printk(ESPI_DBG, "    Maximum Wait state: %d\n", (config >> 12) & 0x0f);

	if (config & ESPI_SLAVE_SUPP_PERIPHERAL_CH)
		printk(ESPI_DBG, "    Peripheral Channel supported\n");
	if (config & ESPI_SLAVE_SUPP_VIRTUAL_WIRE_CH)
		printk(ESPI_DBG, "    Virtual Wire Channel supported\n");
	if (config & ESPI_SLAVE_SUPP_OOB_CH)
		printk(ESPI_DBG, "    OOB Channel supported\n");
	if (config & ESPI_SLAVE_SUPP_FLASH_CH)
		printk(ESPI_DBG, "    Flash Access Channel supported\n");
	printk(ESPI_DBG, "\n");
}

static void espi_show_host_configuration(void)
{
	uint8_t *espi = espi_read_base_address();
	uint32_t slave0_decode_en = read32(espi + ESPI_DECODE);
	uint32_t slave0_config = read32(espi + ESPI_SLAVE0_CONFIG);

	if ((ESPI_DBG < (int)BIOS_EMERG) || (ESPI_DBG >= (int)BIOS_NEVER))
		return;

	printk(ESPI_DBG, "eSPI Host configuration:\n");
	if (slave0_decode_en & ESPI_DECODE_IO_0X2E_0X2F_EN)
		printk(ESPI_DBG, "  eSPI decode of 0x2e - 0x2f enabled\n");
	if (slave0_decode_en & ESPI_DECODE_IO_0X60_0X64_EN)
		printk(ESPI_DBG, "  eSPI decode of 0x60 - 0x64 enabled\n");
	if (slave0_decode_en & ESPI_DECODE_IO_0x80_EN)
		printk(ESPI_DBG, "  eSPI decode of 0x80 enabled\n");

	for (int i = 0; i < 4; i++) {
		if (slave0_decode_en & ESPI_DECODE_IO_RANGE_EN(i)) {
			printk(ESPI_DBG, "  IO_RANGE%hd: 0x%04hx- 0x%04hhx\n", i,
					read16(espi + ESPI_IO_RANGE_BASE(i)),
					read16(espi + ESPI_IO_RANGE_BASE(i)) +
					read8(espi + ESPI_IO_RANGE_SIZE(i)));
		}
	}

	for (int i = 0; i < 4; i++) {
		if (slave0_decode_en & ESPI_DECODE_MMIO_RANGE_EN(i)) {
			printk(ESPI_DBG, "  MMIO_RANGE%d: 0x%08x- 0x%08x\n", i,
					read32(espi + ESPI_MMIO_RANGE_BASE(i)),
					(read32(espi + ESPI_MMIO_RANGE_BASE(i)) +
					read16(espi + ESPI_MMIO_RANGE_SIZE(i))));
		}
	}

	if (slave0_config & ESPI_CRC_CHECKING_EN)
		printk(ESPI_DBG, "  CRC Check Enabled\n");

	if (slave0_config & ESPI_ALERT_MODE)
		printk(ESPI_DBG, "  Dedicated Alert# used to signal alert event\n");
	else
		printk(ESPI_DBG, "  IO bit1 pin used to signal alert event\n");

	if ((slave0_config & (0x03 << 28)) == ESPI_IO_MODE_SINGLE)
		printk(ESPI_DBG, "  eSPI enabled in single IO mode\n");
	else if ((slave0_config & (0x03 << 28)) == ESPI_IO_MODE_DUAL)
		printk(ESPI_DBG, "  eSPI enabled in dual IO mode\n");
	else if ((slave0_config & (0x03 << 28)) == ESPI_IO_MODE_QUAD)
		printk(ESPI_DBG, "  eSPI enabled in quad IO mode\n");
	else
		printk(ESPI_DBG, "  Error: Invalid eSPI IO mode\n");

	if ((slave0_config & (0x07 << 25)) == ESPI_OP_FREQ_16_MHZ)
		printk(ESPI_DBG, "  eSPI enabled at 16MHz\n");
	else if ((slave0_config & (0x07 << 25)) == ESPI_OP_FREQ_33_MHZ)
		printk(ESPI_DBG, "  eSPI enabled at 33MHz\n");
	else if ((slave0_config & (0x07 << 25)) == ESPI_OP_FREQ_66_MHZ)
		printk(ESPI_DBG, "  eSPI enabled at 66MHz\n");
	else
		printk(ESPI_DBG, "  Error: Invalid eSPI frequency\n");

	if (slave0_config & ESPI_PR_EN)
		printk(ESPI_DBG, "  Peripheral Channel Enabled\n");

	if (slave0_config & ESPI_VIRTUAL_WIRE_CH_EN)
		printk(ESPI_DBG, "  Virtual Wire Channel Enabled\n");

	if (slave0_config & ESPI_OOB_CH_EN)
		printk(ESPI_DBG, "  OOB Message Channel Enabled\n");

	if (slave0_config & ESPI_FLASH_CH_EN)
		printk(ESPI_DBG, "  Flash Access Channel Enabled\n");
	printk(ESPI_DBG, "\n");
}

const char *status_strings[32] = {
	"Bus Error",
	"Wait Timeout",
	"CRC_Error",
	"Reserved",
	"No Response",
	"Fatal Error",
	"Non-Fatal Error",
	"Unknown Response",
	"Unknown Cycle Type",
	"Unsuccessful Completion",
	"Illegal Tag",
	"Illegal Length",
	"RX OOB Overflow",
	"RX Message Overflow",
	"Rx Flash Overflow",
	"Protocol Error",
	"Reserved",
	"Reserved",
	"MST Abort",
	"Watchdog Timeout",
	"Reserved",
	"Reserved",
	"Reserved",
	"Reserved",
	"RX Virtual Wire Group 0",
	"RX Virtual Wire Group 1",
	"RX Virtual Wire Group 2",
	"RX Virtual Wire Group 3",
	"Downstream command complete",
	"Peripheral message received",
	"OOB message received",
	"Flash request received"
};

static void show_espi_status(uint32_t status)
{
	if ((ESPI_DBG < (int)BIOS_EMERG) || (ESPI_DBG >= (int)BIOS_NEVER))
		return;

	if (!status) {
		printk(ESPI_DBG, "No eSPI status bits set.\n");
		return;
	}

	for (int i = 0; i < 32; i++) {
		if (status & (1 << i)) {
			printk(ESPI_DBG, "  Status bit: %s\n", status_strings[i]);
		}
	}
}

static uint32_t espi_wait_response(uint8_t *espi)
{
	struct stopwatch sw;
	uint32_t status;

	stopwatch_init_usecs_expire(&sw, 100);
	do {
		status = read32(espi + SLAVE0_INT_STS);

		if (status)
			return status;

	} while (!stopwatch_expired(&sw));
	ASSERT_MSG(0, "Error: eSPI timed out waiting for a response.\n");

	return 0;
}

static bool espi_wait_ready(uint8_t *espi)
{
	struct stopwatch sw;

	stopwatch_init_usecs_expire(&sw, 100);
	do {
		if (!(read32(espi + ESPI_DN_TXDR0) & ESPI_TX_GO_STATUS))
			return true;
	} while (!stopwatch_expired(&sw));

	return false;
}

static void espi_clear(uint8_t *espi, int ccr)
{
	/* Clear status register */
	uint32_t status = read32_espi(espi, SLAVE0_INT_STS);
	if (status)
		write32_espi(espi, SLAVE0_INT_STS, status);

	/* Clear the command registers if requested */
	if (!ccr)
		return;

	for (int i = 0; i < 0xf; i += 4) {
		write32_espi(espi, ESPI_DN_TXDR0 + i, 0x00);
	}
}

/* TODO: look at turning the command into a struct */
static uint32_t espi_send_command(uint8_t *espi, uint32_t cmd0, uint32_t cmd1,
		 uint32_t cmd2, uint32_t cmd3)
{
	uint32_t status;
	if (!espi_wait_ready(espi)) {
		printk(BIOS_WARNING, "Error: eSPI was not ready to accept a command.\n");
		return -1;
	}

	espi_clear(espi, 0);

	write32_espi(espi, ESPI_DN_TXDR1, cmd1);
	write32_espi(espi, ESPI_DN_TXDR2, cmd2);
	write32_espi(espi, ESPI_DN_TXDR3, cmd3);
	/* Dword 0 must be last as this write triggers the transaction */
	write32_espi(espi, ESPI_DN_TXDR0, cmd0);

	if (!espi_wait_ready(espi)) {
		printk(ESPI_DBG, "eSPI cmd0-cmd3: %08x %08x %08x %08x.\n",
		       cmd0, cmd1, cmd2, cmd3);
		ASSERT_MSG(0, "eSPI timed out waiting for command to complete\n");
		return -1;
	}
	status = espi_wait_response(espi);
	show_espi_status(status);

	return status;
}

static uint32_t espi_send_reset(uint8_t *espi)
{
	uint32_t tx_ctl;

	tx_ctl = ESPI_TX_CMD_RESET;
	tx_ctl |= ESPI_TX_GO_STATUS;

	return espi_send_command(espi, tx_ctl, 0, 0, 0);
}

static uint32_t espi_send_vw_cmd(uint8_t *espi, const struct vw_config_def *vw_config,
		 uint8_t val)
{
	uint32_t tx_ctl, vw_cmd, status;

	tx_ctl = (0 << 8);	/* 1 group in the packet */
	tx_ctl |= (0x0 << 4);	/* Slave select -- always 0 */
	tx_ctl |= ESPI_VW_CMD;
	tx_ctl |= ESPI_TX_GO_STATUS;

	vw_cmd = vw_config->index;
	if (vw_config->valid_bit < 8)
		vw_cmd |= ((1 << vw_config->valid_bit) << 8);
	if (vw_config->enable_bit < 8)
		vw_cmd |= ((val << vw_config->enable_bit) << 8);
	else
		vw_cmd |= (val << 8);

	status = espi_send_command(espi, tx_ctl, 0, 0, vw_cmd);
	dump_espi_regs();
	return status;
}

static uint32_t espi_get_configuration(uint8_t *espi, uint16_t addr)
{
	uint32_t tx_ctl, config = -1, status;

	printk(ESPI_EXT_DBG, "Enter %s - addr: 0x%04hx\n", __func__, addr);

	tx_ctl = addr & 0x0f00;		/* HDATA0, Address [15:8].  [15:12]:0h */
	tx_ctl |= (0 << 24);		/* Reserved, always 0 */
	tx_ctl |= (addr & 0xfc) << 16;	/* HDATA1, Address [7:0]. [1:0]:0h */
	tx_ctl |= (0x0 << 4);	/* Slave select, always 0 */
	tx_ctl |= ESPI_TX_CMD_GET_CONFIGURATION;
	tx_ctl |= ESPI_TX_GO_STATUS;

	status = espi_send_command(espi, tx_ctl, 0, 0, 0);

	/* Read the response */
	if ((status != -1) && (status & ESPI_STATUS_DNCMD) &&
			 !(status & ESPI_STATUS_NO_RESPONSE)) {
		config = read32_espi(espi, ESPI_DN_TXDR1);
		if (addr == ESPI_SLAVE_GENERAL_CFG)
			espi_show_slave_configuration(config);
		else
			printk(ESPI_DBG,
					"Get Configuration of reg 0x%04hx returned 0x%08x\n",
					addr, config);
	}

	dump_espi_regs();

	return config;
}

static uint32_t espi_set_configuration(uint8_t *espi, uint16_t addr, uint32_t val)
{
	uint32_t tx_ctl, status;
	printk(ESPI_EXT_DBG, "Enter %s - addr: 0x%04hx, val 0x%08x\n", __func__,
			addr, val);

	tx_ctl = addr & 0x0f00;			/* HDATA0, Address [15:8].  [15:12]:0h */
	tx_ctl |= (addr & 0xfc) << 16;		/* HDATA1, Address [7:0]. [1:0]:0h */
	tx_ctl |= (0x0 << 4);			/* Slave select -- always 0 */
	tx_ctl |= ESPI_TX_CMD_SET_CONFIGURATION;
	tx_ctl |= ESPI_TX_GO_STATUS;

	status = espi_send_command(espi, tx_ctl, val, 0, 0);
	dump_espi_regs();
	return status;
}

static uint32_t espi_wait_channel_ready(uint8_t *espi, uint8_t channel)
{
	struct stopwatch sw;
	uint32_t config;

	stopwatch_init_usecs_expire(&sw, 1000);
	do {
		config = espi_get_configuration(espi, channel);
		if (config & ESPI_SLAVE_CHANNEL_READY)
			return config;
	} while (!stopwatch_expired(&sw));

	return -1;
}

static uint32_t enable_channel(uint8_t *espi, const char *name, uint8_t addr, uint32_t val)
{
	uint32_t config;

	printk(ESPI_DBG, "Enabling %s channel\n", name);
	config = espi_get_configuration(espi, addr);
	if (config == -1) {
		printk(BIOS_WARNING, "eSPI Error: could not read %s channel configuration\n",
				 name);
		return -1;
	}

	printk(ESPI_DBG, "%s channel configuration: 0x%08x\n", name, config);
	espi_set_configuration(espi, addr, config | ESPI_SLAVE_CHANNEL_ENABLE | val);
	config = espi_wait_channel_ready(espi, addr);
	return config;
}

static void espi_setup_slave(uint8_t *espi, const struct espi_config *cfg)
{
	uint32_t slave_cfg_reg = 0;

	slave_cfg_reg |= cfg->enable_crc_checking ? ESPI_CRC_CHECKING_EN : 0;
	slave_cfg_reg |= cfg->alert_pin_on_io1	? 0 : ESPI_ALERT_MODE;

	if (cfg->espi_freq_mhz == ESPI_OP_FREQ_66_MHZ) {
		slave_cfg_reg |= ESPI_SLAVE_OP_FREQ_66_MHZ;
		printk(ESPI_DBG, "Set slave bus to 66MHz\n");
	} else if (cfg->espi_freq_mhz == ESPI_OP_FREQ_33_MHZ) {
		slave_cfg_reg |= ESPI_SLAVE_OP_FREQ_33_MHZ;
		printk(ESPI_DBG, "Set slave bus to 33MHz\n");
	} else {
		slave_cfg_reg |= ESPI_SLAVE_OP_FREQ_20_MHZ;
		printk(ESPI_DBG, "Set slave bus to 20MHz\n");
	}

	if (cfg->bus_width == ESPI_IO_MODE_SINGLE) {
		slave_cfg_reg |= ESPI_SLAVE_IO_MODE_SINGLE;
		printk(ESPI_DBG, "Set slave single channel mode\n");
	} else if (cfg->bus_width == ESPI_IO_MODE_DUAL) {
		slave_cfg_reg |= ESPI_SLAVE_IO_MODE_DUAL;
		printk(ESPI_DBG, "Set slave dual channel mode\n");
	} else {
		slave_cfg_reg |= ESPI_SLAVE_IO_MODE_QUAD;
		printk(ESPI_DBG, "Set slave quad channel mode\n");
	}

	/* Enable the channels, then set the configuration to update the speed & width */

	/* TODO: Configure the peripheral channel's modes and sizes */
	if (cfg->peripheral_ch_en)
		enable_channel(espi, "Peripheral", ESPI_SLAVE_CHAN0_CFG, BIT(2));

	/* TODO: Configure the number of wires correctly */
	if (cfg->virtual_wire_ch_en)
		enable_channel(espi, "Virtual-wire", ESPI_SLAVE_CHAN1_CFG, 0x3f << 16);

	if (cfg->out_of_band_ch_en)
		enable_channel(espi, "Out-of-band", ESPI_SLAVE_CHAN2_CFG, 0);

	if (cfg->flash_ch_en)
		enable_channel(espi, "Flash", ESPI_SLAVE_CHAN3_CFG, 0);

	espi_set_configuration(espi, ESPI_SLAVE_GENERAL_CFG, slave_cfg_reg);
}

static void send_pltrst(uint8_t *espi)
{
	uint32_t config;
	const struct vw_config_def pltrst = {VW_PLTRST_CONFIG};

	config = espi_get_configuration(espi, ESPI_SLAVE_CHAN1_CFG);
	if (config == -1)
		config = 0;
	else
		config = (config & 0x3f00) << 8;

	config = enable_channel(espi, "Virtual-wire", ESPI_SLAVE_CHAN1_CFG, config);

	/* If the channel isn't ready, print an error but try to send PLTRST anyway */
	if (config == -1)
		printk(BIOS_ERR, "Error: Virtual-wire channel not ready when sending PLTRST\n");

	printk(ESPI_DBG, "Sending PLTRST\n");
	espi_send_vw_cmd(espi, &pltrst, 1);
}

static bool is_0x2e_0x2f(uintptr_t base)
{
	return ((base & ~0x01) == 0x2e);
}

static bool is_0x60_0x64(uintptr_t base)
{
	return ((base & ~0x04) == 0x60);
}

static bool is_0x80(uintptr_t base)
{
	return (base == 0x80);
}

/*
 * Contrary to the ESPI_BASE_ADDRESS macro in iomap.h,
 * this is a not a fixed resource.
 */
void *espi_read_base_address(void)
{
	uintptr_t spi_espi_bar, espi;

	spi_espi_bar = pci_read_config32(SOC_LPC_DEV, SPIROM_BASE_ADDRESS_REGISTER);
	espi = (spi_espi_bar & SPI_BAR_ADDRESS_MASK) + ESPI_OFFSET_FROM_BAR;
	return (void *)espi;
}

void espi_setup(const struct espi_config *cfg)
{
	uint32_t cfg_reg = 0;
	uint8_t *espi = espi_read_base_address();

	/* Set correct initial configuration to talk to the slave */
	write32_espi(espi, ESPI_SLAVE0_CONFIG, cfg->espi_initial_mode);

	/* The resets affects both host and slave devices, so set initial config again */
	espi_send_reset(espi);
	write32_espi(espi, ESPI_SLAVE0_CONFIG, cfg->espi_initial_mode);

	/* Enable virtual wire channel and send PLTRST before setting anything else up */
	send_pltrst(espi);
	write32_espi(espi, ESPI_SLAVE0_CONFIG, cfg->espi_initial_mode);

	cfg_reg |= cfg->enable_crc_checking	? ESPI_CRC_CHECKING_EN : 0;
	cfg_reg |= cfg->alert_pin_on_io1	? 0 : ESPI_ALERT_MODE;

	/*
	 * TODO: Make sure remote slave supports the requested configuration and
	 *       adjust the configuration if it doesn't
	 */
	cfg_reg |= cfg->bus_width;
	cfg_reg |= cfg->espi_freq_mhz;

	cfg_reg |= cfg->peripheral_ch_en	? ESPI_PR_EN : 0;
	cfg_reg |= cfg->virtual_wire_ch_en	? ESPI_VIRTUAL_WIRE_CH_EN : 0;
	cfg_reg |= cfg->out_of_band_ch_en	? ESPI_OOB_CH_EN : 0;
	cfg_reg |= cfg->flash_ch_en		? ESPI_FLASH_CH_EN : 0;
	write32_espi(espi, ESPI_SLAVE0_CONFIG, cfg_reg);

	if (cfg->update_slave)
		espi_setup_slave(espi, cfg);

	espi_get_configuration(espi, ESPI_SLAVE_GENERAL_CFG);

	espi_show_host_configuration();
	printk(ESPI_DBG, "Configure host\n");
	write32_espi(espi, ESPI_SLAVE0_CONFIG, cfg_reg);
	espi_show_host_configuration();

	espi_get_configuration(espi, ESPI_SLAVE_GENERAL_CFG);
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

	if (is_0x80(resource->base) && resource->size == 1) {
		allocation->enable_0x80 = true;
		return 1;
	}

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
		if (allocation->io_ranges[i].size > 0x100) {
			printk(BIOS_EMERG, "Error: IO range has a max size of 0x100\n");
			printk(BIOS_EMERG, "IO_RANGE_%d has a size of 0x%04x\n", i,
					(unsigned int)allocation->io_ranges[i].size);
			ASSERT_MSG(0, "IO size too big for ESPI");
		}

		decode_enable |= ESPI_DECODE_IO_RANGE_EN(i);
		write16(espi + ESPI_IO_RANGE_BASE(i),
			allocation->io_ranges[i].base);
		write8(espi + ESPI_IO_RANGE_SIZE(i),
			allocation->io_ranges[i].size - 1);
		printk(ESPI_DBG, "Enabling eSPI decode of 0x%04x - 0x%04x\n",
				(unsigned int)allocation->io_ranges[i].base,
				(unsigned int)(allocation->io_ranges[i].base +
				allocation->io_ranges[i].size - 1));
	}

	for (i = 0; i < allocation->num_mmio_ranges; i++) {
		decode_enable |= ESPI_DECODE_MMIO_RANGE_EN(i);
		write32(espi + ESPI_MMIO_RANGE_BASE(i),
			allocation->mmio_ranges[i].base);
		write16(espi + ESPI_MMIO_RANGE_SIZE(i),
			allocation->mmio_ranges[i].size - 1);
		printk(ESPI_DBG, "ESPI MMIO range: %08llx, Size: %04llx",
		       allocation->mmio_ranges[i].base,
		       allocation->mmio_ranges[i].size-1);
	}

	if (allocation->enable_0x2e_0x2f) {
		printk(ESPI_DBG, "Enabling eSPI decode of 0x2e - 0x2f\n");
		decode_enable |= ESPI_DECODE_IO_0X2E_0X2F_EN;
	}

	if (allocation->enable_0x60_0x64) {
		printk(ESPI_DBG, "Enabling eSPI decode of 0x60 - 0x64\n");
		decode_enable |= ESPI_DECODE_IO_0X60_0X64_EN;
	}

	if (allocation->enable_0x80) {
		printk(ESPI_DBG, "Enabling eSPI decode of 0x80\n");
		decode_enable |= ESPI_DECODE_IO_0x80_EN;
	}

	printk(ESPI_DBG, "ESPI Decode reg: %08x", decode_enable);
	write32(espi + ESPI_DECODE, decode_enable);

	check_lpc_espi_overlap();
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
 * Enable resources specified by resource_list.
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
 * Any previously enabled ranges will be nullified if any values are set here.
 * Intended to be used as the ops->enable_resources of the eSPI bridge device.
 */
void espi_enable_children_resources(struct device *espi)
{
	struct espi_resource_allocator allocation;
	const struct device *child;
	const struct bus *link;
	int set = 0;

	memset(&allocation, 0, sizeof(allocation));

	for (link = espi->link_list; link; link = link->next) {
		for (child = link->children; child; child = child->sibling) {
			if (!child->enabled)
				continue;
			set = 1;
			espi_allocate(&allocation, child->resource_list);
		}
	}

	if (set)
		espi_write_resources(&allocation);
}
