/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <timer.h>
#include <console/console.h>
#include <device/pci_ops.h>
#include <soc/pci_devs.h>
#include <soc/smu.h>

static uint32_t smu_read32(uint32_t reg)
{
	pci_write_config32(SOC_GNB_DEV, SMU_INDEX_ADDR, reg);
	return pci_read_config32(SOC_GNB_DEV, SMU_DATA_ADDR);
}

static void smu_write32(uint32_t reg, uint32_t val)
{
	pci_write_config32(SOC_GNB_DEV, SMU_INDEX_ADDR, reg);
	pci_write_config32(SOC_GNB_DEV, SMU_DATA_ADDR, val);
}

static int smu_poll_response(void)
{
	struct stopwatch sw;
	const long timeout_ms = 10 * MSECS_PER_SEC;

	stopwatch_init_msecs_expire(&sw, timeout_ms);

	do {
		if (!smu_read32(REG_ADDR_MESG_RESP)) {
			printk(BIOS_SPEW, "SMU command consumed %ld msecs\n",
					stopwatch_duration_usecs(&sw));
			return 0;
		}
	} while (!stopwatch_expired(&sw));

	printk(BIOS_ERR, "Error: timeout sending SMU message\n");
	return -1;
}

/*
 * Send a message and bi-directional payload to the SMU.  SMU response, if
 * any, is returned via arg.  Returns 0 if success or -1 on failure.
 */
int send_smu_message(enum smu_message_id id, struct smu_payload *arg)
{
	int i;

	smu_write32(REG_ADDR_MESG_RESP, 0);

	for (i = 0 ; i < NUM_ARGS ; i++)
		smu_write32(REG_ADDR_MESG_ARG(i), arg->msg[i]);

	smu_write32(REG_ADDR_MESG_ID, id);
	if (smu_poll_response())
		return -1;

	for (i = 0 ; i < NUM_ARGS ; i++)
		arg->msg[i] = smu_read32(REG_ADDR_MESG_ARG(i));

	return 0;
}

/*
 * Request the SMU put system into S3, S4, or S5.  On entry, SlpTyp determines
 * S-State and SlpTypeEn is clear.  Function does not return if successful.
 */
void smu_sx_entry(void)
{
	struct smu_payload msg = { 0 }; /* Unused for SMC_MSG_S3ENTRY */

	printk(BIOS_DEBUG, "SMU: Put system into S3/S4/S5\n");
	send_smu_message(SMC_MSG_S3ENTRY, &msg);
}
