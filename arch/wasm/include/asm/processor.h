/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_PROCESSOR_H
#define __ASM_WASM_PROCESSOR_H

#include <asm/page.h>

struct cpuinfo_wasm { int dummy; };

#define cpu_relax()		do { } while (0)
#define cpu_idle_wait()		do { } while (0)

struct pt_regs;
struct task_struct;

struct thread_struct {
	unsigned long sp;
	unsigned long entry;	/* kernel-thread entry (see __switch_to) */
	unsigned long arg;	/* argument passed to the entry function */
	unsigned int started;	/* 1 once the thread has been entered */
};
#define INIT_THREAD { .sp = 0, .started = 1 }

extern unsigned long __get_wchan(struct task_struct *p);

#define ARCH_MIN_TASKALIGN	THREAD_SIZE

#include <asm/current.h>
#include <asm-generic/preempt.h>


/* task pt_regs live at the top of the kernel stack (never used on wasm) */
#define task_pt_regs(task) ((struct pt_regs *)NULL)
#define MAX_REG_OFFSET (sizeof(struct pt_regs))

#endif
