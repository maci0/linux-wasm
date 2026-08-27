// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/jiffies.c: the kernel time base.
 *
 * Mainline defines jiffies_64 in kernel/time/timer.c and aliases the
 * 32-bit jiffies to its low word through the linker script.  wasm-ld
 * ignores linker-script symbol assignments, so wasm defines both here:
 * the alias attribute keeps jiffies and jiffies_64 at the same address
 * (wasm32 is little-endian, so jiffies is the low word).
 */

#include <linux/jiffies.h>
#include <linux/export.h>

__visible u64 jiffies_64 __cacheline_aligned_in_smp = INITIAL_JIFFIES;

/* the 32-bit view of jiffies_64, at the same address (see jiffies.h) */
extern unsigned long volatile jiffies __alias(jiffies_64);

EXPORT_SYMBOL(jiffies_64);
EXPORT_SYMBOL(jiffies);
