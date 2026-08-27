// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/lib/mem.c: string/memory routines.
 *
 * wasm32-freestanding has no libc and no assembly-level memcpy; these are
 * the byte/word loops that lib/string.c normally provides.  They are built
 * with -fno-builtin so they never turn into recursive __builtin_* calls.
 */

#include <linux/types.h>

void *memcpy(void *dest, const void *src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;

	while (n--)
		*d++ = *s++;
	return dest;
}

void *memmove(void *dest, const void *src, size_t n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;

	if (d < s) {
		while (n--)
			*d++ = *s++;
	} else {
		d += n;
		s += n;
		while (n--)
			*--d = *--s;
	}
	return dest;
}

void *memset(void *s, int c, size_t n)
{
	unsigned char *p = s;

	while (n--)
		*p++ = (unsigned char)c;
	return s;
}
