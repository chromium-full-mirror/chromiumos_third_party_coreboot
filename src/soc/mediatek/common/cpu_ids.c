/* SPDX-License-Identifier: GPL-2.0-only */

#include <console/console.h>
#include <device/mmio.h>
#include <soc/cpu_ids.h>
#include <soc/efuse.h>

u32 get_cpuid(void)
{
	u32 id = read32(&mtk_efuse->cpu_id_reg);

	printk(BIOS_INFO, "CPU: %#x\n", id);

	return id;
}
