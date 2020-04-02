/* SPDX-License-Identifier: GPL-2.0-only */
/* This file is part of the coreboot project. */

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <arch/exception.h>
#include <arch/hlt.h>
#include <security/vboot/vboot_common.h>
#include <vendorcode/amd/fsp/picasso/bl_syscall_public.h>
#include <delay.h>
#include <drivers/i2c/designware/dw_i2c.h>
#include <reset.h>
#include <boot_device.h>
#include <console/console.h>
#include <security/vboot/vboot_common.h>
#include <bootmode.h>
#include <timer.h>
#include <halt.h>
#include <string.h>
#include <timestamp.h>
#include <assert.h>
#include <stdint.h>
#include <security/vboot/symbols.h>

#define RUN_PSP_SVC_TESTS 1

static struct mem_region_device boot_dev =
		MEM_REGION_DEV_RO_INIT(NULL, CONFIG_ROM_SIZE);


void __weak verstage_mainboard_init(void)
{
	/* Default empty implementation. */
}

static void test_svc_calls(void)
{
	struct SPIROM_INFO spi = {0};
	uint32_t *addr = NULL;


	/* Test svc_get_spi_rom_info & svc_map_spi_rom */
	svc_debug_print("\nTest: Getting SPI ROM info.\n");
	if (svc_get_spi_rom_info(&spi))
		svc_debug_print("Error getting SPI ROM info.\n");

	if (spi.SpiBiosSmnBase != 0)
		if (svc_map_spi_rom(spi.SpiBiosSmnBase, CONFIG_ROM_SIZE, (void **)&addr))
			svc_debug_print("Error mapping SPI ROM to address.\n");
	printk(BIOS_DEBUG,"SPI ROM info:\n");
	printk(BIOS_DEBUG,"SpiBiosSysHubBase: 0x%08x\n", spi.SpiBiosSysHubBase);
	printk(BIOS_DEBUG,"SpiBiosSmnBase: 0x%08x\n", spi.SpiBiosSmnBase);
	printk(BIOS_DEBUG,"SpiBiosSize: 0x%08x\n", spi.SpiBiosSize);
	printk(BIOS_DEBUG,"addr: 0x%08x\n", addr);

	/* Test svc_debug_print_ex, svc_read_timer_val, svc_wait_10ns_multiple, and svc_delay_in_usec */
	uint64_t timer1;
	uint64_t timer2;
	printk(BIOS_DEBUG,"\nTest: Reading chrono timer twice, delaying 10 uSec, then printing both values\n");
	svc_read_timer_val(PSP_TIMER_TYPE_CHRONO, &timer1);
	svc_wait_10ns_multiple(10000);
	svc_read_timer_val(PSP_TIMER_TYPE_CHRONO, &timer2);

	printk(BIOS_DEBUG,"Chrono timer info:\n");
	printk(BIOS_DEBUG,"Initial timer: 0x%08x_%08x\n", (uint32_t)(timer1>>32),(uint32_t)timer1);
	printk(BIOS_DEBUG,"2nd timer: 0x%08x_%08x\n", (uint32_t)(timer2>>32),(uint32_t)timer2);
	printk(BIOS_DEBUG,"Difference: 0x%08x_%08x\n", (uint32_t)((timer2-timer1)>>32),(uint32_t)(timer2-timer1));
	svc_debug_print_ex((uint32_t)(timer1>>32),(uint32_t)timer1,(uint32_t)(timer2>>32),(uint32_t)timer2);

	printk(BIOS_DEBUG,"\nTest: Reading RTC timer twice, delaying 1 Sec, then printing both values\n");
	svc_read_timer_val(PSP_TIMER_TYPE_CHRONO, &timer1);
	svc_delay_in_usec(1000000);
	svc_read_timer_val(PSP_TIMER_TYPE_CHRONO, &timer2);

	printk(BIOS_DEBUG,"RTC timer info:\n");
	printk(BIOS_DEBUG,"Initial timer: 0x%08x_%08x\n", (uint32_t)(timer1>>32),(uint32_t)timer1);
	printk(BIOS_DEBUG,"2nd timer: 0x%08x_%08x\n", (uint32_t)(timer2>>32),(uint32_t)timer2);
	printk(BIOS_DEBUG,"Difference: 0x%08x_%08x\n", (uint32_t)((timer2-timer1)>>32),(uint32_t)(timer2-timer1));
	svc_debug_print_ex((uint32_t)(timer1>>32),(uint32_t)timer1,(uint32_t)(timer2>>32),(uint32_t)timer2);

	/* Test svc_get_boot_mode */
	uint32_t bootmode = 0;
	printk(BIOS_DEBUG,"\nTest: Getting boot mode.\n");
	if (svc_get_boot_mode(&bootmode))
		printk(BIOS_DEBUG,"Error getting boot mode.\n");

	if (bootmode == PSP_BOOT_MODE_S0)
		printk(BIOS_DEBUG,"Platform is NOT resuming.  What does this value even mean?\n");
	else if (bootmode == PSP_BOOT_MODE_S5_COLD)
		printk(BIOS_DEBUG,"Platform is NOT resuming. Cold boot\n");
	else if (bootmode == PSP_BOOT_MODE_S5_COLD)
		printk(BIOS_DEBUG,"Platform is NOT resuming. Warm boot\n");
	else if (bootmode == PSP_BOOT_MODE_S3_RESUME)
		printk(BIOS_DEBUG,"Platform is resuming from S3.\n");
	else if (bootmode == PSP_BOOT_MODE_S0i3_RESUME)
		printk(BIOS_DEBUG,"Platform is resuming from S0i3.\n");
	else if (bootmode == PSP_BOOT_MODE_S4)
		printk(BIOS_DEBUG,"S4?  Really?\n");

	printk(BIOS_DEBUG,"\nTest: Saving workbuf\n");
	svc_save_uapp_data(UAPP_COPYBUF_CHROME_WORKBUF, _vboot2_work, VB2_KERNEL_WORKBUF_RECOMMENDED_SIZE);

}

