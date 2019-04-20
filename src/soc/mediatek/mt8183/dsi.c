/*
 * This file is part of the coreboot project.
 *
 * Copyright 2015 MediaTek Inc.
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

#include <arch/mmio.h>
#include <console/console.h>
#include <delay.h>
#include <soc/addressmap.h>
#include <soc/dsi.h>
#include <timer.h>

static void mipi_write32(void *a, uint32_t offset, uint32_t v)
{
	write32(a + offset, v);
}

static void mipi_clrsetbits_le32(void *a, uint32_t offset, uint32_t m, uint32_t v)
{
	clrsetbits_le32(a + offset, m, v);
}

static void mipi_clrbits_le32(void *a, uint32_t offset, uint32_t m)
{
	clrbits_le32(a + offset, m);
}

static void mipi_setbits_le32(void *a, uint32_t offset, uint32_t m)
{
	setbits_le32(a + offset, m);
}

static void dsi_write32(void *a, uint32_t v)
{
	write32(a, v);
}

static void dsi_clrsetbits_le32(void *a, uint32_t m, uint32_t v)
{
	clrsetbits_le32(a, m, v);
}

static void dsi_clrbits_le32(void *a, uint32_t m)
{
	clrbits_le32(a, m);
}

static void dsi_setbits_le32(void *a, uint32_t m)
{
	setbits_le32(a, m);
}

static int mtk_dsi_phy_clk_setting(u32 format, u32 lanes,
				   const struct edid *edid, struct mtk_phy_timing *phy_timing)
{
	unsigned int txdiv, txdiv0, txdiv1;
	u64 pcw;
	int data_rate;
	u32 htotal, htotal_bits, bit_per_pixel, overhead_cycles, overhead_bits;
	u64 total_bits;

	switch (format) {
	case MIPI_DSI_FMT_RGB565:
		bit_per_pixel = 16;
		break;
	case MIPI_DSI_FMT_RGB666:
	case MIPI_DSI_FMT_RGB666_PACKED:
		bit_per_pixel = 18;
		break;
	case MIPI_DSI_FMT_RGB888:
	default:
		bit_per_pixel = 24;
		break;
	}

	htotal = edid->mode.hbl + edid->mode.ha;
	htotal_bits = htotal * bit_per_pixel;

	overhead_cycles = 2 * phy_timing->lpx + phy_timing->da_hs_prepare +
			  phy_timing->da_hs_zero + phy_timing->da_hs_trail +
			  phy_timing->da_hs_exit + 1;

	overhead_bits = overhead_cycles * 8U;
	total_bits = htotal_bits + overhead_bits;

	data_rate = (u64)(edid->mode.pixel_clock * 1000 * total_bits) / (htotal * lanes);

	printk(BIOS_ERR, "data_rate: %u bps\n", data_rate);

	if (data_rate >= 2000000000) {
		txdiv = 1;
		txdiv0 = 0;
		txdiv1 = 0;
	} else if (data_rate >= 1000000000) {
		txdiv = 2;
		txdiv0 = 1;
		txdiv1 = 0;
	} else if (data_rate >= 500000000) {
		txdiv = 4;
		txdiv0 = 2;
		txdiv1 = 0;
	} else if (data_rate > 250000000) {
		txdiv = 8;
		txdiv0 = 3;
		txdiv1 = 0;
	} else if (data_rate >= 125000000) {
		txdiv = 16;
		txdiv0 = 4;
		txdiv1 = 0;
	} else {
		printk(BIOS_ERR, "data rate (%u) must be >=50. Please check "
		       "pixel clock (%u), bpp (%u), and number of lanes (%u)\n",
		       data_rate, edid->mode.pixel_clock, bit_per_pixel, lanes);
		return -1;
	}

	mipi_clrbits_le32(mipi_tx, MIPITX_PLL_CON4, BIT(11) | BIT(10));

	mipi_setbits_le32(mipi_tx, MIPITX_PLL_PWR, AD_DSI_PLL_SDM_PWR_ON);
	udelay(30);
	mipi_clrbits_le32(mipi_tx, MIPITX_PLL_PWR, AD_DSI_PLL_SDM_ISO_EN);

	pcw = (u64)((data_rate / 1000000) * (1 << txdiv0) * (1 << txdiv1)) << 24;
	pcw /= 26;

	mipi_write32(mipi_tx, MIPITX_PLL_CON0, pcw);
	mipi_clrsetbits_le32(mipi_tx, MIPITX_PLL_CON1, RG_DSI_PLL_POSDIV,
				txdiv0 << 8);
	udelay(30);
	mipi_setbits_le32(mipi_tx, MIPITX_PLL_CON1, RG_DSI_PLL_EN);

	/* BG_LPF_EN / BG_CORE_EN */
	mipi_write32(mipi_tx, MIPITX_LANE_CON, 0x3FFF0180);
	udelay(40);
	mipi_write32(mipi_tx,  MIPITX_LANE_CON, 0x3FFF00c0);

	/* Switch OFF each Lane */
	mipi_clrbits_le32(mipi_tx, MIPITX_D0_SW_CTL_EN, DSI_SW_CTL_EN);
	mipi_clrbits_le32(mipi_tx, MIPITX_D1_SW_CTL_EN, DSI_SW_CTL_EN);
	mipi_clrbits_le32(mipi_tx, MIPITX_D2_SW_CTL_EN, DSI_SW_CTL_EN);
	mipi_clrbits_le32(mipi_tx, MIPITX_D3_SW_CTL_EN, DSI_SW_CTL_EN);
	mipi_clrbits_le32(mipi_tx, MIPITX_CK_SW_CTL_EN, DSI_SW_CTL_EN);

	mipi_setbits_le32(mipi_tx, MIPITX_CK_CKMODE_EN, DSI_CK_CKMODE_EN);

	return data_rate;
}

