/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_ATOMIC_H
#define __ASM_WASM_ATOMIC_H

#include <linux/types.h>
#include <asm/cmpxchg.h>

/*
 * 32-bit target: linux/types.h only defines atomic64_t under CONFIG_64BIT.
 * Define it here (guarded) so the generic atomic64 fallback code compiles;
 * the actual ops come from include/linux/atomic/atomic-long.h via the
 * fallback layer, implemented with cmpxchg64 loops on this single-threaded
 * target.
 */
#ifndef _LINUX_TYPES_ATOMIC64_H
#define _LINUX_TYPES_ATOMIC64_H
typedef struct {
	s64 counter;
} atomic64_t;
#endif

#define ATOMIC_INIT(i)	{ (i) }
#define ATOMIC64_INIT(i) { (i) }

/* relaxed load/store */
#define arch_atomic_read(v)		__atomic_load_n(&(v)->counter, __ATOMIC_RELAXED)
#define arch_atomic_set(v, i)		__atomic_store_n(&(v)->counter, (i), __ATOMIC_RELAXED)
#define arch_atomic64_read(v)		__atomic_load_n(&(v)->counter, __ATOMIC_RELAXED)
#define arch_atomic64_set(v, i)		__atomic_store_n(&(v)->counter, (i), __ATOMIC_RELAXED)

/* returning add/sub */
#define arch_atomic_add_return(a, v)	__atomic_add_fetch(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic_sub_return(a, v)	__atomic_sub_fetch(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic_inc_return(v)	arch_atomic_add_return(1, (v))
#define arch_atomic_dec_return(v)	arch_atomic_sub_return(1, (v))
#define arch_atomic64_add_return(a, v)	__atomic_add_fetch(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic64_sub_return(a, v)	__atomic_sub_fetch(&(v)->counter, (a), __ATOMIC_SEQ_CST)

/* plain (non-returning) ops (the fallback layer calls these directly) */
#define arch_atomic_add(a, v)		((void)__atomic_fetch_add(&(v)->counter, (a), __ATOMIC_SEQ_CST))
#define arch_atomic_sub(a, v)		((void)__atomic_fetch_sub(&(v)->counter, (a), __ATOMIC_SEQ_CST))
#define arch_atomic_inc(v)		arch_atomic_add(1, (v))
#define arch_atomic_dec(v)		arch_atomic_sub(1, (v))
#define arch_atomic_and(a, v)		((void)__atomic_fetch_and(&(v)->counter, (a), __ATOMIC_SEQ_CST))
#define arch_atomic_or(a, v)		((void)__atomic_fetch_or(&(v)->counter, (a), __ATOMIC_SEQ_CST))
#define arch_atomic_xor(a, v)		((void)__atomic_fetch_xor(&(v)->counter, (a), __ATOMIC_SEQ_CST))

#define arch_atomic64_add(a, v)		((void)__atomic_fetch_add(&(v)->counter, (a), __ATOMIC_SEQ_CST))
#define arch_atomic64_sub(a, v)		((void)__atomic_fetch_sub(&(v)->counter, (a), __ATOMIC_SEQ_CST))
#define arch_atomic64_and(a, v)		((void)__atomic_fetch_and(&(v)->counter, (a), __ATOMIC_SEQ_CST))
#define arch_atomic64_or(a, v)		((void)__atomic_fetch_or(&(v)->counter, (a), __ATOMIC_SEQ_CST))
#define arch_atomic64_xor(a, v)		((void)__atomic_fetch_xor(&(v)->counter, (a), __ATOMIC_SEQ_CST))
#define arch_atomic64_inc_return(v)	arch_atomic64_add_return(1, (v))
#define arch_atomic64_dec_return(v)	arch_atomic64_sub_return(1, (v))

/* fetch ops the fallback layer uses */
#define arch_atomic_fetch_add(a, v)	__atomic_fetch_add(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic_fetch_sub(a, v)	__atomic_fetch_sub(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic_fetch_and(a, v)	__atomic_fetch_and(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic_fetch_or(a, v)	__atomic_fetch_or(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic_fetch_xor(a, v)	__atomic_fetch_xor(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic64_fetch_add(a, v)	__atomic_fetch_add(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic64_fetch_sub(a, v)	__atomic_fetch_sub(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic64_fetch_and(a, v)	__atomic_fetch_and(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic64_fetch_or(a, v)	__atomic_fetch_or(&(v)->counter, (a), __ATOMIC_SEQ_CST)
#define arch_atomic64_fetch_xor(a, v)	__atomic_fetch_xor(&(v)->counter, (a), __ATOMIC_SEQ_CST)

/* exchange / compare-exchange (plain-C, see asm/cmpxchg.h for why the
 * __atomic_* builtins must not be used) */
#define arch_atomic_xchg(v, n)		arch_xchg(&(v)->counter, (n))
#define arch_atomic64_xchg(v, n)	arch_xchg(&(v)->counter, (n))

static inline int arch_atomic_cmpxchg(atomic_t *v, int old, int new)
{
	return arch_cmpxchg(&v->counter, old, new);
}

static inline s64 arch_atomic64_cmpxchg(atomic64_t *v, s64 old, s64 new)
{
	return arch_cmpxchg64(&v->counter, old, new);
}

#define arch_atomic_try_cmpxchg(v, p, n)	arch_try_cmpxchg(&(v)->counter, (p), (n))
#define arch_atomic64_try_cmpxchg(v, p, n)	arch_try_cmpxchg(&(v)->counter, (p), (n))

#endif /* __ASM_WASM_ATOMIC_H */
