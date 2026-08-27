/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_CMPXCHG_H
#define __ASM_WASM_CMPXCHG_H

#include <linux/types.h>

/*
 * wasm32: LLVM's atomicrmw/cmpxchg map to single-threaded semantics with
 * -mthread-model single, so plain __atomic builtins are correct and cheap.
 */
static inline unsigned long __cmpxchg_wasm(volatile void *ptr, unsigned long old,
					   unsigned long new, int size)
{
	switch (size) {
	case 1: {
		unsigned char expected = old;
		__atomic_compare_exchange_n((volatile unsigned char *)ptr,
			&expected, (unsigned char)new, 0,
			__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
		return expected;
	}
	case 2: {
		unsigned short expected = old;
		__atomic_compare_exchange_n((volatile unsigned short *)ptr,
			&expected, (unsigned short)new, 0,
			__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
		return expected;
	}
	case 4: {
		unsigned int expected = old;
		__atomic_compare_exchange_n((volatile unsigned int *)ptr,
			&expected, (unsigned int)new, 0,
			__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
		return expected;
	}
	default:
		__builtin_unreachable();
	}
	return old;
}

/*
 * Generic cmpxchg preserving the pointed-to type (needed by try_cmpxchg's
 * typeof() chains).  Dispatches on sizeof via _Generic.
 */
#define __arch_cmpxchg_wasm(ptr, o, n) ({					\
	typeof(*(ptr)) __o = (o);						\
	typeof(*(ptr)) __n = (n);						\
	typeof(*(ptr)) __e = __o;						\
	(void)__atomic_compare_exchange_n((ptr), &__e, __n, 0,			\
			__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);			\
	__e;									\
})

#define arch_cmpxchg(ptr, o, n)		__arch_cmpxchg_wasm((ptr), (o), (n))
#define arch_cmpxchg_local(ptr, o, n)	__arch_cmpxchg_wasm((ptr), (o), (n))
#define arch_cmpxchg64(ptr, o, n)	({ \
	BUILD_BUG_ON(sizeof(*(ptr)) != 8);	\
	__arch_cmpxchg_wasm((ptr), (o), (n)); })

#define arch_try_cmpxchg(ptr, oldp, new) ({				\
	bool __ret;							\
	typeof(*(ptr)) __old = *(oldp);					\
	typeof(*(ptr)) __r = arch_cmpxchg((ptr), __old, (new));		\
	__ret = (__r == __old);						\
	if (!__ret) *(oldp) = __r;					\
	__ret; })

#define arch_xchg(ptr, x) ({						\
	typeof(*(ptr)) __new = (x);					\
	typeof(*(ptr)) __old;						\
	do {								\
		__old = READ_ONCE(*(ptr));				\
	} while (!arch_try_cmpxchg((ptr), &__old, __new));		\
	__old; })

#endif /* __ASM_WASM_CMPXCHG_H */
