/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_SPINLOCK_H
#define __ASM_WASM_SPINLOCK_H

/* single-threaded wasm: use the generic ticket-free test-and-set lock */
#include <asm-generic/spinlock.h>

/* UP spinlock trylock stubs */
static inline int __raw_spin_trylock(raw_spinlock_t *lock)
{
	return 1; /* always succeeds (single-threaded) */
}

static inline int __raw_spin_trylock_bh(raw_spinlock_t *lock)
{
	return 1;
}

#endif

#include <linux/irqflags.h>

/* wasm: bh locking is a no-op; override the SMP inline that pulls in a
 * computed goto via __local_bh_enable_ip */
#define __raw_spin_unlock_bh(lock) do { do_raw_spin_unlock(lock); } while (0)
