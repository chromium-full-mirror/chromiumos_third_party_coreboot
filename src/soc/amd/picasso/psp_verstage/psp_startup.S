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

#include <arch/asm.h>
#include <vendorcode/amd/fsp/picasso/bl_errorcodes_public.h>

	.global Main
	.global _psp_vs_start

	.global  PSP_VERSTAGE_STACK_BASE
	.global  PSP_VERSTAGE_STACK_END
	.global  PSP_VERSTAGE_TEMP_STACK_END

.arm
.text
.section "PSP_HEADER_DATA", "aw", %note

//==============================================================================
// First 256 bytes of the binary image contain the header.
// Executable code starts from offset 0x100.
//==============================================================================
#include "header.inc"

//==============================================================================
// This is entry point to the binary which is called by main Boot Loader.
//==============================================================================

ENTRY(_psp_vs_start)
	// Map Debug Unlock stack to separate Virtual Address so that stack
	// overflow cause exception instead of data corruption.
	//

	ldr	sp, =PSP_VERSTAGE_TEMP_STACK_END // temporary stack pointer
	ldr	r2, =AllocateStack
	blx	r2

	// Return value contains Virtual Address of mapped stack
	//
	mov	sp, r0                  // set OEM PSP BL user app stack pointer
	ldr	lr, =ShouldNotBeReached // return address

	ldr	r2, =Main               // pass control to verstage main function
	blx	r2

// This point should not be reached. The Main() function should return
// to main BL using Svc_Exit().
//
ShouldNotBeReached:
	mov	r0, #BL_UAPP_ERR_RETURNED_FROM_MAIN  // Returned from Main
	svc	#0x0                    // SVC_EXIT

ENDPROC(_psp_vs_start)

//==============================================================================
// Code to allocate an address space for stack
//==============================================================================
.global AllocateStack

ENTRY(AllocateStack)
	push	{r3,lr}
	ldr	r0, =PSP_VERSTAGE_STACK_BASE
	mov	r2, sp
	ldr	r1, =PSP_VERSTAGE_STACK_END
	svc	#0x1                    // 0x1 is the SvcID for SVC_MAP_USER_STACK
	cmp	r0, #0
	bne	svcExit
	ldr	r0,[sp,#0]
	cmp	r0, #0                  // r0 contains the virtual stack address
	bne	ret

svcExit:
	mov	r0, #BL_UAPP_ERR_PSP_UAPP_ALLOC_STACK_FAIL
	svc	#0x0                    // SVC_EXIT

ret:
	pop	{r3,pc}

ENDPROC(AllocateStack)
.end
