// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/process.c: single-threaded cooperative scheduling.
 *
 * wasm32 has no signals, no fork, no ptrace, and the wasm call stack
 * cannot be captured or restored, so real context switches are
 * impossible.  The boot "threads" therefore run cooperatively: the first
 * time the scheduler picks a task, __switch_to() starts it as a nested
 * function call.  A kernel thread body is a for (;;) loop or ends in
 * panic(), so the nested call never returns; any later switch is the
 * identity and the current execution simply continues.
 */

#include <linux/sched.h>
#include <linux/sched/task.h>
#include <linux/kallsyms.h>
#include <linux/completion.h>
#include <linux/elfcore.h>
#include <linux/pgtable.h>
#include <asm/processor.h>
#include <asm/current.h>
#include <os-wasm.h>
#include <os.h>

struct task_struct *wasm_current_task = &init_task;

int copy_thread(struct task_struct *p, const struct kernel_clone_args *args)
{
	/* kernel threads only: record the entry point for __switch_to() */
	p->thread.entry = (unsigned long)args->fn;
	p->thread.arg = (unsigned long)args->fn_arg;
	p->thread.started = 0;
	return 0;
}

struct task_struct *__switch_to(struct task_struct *prev,
				struct task_struct *next)
{
	wasm_current_task = next;
	if (!next->thread.started) {
		next->thread.started = 1;
		((int (*)(void *))next->thread.entry)((void *)next->thread.arg);
	}
	return prev;
}

unsigned long __get_wchan(struct task_struct *p)
{
	return 0;
}

void show_stack(struct task_struct *task, unsigned long *sp,
		const char *loglvl)
{
	printk("%s<%s> no stack dump on wasm\n", loglvl, current->comm);
}

int dump_fpu(void) { return 0; }

void start_thread(struct pt_regs *regs, unsigned long eip,
		  unsigned long esp) { }

void release_thread(struct task_struct *task) { }

/* ---- per-arch hooks required by the generic core ---- */

void flush_thread(void)
{
}

void show_regs(struct pt_regs *regs)
{
	pr_err("wasm: no register dump (wasm32 has no arch registers)\n");
}

int elf_core_copy_task_fpregs(struct task_struct *t, elf_fpregset_t *fpu)
{
	return 0;
}

/* The init task's kernel stack.  Never actually used as a stack on wasm
 * (execution lives on the wasm call stack), but init_task.stack must point
 * at valid memory. */
unsigned long init_stack[THREAD_SIZE / sizeof(unsigned long)]
	__aligned(THREAD_SIZE);

/* idle: yield to the host event loop */
void arch_cpu_idle(void)
{
	os_idle_sleep(1000000ULL); /* ~1ms of host time */
}
