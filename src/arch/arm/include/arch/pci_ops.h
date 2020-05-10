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

#ifndef ARCH_ARM_PCI_OPS_H
#define ARCH_ARM_PCI_OPS_H

#include <stdint.h>
#include <device/pci_type.h>

#ifdef __SIMPLE_DEVICE__
u8 pci_read_config8(pci_devfn_t dev, unsigned int where);
u16 pci_read_config16(pci_devfn_t dev, unsigned int where);
u32 pci_read_config32(pci_devfn_t dev, unsigned int where);
void pci_write_config8(pci_devfn_t dev, unsigned int where, u8 val);
void pci_write_config16(pci_devfn_t dev, unsigned int where, u16 val);
void pci_write_config32(pci_devfn_t dev, unsigned int where, u32 val);

static inline uint8_t pci_s_read_config8(pci_devfn_t dev, uint16_t reg)   { return 0; }
static inline uint16_t pci_s_read_config16(pci_devfn_t dev, uint16_t reg) { return 0; }
static inline uint32_t pci_s_read_config32(pci_devfn_t dev, uint16_t reg) { return 0; }
static inline void pci_s_write_config8(pci_devfn_t dev, uint16_t reg, uint8_t value)   {}
static inline void pci_s_write_config16(pci_devfn_t dev, uint16_t reg, uint16_t value) {}
static inline void pci_s_write_config32(pci_devfn_t dev, uint16_t reg, uint32_t value) {}
#endif

#endif
