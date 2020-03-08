//-----------------------------------------------------------------------------
// Copyright 2019 by AMD Inc.  All rights reserved.
//
// This document contains proprietary, confidential information that
// may be used, copied and/or disclosed only as authorized by a
// valid licensing agreement with ATI Technologies Inc. This copyright
// notice must be retained on all authorized copies.
//
// This code is provided "as is".  AMD Inc. makes, and
// the end user receives, no warranties or conditions, express,
// implied, statutory or otherwise, and ATI Technologies Inc.
// specifically disclaims any implied warranties of merchantability,
// non-infringement, or fitness for a particular purpose.
//
//-----------------------------------------------------------------------------

/*
 * TODO: Vaibhav will supply a new version of this header file with an
 * open-source license - b/153091404
 */

.arm
.global LastBytes
.section PSP_FOOTER_DATA, "ad", %note
.balign 64

// Note: this is used for determining the size of the binary. It is 64 byte aligned and 64 byte
//       in size so that the binary size is multiple of 64 bytes.
//
LastBytes:
    .byte     0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99
    .byte     0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99
    .byte     0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99
    .byte     0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99,0x99

.end
