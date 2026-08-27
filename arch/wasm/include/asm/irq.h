/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_IRQ_H
#define __ASM_WASM_IRQ_H

#define TIMER_IRQ		0
#define WASM_NET_IRQ		1
#define WASM_LAST_STATIC_IRQ	2
#define NR_IRQS			32

struct pt_regs;
unsigned int do_IRQ(int irq, struct pt_regs *regs);

#include <asm-generic/irq.h>
#endif
