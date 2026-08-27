/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_STRING_H
#define __ASM_WASM_STRING_H

#define __HAVE_ARCH_MEMCPY 1
#define __HAVE_ARCH_MEMSET 1
#define __HAVE_ARCH_MEMMOVE 1

extern void *memcpy(void *dest, const void *src, size_t n);
extern void *memset(void *s, int c, size_t n);
extern void *memmove(void *dest, const void *src, size_t n);

#include <asm-generic/string.h>

#endif
