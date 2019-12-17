/*
 * This file is part of the coreboot project.
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

#ifndef AMDBLOCKS_LPC_CHECKER_H
#define AMDBLOCKS_LPC_CHECKER_H

/* returns 0 if we are ok. 1 if there is a problem/overlap on the ranges */
void check_lpc_espi_overlap(void);

#endif
