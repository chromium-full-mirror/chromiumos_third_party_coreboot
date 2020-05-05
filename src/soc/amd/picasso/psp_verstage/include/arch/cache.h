/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#ifndef ARM_CACHE_H
#define ARM_CACHE_H

void static inline dcache_clean_all(void){}
void static inline cache_sync_instructions(void){}

#endif /* ARM_CACHE_H */
