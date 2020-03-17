/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2012-2018 Advanced Micro Devices, Inc.
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

#include <device/mmio.h>
#include <cpu/x86/msr.h>
#include <cpu/amd/msr.h>
#include <cbfs.h>
#include <region_file.h>
#include <timer.h>
#include <device/pci_def.h>
#include <bootstate.h>
#include <rules.h>
#include <console/console.h>
#include <device/pci_ops.h>
#include <amdblocks/psp.h>
#include "psp_def.h"
#include <soc/iomap.h>
#include <soc/northbridge.h>

struct _c2p_buffer {
	u8 buffer[C2P_BUFFER_MAXSIZE];
} __attribute__((aligned(32)));

struct _p2c_buffer {
	u8 buffer[P2C_BUFFER_MAXSIZE];
} __attribute__((aligned(32)));

static struct _p2c_buffer p2c_buffer;
static struct _c2p_buffer c2p_buffer;

static uint32_t smm_flag; /* Non-zero for SMM, clear when not */

static void set_smm_flag(void)
{
	if (!(ENV_SMM))
		return;
	smm_flag = 1;
}

static void clear_smm_flag(void)
{
	if (!(ENV_SMM))
		return;
	smm_flag = 0;
}

static const char *psp_status_nobase = "error: PSP BAR3 not assigned";
static const char *psp_status_halted = "error: PSP in halted state";
static const char *psp_status_recovery = "error: PSP recovery required";
static const char *psp_status_errcmd = "error sending command";
static const char *psp_status_init_timeout = "error: PSP init timeout";
static const char *psp_status_cmd_timeout = "error: PSP command timeout";
static const char *psp_status_noerror = "";

static const char *status_to_string(int err)
{
	switch (err) {
	case -PSPSTS_NOBASE:
		return psp_status_nobase;
	case -PSPSTS_HALTED:
		return psp_status_halted;
	case -PSPSTS_RECOVERY:
		return psp_status_recovery;
	case -PSPSTS_SEND_ERROR:
		return psp_status_errcmd;
	case -PSPSTS_INIT_TIMEOUT:
		return psp_status_init_timeout;
	case -PSPSTS_CMD_TIMEOUT:
		return psp_status_cmd_timeout;
	default:
		return psp_status_noerror;
	}
}

static u32 v1_rd_mbox_sts(struct pspv1_mbox *mbox)
{
	return read32(&mbox->mbox_status);
}

static void v1_wr_mbox_cmd(struct pspv1_mbox *mbox, u32 cmd)
{
	write32(&mbox->mbox_command, cmd);
}

static u32 v1_rd_mbox_cmd(struct pspv1_mbox *mbox)
{
	return read32(&mbox->mbox_command);
}

static void v1_wr_mbox_cmd_resp(struct pspv1_mbox *mbox, void *buffer)
{
	write64(&mbox->cmd_response, (uintptr_t)buffer);
}

static int v1_wait_initialized(struct pspv1_mbox *mbox)
{
	struct stopwatch sw;

	stopwatch_init_msecs_expire(&sw, PSP_INIT_TIMEOUT);

	do {
		if (v1_rd_mbox_sts(mbox) & PSPV1_STATUS_INITIALIZED)
			return 0;
	} while (!stopwatch_expired(&sw));

	return -PSPSTS_INIT_TIMEOUT;
}

static int v1_wait_command(struct pspv1_mbox *mbox)
{
	struct stopwatch sw;

	stopwatch_init_msecs_expire(&sw, PSP_CMD_TIMEOUT);

	do {
		if (!v1_rd_mbox_cmd(mbox))
			return 0;
	} while (!stopwatch_expired(&sw));

	return -PSPSTS_CMD_TIMEOUT;
}

static int do_command_v1(u32 command, void *buffer)
{
	struct pspv1_mbox *mbox = soc_get_mbox_address();
	if (!mbox)
		return -PSPSTS_NOBASE;

	/* check for PSP error conditions */
	if (v1_rd_mbox_sts(mbox) & PSPV1_STATUS_HALT)
		return -PSPSTS_HALTED;

	if (v1_rd_mbox_sts(mbox) & PSPV1_STATUS_RECOVERY)
		return -PSPSTS_RECOVERY;

	/* PSP must be finished with init and ready to accept a command */
	if (v1_wait_initialized(mbox))
		return -PSPSTS_INIT_TIMEOUT;

	if (v1_wait_command(mbox))
		return -PSPSTS_CMD_TIMEOUT;

	/* set smm flag, address of command-response buffer and write command */
	set_smm_flag();
	v1_wr_mbox_cmd_resp(mbox, buffer);
	v1_wr_mbox_cmd(mbox, command);

	/* PSP clears command register when complete */
	if (v1_wait_command(mbox)) {
		clear_smm_flag();
		return -PSPSTS_CMD_TIMEOUT;
	}

	clear_smm_flag();

	/* check delivery status */
	if (v1_rd_mbox_sts(mbox) & (PSPV1_STATUS_ERROR | PSPV1_STATUS_TERMINATED))
		return -PSPSTS_SEND_ERROR;

	return 0;
}

