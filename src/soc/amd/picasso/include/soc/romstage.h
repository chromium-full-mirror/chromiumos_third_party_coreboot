/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2018 Google LLC
 * Copyright (C) 2019 Advanced Micro Devices, Inc.
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

#ifndef __PICASSO_HYBRID_ROMSTAGE_H__
#define __PICASSO_HYBRID_ROMSTAGE_H__

#include <stdint.h>
#include <arch/cpu.h>
#include <FspmUpd.h>

void mainboard_romstage_early_init(void);
void mainboard_romstage_entry_s3(int s3_resume);
void mainboard_fsp_memory_init_params_cb(FSP_M_CONFIG *mcfg, uint32_t version);

#endif /* __PICASSO_HYBRID_ROMSTAGE_H__ */
