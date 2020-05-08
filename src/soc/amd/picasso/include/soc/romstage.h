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

void romstage_mainboard_early_init(void);
void romstage_mainboard_init(void);

#endif /* __PICASSO_HYBRID_ROMSTAGE_H__ */
