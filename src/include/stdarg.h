/* SPDX-License-Identifier: GPL-2.0-only */

/**
 * Note: This file is only for POSIX compatibility, and is meant to be
 * chain-included via string.h.
 */

#ifndef STDARG_H
#define STDARG_H

#include <stddef.h>

#define va_start(v, l)		__builtin_va_start(v, l)
#define va_end(v)		__builtin_va_end(v)
#define va_arg(v, l)		__builtin_va_arg(v, l)
typedef __builtin_va_list	va_list;

#if !defined(__ROMCC__)
int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
#endif

#endif /* STDARG_H */
