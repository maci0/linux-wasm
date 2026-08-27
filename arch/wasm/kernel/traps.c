// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/traps.c: faults on wasm are traps in the literal sense:
 * a wasm trap aborts the whole module.  We only provide the reporting hooks.
 */

#include <linux/bug.h>
#include <linux/kdebug.h>
#include <linux/sched/task.h>
#include <linux/sched.h>

void __noreturn die(const char *str, struct pt_regs *regs, long err)
{
	console_verbose();
	pr_emerg("wasmux: %s, err: %ld\n", str, err);
	make_task_dead(SIGSEGV);
}

void __warn_printk_on_stack(const char *str) { }
