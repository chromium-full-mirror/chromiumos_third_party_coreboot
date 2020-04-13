/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <stdint.h>
#include <soc/espi.h>

static void *espi_bar;

void *espi_get_bar(void)
{
	return espi_bar;
}

void espi_set_bar(void *bar)
{
	espi_bar = bar;
}
