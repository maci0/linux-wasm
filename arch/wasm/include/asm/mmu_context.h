/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_MMU_CONTEXT_H
#define __ASM_WASM_MMU_CONTEXT_H

#include <linux/sched.h>
#include <linux/mm_types.h>

#include <asm/mm_hooks.h>
#include <asm/mmu.h>

/* single-address-space wasm: switching mm is a no-op */
static inline void switch_mm(struct mm_struct *prev, struct mm_struct *next,
			     struct task_struct *tsk)
{
}

#define init_new_context init_new_context
extern int init_new_context(struct task_struct *task, struct mm_struct *mm);

#define destroy_context destroy_context
extern void destroy_context(struct mm_struct *mm);

#include <asm-generic/mmu_context.h>

#endif