static u16 v2_rd_mbox_sts(struct pspv2_mbox *mbox)
{
	union {
		u32 val;
		struct pspv2_mbox_cmd_fields fields;
	} tmp = { 0 };

	tmp.val = read32(&mbox->val);
	return tmp.fields.mbox_status;
}

static void v2_wr_mbox_cmd(struct pspv2_mbox *mbox, u8 cmd)
{
	union {
		u32 val;
		struct pspv2_mbox_cmd_fields fields;
	} tmp = { 0 };

	/* Write entire 32-bit area to begin command execution */
	tmp.fields.mbox_command = cmd;
	write32(&mbox->val, tmp.val);
}

static u8 v2_rd_mbox_recovery(struct pspv2_mbox *mbox)
{
	union {
		u32 val;
		struct pspv2_mbox_cmd_fields fields;
	} tmp = { 0 };

	tmp.val = read32(&mbox->val);
	return !!tmp.fields.recovery;
}

static void v2_wr_mbox_cmd_resp(struct pspv2_mbox *mbox, void *buffer)
{
	write64(&mbox->cmd_response, (uintptr_t)buffer);
}

static int v2_wait_command(struct pspv2_mbox *mbox, bool wait_for_ready)
{
	struct pspv2_mbox and_mask = { .val = ~0 };
	struct pspv2_mbox expected = { .val = 0 };
	struct stopwatch sw;
	u32 tmp;

	/* Zero fields from and_mask that should be kept */
	and_mask.fields.mbox_command = 0;
	and_mask.fields.ready = wait_for_ready ? 0 : 1;

	/* Expect mbox_cmd == 0 but ready depends */
	if (wait_for_ready)
		expected.fields.ready = 1;

	stopwatch_init_msecs_expire(&sw, PSP_CMD_TIMEOUT);

	do {
		tmp = read32(&mbox->val);
		tmp &= ~and_mask.val;
		if (tmp == expected.val)
			return 0;
	} while (!stopwatch_expired(&sw));

	return -PSPSTS_CMD_TIMEOUT;
}

static int do_command_v2(u32 command, void *buffer)
{
	struct pspv2_mbox *mbox = soc_get_mbox_address();
	if (!mbox)
		return -PSPSTS_NOBASE;

	if (v2_rd_mbox_recovery(mbox))
		return -PSPSTS_RECOVERY;

	if (v2_wait_command(mbox, true))
		return -PSPSTS_CMD_TIMEOUT;

	/* set smm flag, address of command-response buffer and write command */
	set_smm_flag();
	v2_wr_mbox_cmd_resp(mbox, buffer);
	v2_wr_mbox_cmd(mbox, command);

	/* PSP clears command register when complete.  All commands except
	 * SxInfo set the Ready bit. */
	if (v2_wait_command(mbox,
			command == MBOX_BIOS_CMD_SX_INFO ? false : true)) {
		clear_smm_flag();
		return -PSPSTS_CMD_TIMEOUT;
	}

	clear_smm_flag();

	/* check delivery status */
	if (v2_rd_mbox_sts(mbox))
		return -PSPSTS_SEND_ERROR;

	return 0;
}

static int send_psp_command(u32 command, void *buffer)
{
	if (CONFIG(SOC_AMD_COMMON_BLOCK_PSP_GEN1))
		return do_command_v1(command, buffer);

	return do_command_v2(command, buffer);
}

static u32 rd_resp_sts(struct mbox_default_buffer *buffer)
{
	return read32(&buffer->header.status);
}

/*
 * Print meaningful status to the console.  Caller only passes a pointer to a
 * buffer if it's expected to contain its own status.
 */
static void print_cmd_status(int cmd_status, struct mbox_default_buffer *buffer)
{
	if (buffer && rd_resp_sts(buffer))
		printk(BIOS_DEBUG, "buffer status=0x%x ", rd_resp_sts(buffer));

	if (cmd_status)
		printk(BIOS_DEBUG, "%s\n", status_to_string(cmd_status));
	else
		printk(BIOS_DEBUG, "OK\n");
}

/*
 * Notify the PSP that DRAM is present.  Upon receiving this command, the PSP
 * will load its OS into fenced DRAM that is not accessible to the x86 cores.
 */
int psp_notify_dram(void)
{
	int cmd_status;
	struct mbox_default_buffer buffer = {
		.header = {
			.size = sizeof(buffer)
		}
	};

	printk(BIOS_DEBUG, "PSP: Notify that DRAM is available... ");

	cmd_status = send_psp_command(MBOX_BIOS_CMD_DRAM_INFO, &buffer);

	/* buffer's status shouldn't change but report it if it does */
	print_cmd_status(cmd_status, &buffer);

	return cmd_status;
}

/*
 * Notify the PSP that the system is completing the boot process.  Upon
 * receiving this command, the PSP will only honor commands where the buffer
 * is in SMM space.
 */