static uintptr_t *map_spi_rom(void)
{
	uintptr_t *addr = NULL;
	struct SPIROM_INFO spi = {0};

	printk(BIOS_DEBUG,"Getting SPI ROM info.\n");
	if (svc_get_spi_rom_info(&spi))
		printk(BIOS_DEBUG,"Error getting SPI ROM info.\n");

	if (spi.SpiBiosSmnBase != 0)
		if (svc_map_spi_rom(spi.SpiBiosSmnBase, CONFIG_ROM_SIZE, (void **)&addr))
			printk(BIOS_DEBUG,"Error mapping SPI ROM to address.\n");

	return addr;
}

extern char _bss_start, _bss_end;

void Main(void)
{
	uint32_t retval = 0;

	printk(BIOS_DEBUG,"Entering verstage on PSP\n");
	memset(&_bss_start, '\0', &_bss_end - &_bss_start);

	verstage_mainboard_init();
	verstage_main();

	#if 0 //TODO
	uint32_t *psp_dir_offset = NULL;
	uint32_t *bios_dir_offset = NULL;
	uint32_t *workbuf_addr = NULL;

	if (svcc_save_uapp_data(UAPP_COPYBUF_CHROME_WORKBUF, workbuf_addr,
			VB2_FIRMWARE_WORKBUF_RECOMMENDED_SIZE))
		printk(BIOS_ERR,"Error: could not save workbuf\n");

	svc_update_psp_bios_dir(psp_dir_offset, bios_dir_offset, DIR_OFFSET_SET);
	#endif

	if (RUN_PSP_SVC_TESTS)
		test_svc_calls();

	if (boot_dev.base){
		printk(BIOS_DEBUG,"Unmapping SPI rom\n");
		if (svc_unmap_spi_rom((void *)boot_dev.base))
			printk(BIOS_ERR,"Error unmapping SPI rom\n");
	}

	printk(BIOS_DEBUG,"Leaving verstage on PSP\n");
	svc_exit(retval);
	return;
}

void udelay(uint32_t usecs)
{
	svc_delay_in_usec(usecs);
}


int vboot_platform_is_resuming(void)
{
	uint32_t bootmode = 0;
	if (svc_get_boot_mode(&bootmode))
		printk(BIOS_ERR,"Error getting boot mode.\n");

	if ((bootmode == PSP_BOOT_MODE_S3_RESUME) || (bootmode == PSP_BOOT_MODE_S0i3_RESUME))
		bootmode=1;

	return bootmode;
}

const struct region_device *boot_device_ro(void)
{
	uintptr_t *addr;

	addr = map_spi_rom();
	mem_region_device_ro_init(&boot_dev, (void *)addr, CONFIG_ROM_SIZE);

	return &boot_dev.rdev;
}

void printk(int LEVEL, const char *fmt, ...)
{
	if (LEVEL > CONFIG_DEFAULT_CONSOLE_LOGLEVEL)
		return;

	va_list args;
	char buf[128];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	svc_debug_print(buf);
}

void __noreturn die(const char *fmt, ...)
{
	va_list args;
	char buf[128];

	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	svc_debug_print(buf);
	halt();
}


/* Stubs that still need to be implemented */

void timer_monotonic_get(struct mono_time *mt)
{
	svc_read_timer_val(PSP_TIMER_TYPE_CHRONO, (uint64_t *)&mt->microseconds);
}

uintptr_t dw_i2c_base_address(uint32_t bus)
{
	// Map it using the svc_map_fch_dev call

	return 0;
}

void do_board_reset(void)
{
	svc_reset_system(RESET_TYPE_WARM);
}

int get_recovery_mode_switch(void)
{
	return 0;
}
int get_lid_switch(void)
{
	return 0;
}

void post_code(u8 value)
{
	printk(BIOS_WARNING, "Post code: 0x%02x", value);
	return;
}

void timestamp_add_now(enum timestamp_id id)
{
	return;
}

void timestamp_add(enum timestamp_id id, uint64_t ts)
{
	return;
}

uint64_t timestamp_get(void)
{
	return 0;
}
