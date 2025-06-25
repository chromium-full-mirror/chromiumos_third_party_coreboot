/* SPDX-License-Identifier: GPL-2.0-only */

#include <libpayload.h>

extern long int boot_hartid;

/**
 * This is our C entry function - set up the system
 * and jump into the payload entry point.
 */
void start_main(void);
void start_main(void)
{
	extern int main(int argc, char **argv);

	/* Gather system information. */
	lib_get_sysinfo();

#if !CONFIG(LP_SKIP_CONSOLE_INIT)
	console_init();
#endif

	/*
	 * Go to the entry point.
	 * In the future we may care about the return value.
	 */
	main(0, NULL);
	die("Unexpected return from payload\n");
}
