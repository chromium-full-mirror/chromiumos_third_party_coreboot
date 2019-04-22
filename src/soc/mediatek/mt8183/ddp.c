/*
 * This file is part of the coreboot project.
 *
 * Copyright 2019 MediaTek Inc.
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
#include <edid.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <soc/addressmap.h>
#include <soc/ddp.h>

#define RDMA_FIFO_PSEUDO_SIZE(bytes)            (((bytes) / 16) << 16)
#define RDMA_OUTPUT_VALID_FIFO_THRESHOLD(bytes) ((bytes) / 16)

static void disp_config_main_path_connection(void)
{
	write32(&mmsys_cfg->disp_ovl0_mout_en, OVL0_MOUT_EN_OVL0_2L);
	write32(&mmsys_cfg->disp_ovl0_2l_mout_en, OVL0_2L_MOUT_EN_DISP_PATH0);
	write32(&mmsys_cfg->disp_path0_sel_in, DISP_PATH0_SEL_IN_OVL0_2L);
	write32(&mmsys_cfg->disp_rdma0_sout_sel_in, RDMA0_SOUT_SEL_IN_COLOR);
	write32(&mmsys_cfg->disp_dither0_mout_en, DITHER0_MOUT_EN_DISP_DSI0);
	write32(&mmsys_cfg->dsi0_sel_in, DSI0_SEL_IN_DITHER0_MOUT);
}

static void disp_config_main_path_mutex(void)
{
	write32(&disp_mutex->mutex[0].mod, MUTEX_MOD_MAIN_PATH);

	/* Clock source from DSI0 */
	write32(&disp_mutex->mutex[0].ctl,
		MUTEX_SOF_DSI0 | (MUTEX_SOF_DSI0 << 6));
	write32(&disp_mutex->mutex[0].en, BIT(0));
}

static void ovl_set_roi(u32 idx, u32 width, u32 height, u32 color)
{
	write32(&disp_ovl[idx]->roi_size, height << 16 | width);
	write32(&disp_ovl[idx]->roi_bgclr, color);
}

static void ovl_layer_enable(u32 idx)
{
	write32(&disp_ovl[idx]->rdma[0].ctrl, BIT(0));
	write32(&disp_ovl[idx]->rdma[0].mem_gmc_setting, RDMA_MEM_GMC);

	setbits_le32(&disp_ovl[idx]->src_con, BIT(0));
}

static void ovl_bgclr_in_sel(u32 idx)
{
	setbits_le32(&disp_ovl[idx]->datapath_con, BIT(2));
}

static void rdma_start(u32 idx)
{
	setbits_le32(&disp_rdma[idx]->global_con, RDMA_ENGINE_EN);
}

static void rdma_config(u32 idx, u32 width, u32 height, u32 vrefresh)
{
	u32 threshold;
	u32 reg;
	u32 fifo_size;

	clrsetbits_le32(&disp_rdma[idx]->size_con_0, 0x1FFF, width);
	clrsetbits_le32(&disp_rdma[idx]->size_con_1, 0xFFFFF, height);

	/*
	 * Enable FIFO underflow since DSI and DPI can't be blocked. Keep the
	 * FIFO pseudo size reset default of 8 KiB. Set the output threshold to
	 * 6 microseconds with 7/6 overhead to account for blanking, and with a
	 * pixel depth of 4 bytes:
	 */
	fifo_size = RDMA_FIFO_SIZE_0 * KiB;

	threshold = width * height * vrefresh * 4 * 7 / 1000000;

	if (threshold > fifo_size)
		threshold = fifo_size;

	reg = RDMA_FIFO_UNDERFLOW_EN |
	      RDMA_FIFO_PSEUDO_SIZE(fifo_size) |
	      RDMA_OUTPUT_VALID_FIFO_THRESHOLD(threshold);

	write32(&disp_rdma[idx]->fifo_con, reg);
}

static void color_start(u32 width, u32 height)
{
	write32(&disp_color->width, width);
	write32(&disp_color->height, height);
	write32(&disp_color->cfg_main, COLOR_BYPASS_ALL | COLOR_SEQ_SEL);
	write32(&disp_color->start, BIT(0));
}

static void aal_start(u32 width, u32 height)
{
	write32(&disp_aal->size, height << 16 | width);
	write32(&disp_aal->en, PQ_EN);
}

static void ccorr_start(u32 width, u32 height)
{
	write32(&disp_ccorr->size, height << 16 | width);
	write32(&disp_ccorr->cfg, PQ_RELAY_MODE);
	write32(&disp_ccorr->en, PQ_EN);
}

static void dither_start(u32 width, u32 height)
{
	write32(&disp_dither->size, height << 16 | width);
	write32(&disp_dither->cfg, PQ_RELAY_MODE);
	write32(&disp_dither->en, PQ_EN);
}

static void gamma_start(u32 width, u32 height)
{
	write32(&disp_gamma->size, height << 16 | width);
	write32(&disp_gamma->en, PQ_EN);
}

static void ovl_layer_config(u32 idx, u32 fmt, u32 bpp, u32 width, u32 height)
{
	write32(&disp_ovl[idx]->layer[0].con, fmt << 12);
	write32(&disp_ovl[idx]->layer[0].src_size, height << 16 | width);
	write32(&disp_ovl[idx]->layer[0].pitch, (width * bpp) & 0xFFFF);

	ovl_layer_enable(idx);
}

static void main_disp_path_setup(u32 width, u32 height, u32 vrefresh)
{
	u32 idx = 0;

	/* Setup OVL */
	for (idx = 0; idx < MAIN_PATH_OVL_NR; idx++) {
		u32 color = 0;

		if (idx == 0)
			color = 0xFF0000FF;

		ovl_set_roi(idx, width, height, color);
	}

	idx = 0;
	rdma_config(idx, width, height, vrefresh);

	color_start(width, height);

	ccorr_start(width, height);

	aal_start(width, height);

	gamma_start(width, height);

	dither_start(width, height);

	disp_config_main_path_connection();

	disp_config_main_path_mutex();
}

static void disp_clock_on(void)
{
	clrbits_le32(&mmsys_cfg->mmsys_cg_con0, CG_CON0_DISP_ALL);

	clrbits_le32(&mmsys_cfg->mmsys_cg_con1, CG_CON1_DISP_DSI0 |
						CG_CON1_DISP_DSI0_INTERFACE);
}

static void disp_m4u_port_off(void)
{
	write32((void *)(SMI_LARB0 + SMI_LARB_NON_SEC_CON), 0);
}

void mtk_ddp_init(void)
{
	disp_clock_on();
	disp_m4u_port_off();
}

void mtk_ddp_mode_set(const struct edid *edid)
{
	u32 fmt = OVL_INFMT_RGBA8888;
	u32 bpp = edid->framebuffer_bits_per_pixel / 8;
	u32 idx = 0;
	u32 width = edid->mode.ha;
	u32 height = edid->mode.va;
	u32 vrefresh = edid->mode.refresh;

	main_disp_path_setup(width, height, vrefresh);

	rdma_start(idx);

	ovl_layer_config(idx, fmt, bpp, width, height);

	ovl_bgclr_in_sel(idx+1);
}
