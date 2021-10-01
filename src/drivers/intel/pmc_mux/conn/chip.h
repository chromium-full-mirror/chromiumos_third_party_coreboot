/* SPDX-License-Identifier: GPL-2.0-or-later */

#ifndef __DRIVERS_INTEL_PMC_MUX_CONN_H__
#define __DRIVERS_INTEL_PMC_MUX_CONN_H__

#include <boot/coreboot_tables.h>

struct drivers_intel_pmc_mux_conn_config {
	/* 1-based port numbers (from SoC point of view) */
	int usb2_port_number;
	/* 1-based port numbers (from SoC point of view) */
	int usb3_port_number;
	/* Orientation of the sideband signals (SBU) */
	enum type_c_orientation sbu_orientation;
	/* Orientation of the High Speed lines */
	enum type_c_orientation hsl_orientation;
};

#endif /* __DRIVERS_INTEL_PMC_MUX_CONN_H__ */