static void mtk_dsi_phy_timconfig(u32 data_rate, struct mtk_phy_timing *phy_timing)
{
	u32 timcon0, timcon1, timcon2, timcon3;

	timcon0 = phy_timing->lpx | phy_timing->da_hs_prepare << 8 |
		  phy_timing->da_hs_zero << 16 | phy_timing->da_hs_trail << 24;
	timcon1 = phy_timing->ta_go | phy_timing->ta_sure << 8 |
		  phy_timing->ta_get << 16 | phy_timing->da_hs_exit << 24;
	timcon2 = 1 << 8 | phy_timing->clk_hs_zero << 16 |
		  phy_timing->clk_hs_trail << 24;
	timcon3 = phy_timing->clk_hs_prepare | phy_timing->clk_hs_post << 8 |
		  phy_timing->clk_hs_exit << 16;

	dsi_write32(&dsi->dsi_phy_timecon0, timcon0);
	dsi_write32(&dsi->dsi_phy_timecon1, timcon1);
	dsi_write32(&dsi->dsi_phy_timecon2, timcon2);
	dsi_write32(&dsi->dsi_phy_timecon3, timcon3);
}

static void mtk_dsi_reset(void)
{
	dsi_setbits_le32(&dsi->dsi_con_ctrl, 3);
	dsi_clrbits_le32(&dsi->dsi_con_ctrl, 1);
}

static void mtk_dsi_clk_hs_mode_enable(void)
{
	dsi_setbits_le32(&dsi->dsi_phy_lccon, LC_HS_TX_EN);
}

static void mtk_dsi_clk_hs_mode_disable(void)
{
	dsi_clrbits_le32(&dsi->dsi_phy_lccon, LC_HS_TX_EN);
}

