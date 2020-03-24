/*
 * This file is part of the coreboot project.
 *
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

#ifndef __PI_PICASSO_PCI_DEVS_H__
#define __PI_PICASSO_PCI_DEVS_H__

#include <device/pci_def.h>

#if !defined(__SIMPLE_DEVICE__)
#include <device/device.h>
#define _SOC_DEV(slot, func)	pcidev_on_root(slot, func)
#else
#define _SOC_DEV(slot, func)	PCI_DEV(0, slot, func)
#endif

/* See AMD 55570-B1 Table 13: PCI Device ID Assignments.*/

/* GNB Root Complex */
#define GNB_DEV			0x0
#define GNB_FUNC		0
#define GNB_DEVID		0x1576
#define GNB_DEVFN		PCI_DEVFN(GNB_DEV, GNB_FUNC)
#define SOC_GNB_DEV		_SOC_DEV(GNB_DEV, GNB_FUNC)

/* IOMMU */
#define IOMMU_DEV		0x0
#define IOMMU_FUNC		2
#define IOMMU_DEVID		0x1577
#define IOMMU_DEVFN		PCI_DEVFN(IOMMU_DEV, IOMMU_FUNC)
#define SOC_IOMMU_DEV		_SOC_DEV(IOMMU_DEV, IOMMU_FUNC)

/* PCIe GPP Bridges 0 - 6 */
#define PCIE_DEV		0x1
#define PCIE_BRIDGE_DEVID	0x15d3

#define PCIE_GPP_0_FUNC		1
#define PCIE_GPP_0_DEVFN	PCI_DEVFN(PCIE_DEV, PCIE_GPP_0_FUNC)
#define SOC_GPP_0_DEV		_SOC_DEV(PCIE_DEV, PCIE_GPP_0_FUNC)

#define PCIE_GPP_1_FUNC		2
#define PCIE_GPP_1_DEVFN	PCI_DEVFN(PCIE_DEV, PCIE_GPP_1_FUNC)
#define SOC_GPP_1_DEV		_SOC_DEV(PCIE_DEV, PCIE_GPP_1_FUNC)

#define PCIE_GPP_2_FUNC		3
#define PCIE_GPP_2_DEVFN	PCI_DEVFN(PCIE_DEV, PCIE_GPP_2_FUNC)
#define SOC_GPP_2_DEV		_SOC_DEV(PCIE_DEV, PCIE_GPP_2_FUNC)

#define PCIE_GPP_3_FUNC		4
#define PCIE_GPP_3_DEVFN	PCI_DEVFN(PCIE_DEV, PCIE_GPP_3_FUNC)
#define SOC_GPP_3_DEV		_SOC_DEV(PCIE_DEV, PCIE_GPP_3_FUNC)

#define PCIE_GPP_4_FUNC		5
#define PCIE_GPP_4_DEVFN	PCI_DEVFN(PCIE_DEV, PCIE_GPP_4_FUNC)
#define SOC_GPP_4_DEV		_SOC_DEV(PCIE_DEV, PCIE_GPP_4_FUNC)

#define PCIE_GPP_5_FUNC		6
#define PCIE_GPP_5_DEVFN	PCI_DEVFN(PCIE_DEV, PCIE_GPP_5_FUNC)
#define SOC_GPP_5_DEV		_SOC_DEV(PCIE_DEV, PCIE_GPP_5_FUNC)

#define PCIE_GPP_6_FUNC		7
#define PCIE_GPP_6_DEVFN	PCI_DEVFN(PCIE_DEV, PCIE_GPP_6_FUNC)
#define SOC_GPP_6_DEV		_SOC_DEV(PCIE_DEV, PCIE_GPP_6_FUNC)

/* Bridges 7 - 8 are to Bus A and Bus B devices*/
#define PCIE_AB_BRIDGE_DEV	0x8

#define PCIE_A_BRIDGE_DEVID	0x15db
#define PCIE_GPP_A_FUNC		1
#define PCIE_A_DEVFN		PCI_DEVFN(PCIE_AB_BRIDGE_DEV, PCIE_GPP_A_FUNC)
#define SOC_PCIEA_DEV		_SOC_DEV(PCIE_AB_BRIDGE_DEV, PCIE_GPP_A_FUNC)

#define PCIE_B_BRIDGE_DEVID	0x15dc
#define PCIE_GPP_B_FUNC		2
#define PCIE_B_DEVFN		PCI_DEVFN(PCIE_AB_BRIDGE_DEV, PCIE_GPP_B_FUNC)
#define SOC_PCIEB_DEV		_SOC_DEV(PCIE_AB_BRIDGE_DEV, PCIE_GPP_B_FUNC)


/* Data Fabric functions */
#define DF_DEV			0x18