static void psp_notify_boot_done(void *unused)
{
	int cmd_status;
	struct mbox_default_buffer buffer = {
		.header = {
			.size = sizeof(buffer)
		}
	};

	printk(BIOS_DEBUG, "PSP: Notify that POST is finishing... ");

	cmd_status = send_psp_command(MBOX_BIOS_CMD_BOOT_DONE, &buffer);

	/* buffer's status shouldn't change but report it if it does */
	print_cmd_status(cmd_status, &buffer);
}

int psp_notify_smm(void)
{
	msr_t msr;
	int cmd_status;
	struct mbox_cmd_smm_info_buffer buffer = {
		.header = {
			.size = sizeof(buffer)
		}
	};

	if (!(ENV_SMM)) {
		printk (BIOS_ERR, "PSP: Error, cannot send SMM info from outside SMM\n");
		return PSPSTS_UNSUPPORTED;
	}

	msr = rdmsr(SMM_ADDR_MSR);
	buffer.req.smm_base = ((uint64_t)msr.hi << 32) | msr.lo;
	msr = rdmsr(SMM_MASK_MSR);
	msr.lo &= 0xfffff000;
	buffer.req.smm_mask = ((uint64_t)msr.hi << 32) | msr.lo;

	soc_fill_smm_trig_info(&buffer.req.smm_trig_info);
#if (CONFIG(SOC_AMD_COMMON_BLOCK_PSP_GEN2))
	soc_fill_smm_reg_info(&buffer.req.smm_reg_info);
#endif

	buffer.req.psp_smm_data_region = (uintptr_t)p2c_buffer.buffer;
	buffer.req.psp_smm_data_length = sizeof(p2c_buffer);

	buffer.req.psp_mbox_smm_buffer_address = (uintptr_t)c2p_buffer.buffer;
	buffer.req.psp_mbox_smm_flag_address = (uintptr_t)&smm_flag;

	printk(BIOS_DEBUG, "PSP: Notify SMM info... ");

	cmd_status = send_psp_command(MBOX_BIOS_CMD_SMM_INFO, &buffer);

	/* buffer's status shouldn't change but report it if it does */
	print_cmd_status(cmd_status, (struct mbox_default_buffer *)&buffer);

	return cmd_status;
}

/* Notify PSP the system is going to a sleep state. */
void psp_notify_sx_info(u8 sleep_type)
{
	int cmd_status;
	struct mbox_cmd_sx_info_buffer *buffer;

	buffer = (struct mbox_cmd_sx_info_buffer *)c2p_buffer.buffer;
	memset(buffer, 0, sizeof(*buffer));
	buffer->header.size = sizeof(*buffer);

	if (sleep_type > 7) {
		printk(BIOS_ERR, "PSP: Bug: sleep type 0x%x requested\n", sleep_type);
		sleep_type &= 7;
	}

	printk(BIOS_DEBUG, "PSP: Prepare to enter sleep state %d\n ", sleep_type);

	buffer->sleep_type = sleep_type;
	cmd_status = send_psp_command(MBOX_BIOS_CMD_SX_INFO, buffer);

	/* buffer's status shouldn't change but report it if it does */
	print_cmd_status(cmd_status, (struct mbox_default_buffer *)buffer);
}

/*
 * Tell the PSP to load a firmware blob from a location in the BIOS image.
 */
int psp_load_named_blob(enum psp_blob_type type, const char *name)
{
	int cmd_status;
	u32 command;
	void *blob;
	struct cbfsf cbfs_file;
	struct region_device rdev;

	switch (type) {
	case BLOB_SMU_FW:
		command = MBOX_BIOS_CMD_SMU_FW;
		break;
	case BLOB_SMU_FW2:
		command = MBOX_BIOS_CMD_SMU_FW2;
		break;
	default:
		printk(BIOS_ERR, "BUG: Invalid PSP blob type %x\n", type);
		return -PSPSTS_INVALID_BLOB;
	}

	if (!CONFIG(SOC_AMD_PSP_SELECTABLE_SMU_FW) &&
			(type == BLOB_SMU_FW || type == BLOB_SMU_FW2)) {
		printk(BIOS_ERR, "BUG: Selectable firmware is not supported\n");
		return -PSPSTS_UNSUPPORTED;
	}

	if (cbfs_boot_locate(&cbfs_file, name, NULL)) {
		printk(BIOS_ERR, "BUG: Cannot locate blob for PSP loading\n");
		return -PSPSTS_INVALID_NAME;
	}

	cbfs_file_data(&rdev, &cbfs_file);
	blob = rdev_mmap_full(&rdev);
	if (!blob) {
		printk(BIOS_ERR, "BUG: Cannot map blob for PSP loading\n");
		return -PSPSTS_INVALID_NAME;
	}

	printk(BIOS_DEBUG, "PSP: Load blob type %x from @%p... ", type, blob);

	/* Blob commands use the buffer registers as data, not pointer to buf */
	cmd_status = send_psp_command(command, blob);
	print_cmd_status(cmd_status, NULL);

	rdev_munmap(&rdev, blob);
	return cmd_status;
}

BOOT_STATE_INIT_ENTRY(BS_PAYLOAD_BOOT, BS_ON_ENTRY,
		psp_notify_boot_done, NULL);