static void mtk_dsi_set_mode(u32 mode_flags)
{
	u32 tmp_reg1 = 0;

	if (mode_flags & MIPI_DSI_MODE_VIDEO) {
		tmp_reg1 = SYNC_PULSE_MODE;

		if (mode_flags & MIPI_DSI_MODE_VIDEO_BURST)
			tmp_reg1 = BURST_MODE;

		if (mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
			tmp_reg1 = SYNC_PULSE_MODE;
	}

	dsi_write32(&dsi->dsi_mode_ctrl, tmp_reg1);
}

static void mtk_dsi_phy_timing_calc(u32 format, u32 lanes,
				   const struct edid *edid, struct mtk_phy_timing *phy_timing)
{
	u32 ui, cycle_time, data_rate;
	u32 bit_per_pixel;

	switch (format) {
	case MIPI_DSI_FMT_RGB565:
		bit_per_pixel = 16;
		break;
	case MIPI_DSI_FMT_RGB666:
	case MIPI_DSI_FMT_RGB666_PACKED:
		bit_per_pixel = 18;
		break;
	case MIPI_DSI_FMT_RGB888:
	default:
		bit_per_pixel = 24;
		break;
	}

	data_rate = edid->mode.pixel_clock * bit_per_pixel / lanes;

	ui = 1000 / (data_rate / 1000) + 1U;
	cycle_time = 8000 / (data_rate / 1000) + 1U;

	phy_timing->lpx = DIV_ROUND_UP(0x50, cycle_time);
	phy_timing->da_hs_prepare = DIV_ROUND_UP((0x40 + 0x5 * ui), cycle_time);
	phy_timing->da_hs_zero = DIV_ROUND_UP((0xc8 + 0x0a * ui), cycle_time);
	phy_timing->da_hs_trail = DIV_ROUND_UP(((0x4 * ui) + 0x50), cycle_time);

	if (phy_timing->da_hs_zero > phy_timing->da_hs_prepare)
		phy_timing->da_hs_zero -= phy_timing->da_hs_prepare;

	phy_timing->ta_go = 4U * phy_timing->lpx;
	phy_timing->ta_sure = 3U * phy_timing->lpx / 2U;
	phy_timing->ta_get = 5U * phy_timing->lpx;
	phy_timing->da_hs_exit = 2U * phy_timing->lpx;

	phy_timing->clk_hs_zero = DIV_ROUND_UP(0x150U, cycle_time);
	phy_timing->clk_hs_trail = DIV_ROUND_UP(0x64U, cycle_time) + 0xaU;

	phy_timing->clk_hs_prepare = DIV_ROUND_UP(0x40U, cycle_time);
	phy_timing->clk_hs_post = DIV_ROUND_UP(80U + 52U * ui, cycle_time);
	phy_timing->clk_hs_exit = 2U * phy_timing->lpx;
}

static void mtk_dsi_rxtx_control(u32 mode_flags, u32 lanes)
{
	u32 tmp_reg = 0;

	switch (lanes) {
	case 1:
		tmp_reg = 1 << 2;
		break;
	case 2:
		tmp_reg = 3 << 2;
		break;
	case 3:
		tmp_reg = 7 << 2;
		break;
	case 4:
	default:
		tmp_reg = 0xf << 2;
		break;
	}

	tmp_reg |= (mode_flags & MIPI_DSI_CLOCK_NON_CONTINUOUS) << 6;
	tmp_reg |= (mode_flags & MIPI_DSI_MODE_EOT_PACKET) >> 3;

	dsi_write32(&dsi->dsi_txrx_ctrl, tmp_reg);
}

static void mtk_dsi_config_vdo_timing(u32 mode_flags, u32 format,
				      const struct edid *edid)
{
	u32 hsync_active_byte;
	u32 hbp_byte;
	u32 hfp_byte;
	u32 vbp_byte;
	u32 vfp_byte;
	u32 bpp;
	u32 packet_fmt;
	u32 hactive;

	if (format == MIPI_DSI_FMT_RGB565)
		bpp = 2;
	else
		bpp = 3;

	vbp_byte = edid->mode.vbl - edid->mode.vso - edid->mode.vspw -
		   edid->mode.vborder;
	vfp_byte = edid->mode.vso - edid->mode.vborder;


	dsi_write32(&dsi->dsi_vsa_nl, edid->mode.vspw);
	dsi_write32(&dsi->dsi_vbp_nl, vbp_byte);
	dsi_write32(&dsi->dsi_vfp_nl, vfp_byte);
	dsi_write32(&dsi->dsi_vact_nl, edid->mode.va);

	if (mode_flags & MIPI_DSI_MODE_VIDEO_SYNC_PULSE)
		hbp_byte = (edid->mode.hbl - edid->mode.hso - edid->mode.hspw -
			    edid->mode.hborder) * bpp - 10;
	else
		hbp_byte = (edid->mode.hbl - edid->mode.hso -
			    edid->mode.hborder) * bpp - 10;

	hsync_active_byte = edid->mode.hspw * bpp - 10;
	hfp_byte = (edid->mode.hso - edid->mode.hborder) * bpp - 12;

	dsi_write32(&dsi->dsi_hsa_wc, hsync_active_byte);
	dsi_write32(&dsi->dsi_hbp_wc, hbp_byte);
	dsi_write32(&dsi->dsi_hfp_wc, hfp_byte);

	switch (format) {
	case MIPI_DSI_FMT_RGB888:
		packet_fmt = PACKED_PS_24BIT_RGB888;
		break;
	case MIPI_DSI_FMT_RGB666:
		packet_fmt = LOOSELY_PS_18BIT_RGB666;
		break;
	case MIPI_DSI_FMT_RGB666_PACKED:
		packet_fmt = PACKED_PS_18BIT_RGB666;
		break;
	case MIPI_DSI_FMT_RGB565:
		packet_fmt = PACKED_PS_16BIT_RGB565;
		break;
	default:
		packet_fmt = PACKED_PS_24BIT_RGB888;
		break;
	}

	hactive = edid->mode.ha;
	packet_fmt |= (hactive * bpp) & DSI_PS_WC;

	dsi_write32(&dsi->dsi_psctrl, 0x2c << 24 | packet_fmt);
	dsi_write32(&dsi->dsi_size_con, edid->mode.va << 16 | hactive);
}

static void mtk_dsi_start(void)
{
	dsi_write32(&dsi->dsi_start, 0);
	dsi_write32(&dsi->dsi_start, 1);
}

static void mtk_dsi_cmdq(u8 *data, u8 len)
{
	struct stopwatch sw;
	u8 *tx_buf = data;
	u8 cmdq_size;
	u32 reg_val, cmdq_mask, i, config, cmdq_off, type, intsta_0;

	switch (len) {
	case 0:
		return;

	case 1:
		type = MIPI_DSI_DCS_SHORT_WRITE;
		break;

	case 2:
		type = MIPI_DSI_DCS_SHORT_WRITE_PARAM;
		break;

	default:
		type = MIPI_DSI_DCS_LONG_WRITE;
		break;
	}

	if (MTK_DSI_HOST_IS_READ(type))
		config = BTA;
	else
		config = (len > 2) ? LONG_PACKET : SHORT_PACKET;

	if (len > 2) {
		cmdq_size = 1 + (len + 3) / 4;
		cmdq_off = 4;
		cmdq_mask = CONFIG | DATA_ID | DATA_0 | DATA_1;
		reg_val = (len << 16) | (type << 8) | config;
	} else {
		cmdq_size = 1;
		cmdq_off = 2;
		cmdq_mask = CONFIG | DATA_ID;
		reg_val = (type << 8) | config;
	}

	for (i = 0; i < len; i++)
		dsi_clrsetbits_le32(&dsi->dsi_cmdq0 + ((cmdq_off + i) & (~0x3)),
			     (0xff << (((i + cmdq_off) & 3) * 8)),
			     tx_buf[i] << (((i + cmdq_off) & 3) * 8));

	dsi_clrsetbits_le32(&dsi->dsi_cmdq0, cmdq_mask, reg_val);
	dsi_clrsetbits_le32(&dsi->dsi_cmdq_size, CMDQ_SIZE, cmdq_size);
	dsi_write32(&dsi->dsi_intsta, 0);
	mtk_dsi_start();

	stopwatch_init_usecs_expire(&sw, 400);
	do {
		intsta_0 = read32(&dsi->dsi_intsta);
		if (intsta_0 & CMD_DONE_INT_FLAG)
			break;
		udelay(4);
	} while (!stopwatch_expired(&sw));

	if (!(intsta_0 & CMD_DONE_INT_FLAG))
		printk(BIOS_ERR, "dsi DONE INT Timeout\n");

	dsi_write32(&dsi->dsi_start, 0);
}

static void push_table(struct lcm_init_table *init_cmd, u32 count)
{
	u32 cmd, i;

	for (i = 0; i  < count; i++) {
		cmd = init_cmd[i].cmd;

		switch (cmd) {
		case DELAY_CMD:
			mdelay(init_cmd[i].cmd);
			break;

		case END_OF_TABLE:
			break;

		case INIT_CMD:
		default:
			mtk_dsi_cmdq(init_cmd[i].data, init_cmd[i].len);
			break;
		}
	}
}

int mtk_dsi_init(u32 mode_flags, u32 format, u32 lanes, bool dual,
		 const struct edid *edid, struct lcm_init_table *init_cmd, u32 count)
{
	int data_rate;
	struct mtk_phy_timing phy_timing;

	mtk_dsi_phy_timing_calc(format, lanes, edid, &phy_timing);

	data_rate = mtk_dsi_phy_clk_setting(format, lanes, edid, &phy_timing);

	if (data_rate < 0)
		return -1;

	mtk_dsi_reset();
	dsi_write32(&dsi->dsi_force_commit, 3);
	mtk_dsi_phy_timconfig(data_rate, &phy_timing);
	mtk_dsi_rxtx_control(mode_flags, lanes);
	mtk_dsi_clk_hs_mode_disable();
	mtk_dsi_config_vdo_timing(mode_flags, format, edid);
	mtk_dsi_clk_hs_mode_enable();
	mtk_dsi_set_mode(0);
	push_table(init_cmd, count);
	mtk_dsi_set_mode(mode_flags);
	mtk_dsi_start();

	return 0;
}