#define DF_F0_DEVID		0x15e8
#define DF_F0_DEVFN		PCI_DEVFN(DF_DEV, 0)
#define SOC_DF_F0_DEVFN		_SOC_DEV(DF_DEV, 0)

#define DF_F1_DEVID		0x15e9
#define DF_F1_DEVFN		PCI_DEVFN(DF_DEV, 1)
#define SOC_DF_F1_DEVFN		_SOC_DEV(DF_DEV, 1)

#define DF_F2_DEVID		0x15ea
#define DF_F2_DEVFN		PCI_DEVFN(DF_DEV, 2)
#define SOC_DF_F2_DEVFN		_SOC_DEV(DF_DEV, 2)

#define DF_F3_DEVID		0x15eb
#define DF_F3_DEVFN		PCI_DEVFN(DF_DEV, 3)
#define SOC_DF_F3_DEVFN		_SOC_DEV(DF_DEV, 3)

#define DF_F4_DEVID		0x15ec
#define DF_F4_DEVFN		PCI_DEVFN(DF_DEV, 4)
#define SOC_DF_F4_DEVFN		_SOC_DEV(DF_DEV, 4)

#define DF_F5_DEVID		0x15ed
#define DF_F5_DEVFN		PCI_DEVFN(DF_DEV, 5)
#define SOC_DF_F5_DEVFN		_SOC_DEV(DF_DEV, 5)

#define DF_F6_DEVID		0x15ee
#define DF_F6_DEVFN		PCI_DEVFN(DF_DEV, 6)
#define SOC_DF_F6_DEVFN		_SOC_DEV(DF_DEV, 6)

/* USB 3.1 */
#define XHCI0_DEV		0x0
#define XHCI0_FUNC		3
#define XHCI0_PICASSO_DEVID	0x15e0 // Not actually used anywhere
#define XHCI0_DALI_DEVID	0x15e5
#define XHCI0_DEVFN		PCI_DEVFN(XHCI0_DEV, XHCI0_FUNC)
#define SOC_XHCI0_DEV		_SOC_DEV(XHCI0_DEV, XHCI0_FUNC)

/* USB 3.1 */
#define XHCI1_DEV		0x0
#define XHCI1_FUNC		4
#define XHCI1_PICASSO_DEVID	0x15e1
#define XHCI1_DEVFN		PCI_DEVFN(XHCI1_DEV, XHCI1_FUNC)
#define SOC_XHCI1_DEV		_SOC_DEV(XHCI1_DEV, XHCI1_FUNC)

/* SATA */
#define SATA_DEV		0x11
#define SATA_FUNC		0
#define SATA_IDE_DEVID		0x7900
#define AHCI_DEVID_MS		0x7901
#define AHCI_DEVID_AMD		0x7904
#define SATA_DEVFN		PCI_DEVFN(SATA_DEV, SATA_FUNC)
#define SOC_SATA_DEV		_SOC_DEV(SATA_DEV, SATA_FUNC)

/* SMBUS */
#define SMBUS_DEV		0x14
#define SMBUS_FUNC		0
#define SMBUS_DEVID		0x790b
#define SMBUS_DEVFN		PCI_DEVFN(SMBUS_DEV, SMBUS_FUNC)
#define SOC_SMBUS_DEV		_SOC_DEV(SMBUS_DEV, SMBUS_FUNC)

/* LPC BUS */
#define PCU_DEV			0x14
#define LPC_FUNC		3
#define LPC_DEVID		0x790e
#define LPC_DEVFN		PCI_DEVFN(PCU_DEV, LPC_FUNC)
#define SOC_LPC_DEV		_SOC_DEV(PCU_DEV, LPC_FUNC)

/* SDHCI controller */
#define SDHCI_DEVFN		PCI_DEVFN(0x14, 6)

/* Internal Graphics */
#define GFX_DEV			0x0
#define GFX_FUNC		0
#define GFX_DEVID		0x15d8
#define GFX_DEVFN		PCI_DEVFN(GFX_DEV, GFX_FUNC)
#define SOC_GFX_DEV		_SOC_DEV(GFX_DEV, GFX_FUNC)

/* Audio 0 */
#define HDA0_DEV		0x0
#define HDA0_FUNC		5
#define HDA0_DEVID		0x15e2
#define HDA0_DEVFN		PCI_DEVFN(HDA0_DEV, HDA0_FUNC)
#define SOC_HDA0_DEV		_SOC_DEV(HDA0_DEV, HDA0_FUNC)

/* HD Audio 1 */
#define HDA1_DEV		0x0
#define HDA1_FUNC		6
#define HDA1_DEVID		0x15e3
#define HDA1_DEVFN		PCI_DEVFN(HDA1_DEV, HDA1_FUNC)
#define SOC_HDA1_DEV		_SOC_DEV(HDA1_DEV, HDA1_FUNC)

#endif /* __PI_PICASSO_PCI_DEVS_H__ */
