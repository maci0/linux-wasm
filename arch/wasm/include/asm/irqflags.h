/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_IRQFLAGS_H
#define __ASM_WASM_IRQFLAGS_H

/*
 * wasm32 is single-threaded and cannot be interrupted asynchronously:
 * "interrupts disabled" is a compile-time constant.  The flags-taking
 * forms accept and ignore the argument (matching raw_irqs_disabled_flags).
 */
#define arch_local_irq_disable()	do { } while (0)
#define arch_local_irq_enable()		do { } while (0)
#define arch_local_irq_save()		(0UL)
#define arch_local_irq_restore(x)	do { (void)(x); } while (0)
#define arch_irqs_disabled()		(1)
#define arch_irqs_disabled_flags(x)	(1)

#define arch_local_save_flags arch_local_save_flags
static inline unsigned long arch_local_save_flags(void)
{
	return 0;
}

#include <asm-generic/irqflags.h>

#endif
