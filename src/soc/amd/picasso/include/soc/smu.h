/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#ifndef __PICASSO_SMU_H__
#define __PICASSO_SMU_H__

/* SMU registers accessed indirectly using an index/data pair in D0F00 */
#define SMU_INDEX_ADDR		0xb8
#define SMU_DATA_ADDR		0xbc

#define REG_ADDR_MESG_ID	0x3b10528
#define REG_ADDR_MESG_RESP	0x3b10564
#define REG_ADDR_MESG_ARGS_BASE	0x0b10998

/* Argument 0-5 indexed locations are contiguous */
#define NUM_ARGS		6
#define REG_ADDR_MESG_ARG(x)	(REG_ADDR_MESG_ARGS_BASE + ((x) * sizeof(uint32_t)))

enum smu_message_id {
	SMC_MSG_S3ENTRY = 0xc,
};

struct smu_payload {
	uint32_t msg[NUM_ARGS];
};

/*
 * Send a message and bi-directional payload to the SMU.  SMU response, if
 * any, is returned via arg.  Returns 0 if success or -1 on failure.
 */
int send_smu_message(enum smu_message_id id, struct smu_payload *arg);

/*
 * Request the SMU put system into S3, S4, or S5.  On entry, SlpTyp determines
 * S-State and SlpTypeEn is clear.  Function does not return if successful.
 */
void smu_sx_entry(void);

#endif /* __PICASSO_SMU_H__ */
