/* SPDX-License-Identifier: GPL-2.0-only */

#ifndef __MIPI_PANEL_H__
#define __MIPI_PANEL_H__

#include <commonlib/mipi/cmd.h>
#include <edid.h>
#include <types.h>

/* Definitions for flags in panel_serializable_data */
enum panel_flag {
	PANEL_FLAG_CPHY = BIT(0),
};

/*
 * The data to be serialized and put into CBFS.
 * Note some fields, for example edid.mode.name, were actually pointers and
 * cannot be really serialized.
 */
struct panel_serializable_data {
	u32 flags; /* flags of panel_flag */
	struct edid edid;  /* edid info of this panel */
	u8 init[]; /* A packed array of panel_init_command */
};

#endif /* __MIPI_PANEL_H__ */
