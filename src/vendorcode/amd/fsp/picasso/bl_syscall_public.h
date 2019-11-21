/*****************************************************************************
 *
 * Copyright (c) 2019, Advanced Micro Devices, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Advanced Micro Devices, Inc. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL ADVANCED MICRO DEVICES, INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ***************************************************************************/

#ifndef _BL_SYSCALL_PUBLIC_H_
#define _BL_SYSCALL_PUBLIC_H_


#define SVC_EXIT			0x00
#define SVC_MAP_USER_STACK		0x01
#define SVC_DEBUG_PRINT			0x06
#define SVC_DEBUG_PRINT_EX		0x1A
#define SVC_WAIT_10NS_MULTIPLE		0x1B
#define SVC_GET_BOOT_MODE		0x1C
#define SVC_DELAY_IN_MICRO_SECONDS	0x2F
#define SVC_GET_SPI_INFO		0x35
#define SVC_MAP_FCH_IO_DEVICE		0x36
#define SVC_UNMAP_FCH_IO_DEVICE		0x37
#define SVC_MAP_SPIROM_DEVICE		0x38
#define SVC_UNMAP_SPIROM_DEVICE		0x39
#define SVC_UPDATE_PSP_BIOS_DIR		0x40


typedef enum _PSP_BOOT_MODE
{
	PSP_BOOT_MODE_S0 = 0x0,
	PSP_BOOT_MODE_S0i3_RESUME = 0x1,
	PSP_BOOT_MODE_S3_RESUME = 0x2,
	PSP_BOOT_MODE_S4 = 0x3,
	PSP_BOOT_MODE_S5_COLD = 0x4,
	PSP_BOOT_MODE_S5_WARM = 0x5,
} PSP_BOOT_MODE;

/* TLB2_n settings for AWUSER and TLB3_n settings for ARUSER:
 * USER[0] - ReqIO bit, 1'b1 for FCH MMIO address
 * USER[1] - Compat bit, 1'b1 for FCH access, 0 for everything else
 * USER[2] - ByPass_IOMMU bit, 1'b1 to always bypass IOMMU, 0 for IOMMU translation
 */
typedef enum SYSHUB_TARGET_TYPE_E
{
	// Target Type			// Address		// [2:0] =[Bypass,Compat,ReqIO]
	AxUSER_PCIE_HT0 = 0x0,		// PCIe HT (Bypass=0)	// [2:0] =[0,0,0]
	AxUSER_DRAM_VIA_IOMMU = 0x1,	// DRAM ACCESS via IOMMU// [2:0] =[0,0,1]
	AxUSER_PCIE_HT1 = 0x2,		// PCIe HT  (Bypass=1)	// [2:0] =[0,1,0]
	AxUSER_RSVD = 0x3,		// - NOT USED ,INVALID 	// [2:0] =[0,1,1]
	AxUSER_DRAM_BYPASS_IOMMU = 0x4,	// GENERAL DRAM 	// [2:0] =[1,0,0]
	AxUSER_PCIE_MMIO = 0x5,		// PCIe MMIO		// [2:0] =[1,0,1]
	AxUSER_FCH_HT_IO = 0x6,		// FCH HT (port80)	// [2:0] =[1,1,0]
	AxUSER_FCH_MMIO = 0x6		// FCH MMIO 		// [2:0] =[1,1,1]
} SYSHUB_TARGET_TYPE;

typedef enum FCH_IO_DEVICE {
	FCH_IO_DEVICE_SPI,
	FCH_IO_DEVICE_I2C,
	FCH_IO_DEVICE_GPIO,
	FCH_IO_DEVICE_eSPI,
	FCH_IO_DEVICE_END,
} FCH_IO_DEVICE;

/* Svc_UpdatePspBiosDir can be used to GET or SET the PSP or BIOS directory
 * offsets. This enum is used to specify whether it is a GET or SET operation.
 */
typedef enum DIR_OFFSET_OPERATION_E {
	DIR_OFFSET_GET = 0x0,
	DIR_OFFSET_SET,
	DIR_OFFSET_OPERATION_MAX
} DIR_OFFSET_OPERATION;

typedef enum FCH_I2C_CONTROLLER_ID_E
{
	FCH_I2C_CONTROLLER_ID_2 = 2,
	FCH_I2C_CONTROLLER_ID_3 = 3,
	FCH_I2C_CONTROLLER_ID_4 = 4,
	FCH_I2C_CONTROLLER_ID_MAX,
} FCH_I2C_CONTROLLER_ID;

typedef struct SPIROM_INFO
{
	unsigned int SpiBiosSysHubBase;
	unsigned int SpiBiosSmnBase;
	unsigned int SpiBiosSize;
} SPIROM_INFO;

typedef struct SYSHUB_RW_PARMS_EX_E
{
	unsigned int SyshubAddressLo;
	unsigned int SyshubAddressHi;
	unsigned int *pValue;
	unsigned int Size;
	SYSHUB_TARGET_TYPE TargetType;
} SYSHUB_RW_PARMS_EX;


/* Exit to the main Boot Loader. This does not return back to user application.
 *
 * Parameters:
 *     Status  -   either Ok or error code defined by AGESA
 */
__svc(SVC_EXIT) void Svc_Exit(unsigned int Status);


/* Maps buffer for stack usage.
 *
 * Parameters:
 *     StartAddr   -   start address of the stack buffer
 *     EndAddr     -   end of the stack buffer
 *     pStackVa    -   [out] mapped stack Virtual Address
 *
 * Return value: BL_OK or error code
 */
