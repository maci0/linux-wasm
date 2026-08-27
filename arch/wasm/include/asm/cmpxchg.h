/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_CMPXCHG_H
#define __ASM_WASM_CMPXCHG_H

#include <linux/types.h>

/*
 * wasm32 runs single-threaded and non-preemptible (no SMP, no async
 * interrupts), so compare-and-swap never contends: a plain volatile load,
 * compare and conditional store is exactly correct.
 *
 * Do NOT use the __atomic_* builtins here: with LLVM's
 * -mthread-model single they lower to a sequence that treats the expected
 * value as zero (the store becomes `*ptr ? *ptr : new`), turning arch_xchg
 * into an infinite spin that never installs the new value.
 *
 * Plain volatile access is used instead of READ_ONCE/WRITE_ONCE because
 * this header is included from very early contexts where the kernel's
 * compiler.h helpers are not yet in scope.
 */

#define __wasm_atomic_load(ptr)		(*(volatile typeof(*(ptr)) *)(ptr))
#define __wasm_atomic_store(ptr, val)	(*(volatile typeof(*(ptr)) *)(ptr) = (val))

#define __arch_cmpxchg_wasm(ptr, o, n) ({				\
	typeof(*(ptr)) __a = __wasm_atomic_load(ptr);			\
	if (__a == (o))							\
		__wasm_atomic_store(ptr, (n));				\
	__a; })

#define arch_cmpxchg(ptr, o, n)		__arch_cmpxchg_wasm((ptr), (o), (n))
#define arch_cmpxchg_local(ptr, o, n)	__arch_cmpxchg_wasm((ptr), (o), (n))
#define arch_cmpxchg64(ptr, o, n)	__arch_cmpxchg_wasm((ptr), (o), (n))

#define arch_try_cmpxchg(ptr, oldp, new) ({				\
	bool __ret;							\
	typeof(*(ptr)) __old = *(oldp);					\
	typeof(*(ptr)) __r = arch_cmpxchg((ptr), __old, (new));		\
	__ret = (__r == __old);						\
	if (!__ret) *(oldp) = __r;					\
	__ret; })

#define arch_xchg(ptr, x) ({						\
	typeof(*(ptr)) __ret = __wasm_atomic_load(ptr);			\
	__wasm_atomic_store(ptr, (x));					\
	__ret; })

#endif /* __ASM_WASM_CMPXCHG_H */
