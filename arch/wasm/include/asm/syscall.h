/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_SYSCALL_H
#define __ASM_WASM_SYSCALL_H

#include <linux/audit.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <asm/ptrace.h>

typedef long (*sys_call_ptr_t)(const struct pt_regs *);
extern const sys_call_ptr_t sys_call_table[];

static inline int syscall_get_nr(struct task_struct *task, struct pt_regs *regs)
{
	return regs->regs[0]; /* wasm: syscall number passed in reg 0 */
}

static inline void syscall_rollback(struct task_struct *task,
				    struct pt_regs *regs)
{
}

static inline long syscall_get_error(struct task_struct *task,
				     struct pt_regs *regs)
{
	return 0;
}

static inline long syscall_get_return_value(struct task_struct *task,
					    struct pt_regs *regs)
{
	return regs->regs[0];
}

static inline void syscall_set_return_value(struct task_struct *task,
					    struct pt_regs *regs,
					    int error, long val)
{
	regs->regs[0] = val;
}

static inline void syscall_get_arguments(struct task_struct *task,
					 struct pt_regs *regs,
					 unsigned long *args)
{
	memcpy(args, &regs->regs[1], 6 * sizeof(args[0]));
}

static inline int syscall_get_arch(struct task_struct *task)
{
	return 0; /* no audit arch for wasm */
}

#endif /* __ASM_WASM_SYSCALL_H */