__svc(SVC_MAP_USER_STACK) unsigned int Svc_MapUserStack(unsigned int StartAddr,
		unsigned int EndAddr, unsigned int *pStackVa);


/* Print debug message into serial console.
 *
 * Parameters:
 *     pString     -   null-terminated string
 */
__svc(SVC_DEBUG_PRINT) void Svc_DebugPrint(char *pString);


/* Print 4 DWORD values in hex to serial console
 *
 * Parameters:
 *     Dword0...Dword3 - 32-bit DWORD to print
 */
__svc(SVC_DEBUG_PRINT_EX) void Svc_DebugPrintEx(unsigned int Dword0,
		unsigned int Dword1, unsigned int Dword2, unsigned int Dword3);


/* Waits in a blocking call for multiples of 10ns (100MHz timer) before returning
 *
 * Parameters:
 *     Multiple    - The number of multiples of 10ns to wait
 *
 * Return value: BL_OK, or BL_ERR_TIMER_PARAM_OVERFLOW
 */
__svc(SVC_WAIT_10NS_MULTIPLE) unsigned int Svc_Wait10nsMultiple(unsigned int Multiple);


/* Description     - Returns the current boot mode from the type PSP_BOOT_MODE found in
 *                   bl_public.h.
 *
 * Inputs          - pBootMode - Output parameter passed in R0
 *
 * Outputs         - The boot mode in pBootMode.
 *                   See Return Values.
 *
 * Return Values   - BL_OK
 *                   BL_ERR_NULL_PTR
 *                   Other BL_ERRORs lofted up from called functions
 */
__svc(SVC_GET_BOOT_MODE) unsigned int Svc_GetBootMode(unsigned int *pBootMode);


/* Add delay in micro seconds
 *
 * Parameters:
 *     delay       - required delay value in microseconds
 *
 * Return value: NONE
 */
__svc(SVC_DELAY_IN_MICRO_SECONDS) void Svc_DelayInMicroSeconds(unsigned int delay);


/* Get the SPI-ROM information
 *
 * Parameters:
 *     SpiRomInfo  - SPI-ROM information
 *
 * Return value: BL_OK or error code
 */
__svc(SVC_GET_SPI_INFO) unsigned int Svc_GetSpiRomInfo(SPIROM_INFO *pSpiRomInfo);


/* Map the FCH IO device register space (SPI/I2C/GPIO/eSPI/etc...)
 *
 * Parameters:
 *     IODevice          - ID for respective FCH IO controller register space to be mapped
 *     Agr1              - Based on IODevice ID, interpretation of this argument changes.
 *     Arg2              - Based on IODevice ID, interpretation of this argument changes.
 *     ppIODeviceAddrAxi - AXI address, for respective FCH IO device register space
 *
 * Return value: BL_OK or error code
 */
__svc(SVC_MAP_FCH_IO_DEVICE) unsigned int Svc_MapFchIODevice(FCH_IO_DEVICE IODevice,
		unsigned int Arg1, unsigned int Arg2, void **ppIODeviceAddrAxi);


/* Unmap the FCH IO device register space mapped earlier using Svc_MapFchIODevice()
 *
 * Parameters:
 *     IODevice        - ID for respective FCH IO controller register space to be unmapped
 *     IODeviceAddrAxi - AXI address, for respective FCH IO device register space
 *
 * Return value: BL_OK or error code
 */
__svc(SVC_UNMAP_FCH_IO_DEVICE) unsigned int Svc_UnMapFchIODevice(FCH_IO_DEVICE IODevice,
		void *IODeviceAddrAxi);


/* Map the SPIROM FLASH device address space
 *
 * Parameters:
 *     SpiRomAddr     - Address in SPIROM tobe mapped (SMN based)
 *     size           - Size to be mapped
 *     pSpiRomAddrAxi - Mapped address in AXI space
 *
 * Return value: BL_OK or error code
 */
__svc(SVC_MAP_SPIROM_DEVICE) unsigned int Svc_MapSpiRomDevice(unsigned int SpiRomAddr,
		unsigned int size, void **ppSpiRomAddrAxi);


/* Unmap the SPIROM FLASH device address space mapped earlier using Svc_MapSpiRomDevice()
 *
 * Parameters:
 *     pSpiRomAddrAxi - Address in AXI address space previously mapped
 *
 * Return value: BL_OK or error code
 */
__svc(SVC_UNMAP_SPIROM_DEVICE) unsigned int Svc_UnMapSpiRomDevice(void *pSpiRomAddrAxi);


/* Updates the offset at which PSP or BIOS Directory can be found in the
 * SPI flash
 *
 * Parameters:
 *     pPspDirOffset  - [in/out] Offset at which PSP Directory can be
 *                      found in the SPI Flash. Same pointer is used
 *                      to return the offset in case of GET opertaion
 *     pBiosDirOffset - [in/out] Offset at which BIOS Directory can be
 *                      found in the SPI Flash. Same pointer is used
 *                      to return the offset in case of GET opertaion
 *     Operation      - [in] Specifies whether this call is used for
 *                      getting or setting the offset.
 *
 * Return value: BL_OK or error code
 */
__svc(SVC_UPDATE_PSP_BIOS_DIR) unsigned int Svc_UpdatePspBiosDir(unsigned int *pPspDirOffset,
		unsigned int *pBiosDirOffset, DIR_OFFSET_OPERATION Operation);

#endif /* _BL_SYSCALL__PUBLIC_H_ */
