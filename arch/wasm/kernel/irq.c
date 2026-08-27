// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/irq.c: irq delivery for the wasm port.
 *
 * IRQ 0 is the timer; IRQ 1 is the network device.  The runtime raises
 * them by calling wasm_raise_irq() exported from this file, which runs the
 * generic IRQ machinery exactly as UML's signal handlers do.
 */

#include <linux/init.h>
#include <linux/irq.h>
#include <linux/interrupt.h>

#define WASM_TIMER_IRQ	0
#define WASM_LAST_IRQ	16

static void dummy_action(struct irq_data *d) { }

static struct irq_chip wasm_irq_chip = {
	.name = "wasm",
	.irq_disable = dummy_action,
	.irq_enable = dummy_action,
	.irq_ack = dummy_action,
	.irq_mask = dummy_action,
	.irq_unmask = dummy_action,
};

/* called from the host runtime (via an exported wasm function) */
void wasm_raise_irq(int irq)
{
	struct pt_regs regs;

	memset(&regs, 0, sizeof(regs));
	do_IRQ(irq, &regs);
}

/* Generic IRQ dispatch, UML-style (see arch/um/kernel/irq.c). */
unsigned int do_IRQ(int irq, struct pt_regs *regs)
{
	struct pt_regs *old_regs = set_irq_regs(regs);

	irq_enter();
	generic_handle_irq(irq);
	irq_exit();
	set_irq_regs(old_regs);
	return 1;
}

void __init init_IRQ(void)
{
	int i;

	irq_set_chip_and_handler(WASM_TIMER_IRQ, &wasm_irq_chip,
				 handle_percpu_irq);
	for (i = 1; i < WASM_LAST_IRQ; i++)
		irq_set_chip_and_handler(i, &wasm_irq_chip,
					 handle_edge_irq);
}

int __init arch_probe_nr_irqs(void)
{
	return WASM_LAST_IRQ;
}
