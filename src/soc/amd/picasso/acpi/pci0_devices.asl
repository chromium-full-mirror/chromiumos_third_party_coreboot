/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

// 0:00.0   # Root Complex
Device (GNB){
    Name(_ADR, 0x00000000)
}
// 0:00.2   # IOMMU
Device (IOMM){
    Name(_ADR, 0x00000002)
}
// 0:01.1   # GPP Bridge 0
Device (PBR0){
    Name(_ADR, 0x00010001)
}
// 0:01.2   # GPP Bridge 1
Device (PBR1){
    Name(_ADR, 0x00010002)
}
// 0:01.3   # GPP Bridge 2
Device (PBR2){
    Name(_ADR, 0x00010003)
}
// 0:01.4   # GPP Bridge 3
Device (PBR3){
    Name(_ADR, 0x00010004)
}
// 0:01.5   # GPP Bridge 4
Device (PBR4){
    Name(_ADR, 0x00010005)
}
// 0:01.6   # GPP Bridge 5
Device (PBR5){
    Name(_ADR, 0x00010006)
}
// 0:01.7   # GPP Bridge 6
Device (PBR6){
    Name(_ADR, 0x00010007)
}
// 0:08.1   # Internal GPP Bridge 0 to Bus A
Device (PBRA){
    Name(_ADR, 0x00080001)
    // 0.1      # Display HDA
    // 0.2      # Crypto Coprocesor

    // 0.3      # XHCI 0 under Bus A
    Device(XHC0) {
        Name(_ADR, 0x00000003)
        Name(_PRW, Package() { 0xb, 3 })
        Device (RHUB) {
            Name (_ADR, 0x0)
            /* Devices dynamically generated at boot */
        }

        Method(_S0W,0) {
            Return(0)
        }

        Method(_S3W,0) {
            Return(4)
        }

        Method(_S4W,0) {
            Return(4)
        }

    } /* end XHC0 */

    // 0.4      # XHCI 1 under Bus A
    Device(XHC1) {
        Name(_ADR, 0x00000004)
        Name(_PRW, Package() { 0xb, 3 })
        Device (RHUB) {
            Name (_ADR, Zero)
            Device (HS05) { Name (_ADR, 1) }
            Device (HS06) { Name (_ADR, 2) }
            Device (SS05) { Name (_ADR, 3) }
        }

        Method(_S0W,0) {
            Return(0)
        }

        Method(_S3W,0) {
            Return(4)
        }

        Method(_S4W,0) {
            Return(4)
        }

    } /* end XHC1 */

    // 0.5      # Audio Processor
    // 0.6      # Audio Processor - HD Audio Controller
    Device(AZHD) {
        Name(_ADR, 0x00000006)
    } /* end AZHD */
    // 0.7      # non-Sensor Fusion Hub device
}

// 0:08.2   # Internal GPP Bridge 0 to Bus B
Device (PBRB){
    Name(_ADR, 0x00080002)
}
// 0:14.0   # SM
Device(SBUS) {
    Name(_ADR, 0x00140000)
}
// 0:14.3   # D14F3 bridge/LPC Bus
#include <soc/amd/common/acpi/lpc.asl>

/* Data Fabric Devices */
// 0:18.0
Device(DFD0) {
    Name(_ADR, 0x00180000)
}

// 0:18.1
Device(DFD1) {
    Name(_ADR, 0x00180001)
}

// 0:18.2
Device(DFD2) {
    Name(_ADR, 0x00180002)
}

// 0:18.3
Device(DFD3) {
    Name(_ADR, 0x00180003)
}

// 0:18.4
Device(DFD4) {
    Name(_ADR, 0x00180004)
}

// 0:18.5
Device(DFD5) {
    Name(_ADR, 0x00180005)
}

// 0:18.6
Device(DFD6) {
    Name(_ADR, 0x00180006)
}