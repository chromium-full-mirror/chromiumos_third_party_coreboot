/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <stdint.h>
#include <device/pci_ops.h>
#include <soc/pci_devs.h>
#include <amdblocks/lpc.h>
#include <soc/espi.h>

/*
 * Contrary to the ESPI_BASE_ADDRESS macro in iomap.h,
 * this is a not a fixed resource.
 */
void *espi_get_bar(void)
{
	uintptr_t spi_espi_bar, espi;

	spi_espi_bar = pci_read_config32(SOC_LPC_DEV, SPIROM_BASE_ADDRESS_REGISTER);
	espi = (spi_espi_bar & SPI_BAR_ADDRESS_MASK) + ESPI_OFFSET_FROM_BAR;
	return (void *)espi;

}
