/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_UACCESS_H
#define __ASM_WASM_UACCESS_H

/*
 * wasm32 flat address space: all pointers are kernel pointers.
 * We provide the complete uaccess API without including the generic
 * one, which gets the INLINE_COPY ordering wrong for arch overrides.
 */

#include <linux/string.h>
#include <linux/errno.h>
#include <asm/segment.h>

#define TASK_SIZE		(0x80000000UL)
#define TASK_SIZE_MAX		TASK_SIZE
#define USER_DS		((mm_segment_t){ 0 })
#define KERNEL_DS	((mm_segment_t){ ~0UL })
#define get_fs()	(USER_DS)
#define set_fs(x)		do { } while (0)
#define uaccess_kernel()	(0)

#define access_ok(ptr, size)	(1)
#define __access_ok(ptr, size)	(1)

static inline __must_check unsigned long
raw_copy_to_user(void __user *to, const void *from, unsigned long n)
{
	memcpy((void __force *)to, from, n);
	return 0;
}

static inline __must_check unsigned long
raw_copy_from_user(void *to, const void __user *from, unsigned long n)
{
	memcpy(to, (const void __force *)from, n);
	return 0;
}

#define INLINE_COPY_TO_USER
#define INLINE_COPY_FROM_USER

#include <asm/extable.h>

/* strncpy/strnlen from lib/strn*.c; declare to satisfy headers */
extern long strncpy_from_user(char *dst, const char __user *src, long count);
extern long strnlen_user(const char __user *src, long count);

/*
 * The generic fallback implementations that call raw_copy_*
 * are in include/linux/uaccess.h which includes this file.
 * We just need raw_copy_* and the INLINE_COPY flags defined.
 */

#endif /* __ASM_WASM_UACCESS_H */

/* ---- get_user/put_user/clear_user (flat memory, no faults possible) ---- */

#define __put_user(x, ptr)						\
({									\
	__typeof__(*(ptr)) __user *__p = (ptr);				\
	*__p = (x);							\
	0;								\
})

#define put_user(x, ptr)	__put_user((x), (ptr))

#define __get_user(x, ptr)						\
({									\
	__typeof__(*(ptr)) __user *__p = (ptr);				\
	(x) = *__p;							\
	0;								\
})

#define get_user(x, ptr)	__get_user((x), (ptr))

#define __get_user_fn(sz, u, k) ({ *(void *)(k) = *(const void *)(u); 0; })
#define __put_user_fn(sz, u, k) ({ *(void *)(u) = *(const void *)(k); 0; })

static inline __must_check unsigned long
clear_user(void __user *to, unsigned long n)
{
	memset((void __force *)to, 0, n);
	return 0;
}
#define __clear_user(to, n)	clear_user(to, n)

extern int __get_user_bad(void);

