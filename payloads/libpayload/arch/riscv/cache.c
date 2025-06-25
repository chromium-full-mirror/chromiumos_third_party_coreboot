/* SPDX-License-Identifier: GPL-2.0-only */

#include <arch/cache.h>

void tlb_invalidate_all(void)
{
    __asm__ __volatile__ ("sfence.vma" : : : "memory");
}

void dcache_clean_invalidate_all(void)
{

}