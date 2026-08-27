/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_PERF_REGS_H
#define __ASM_WASM_PERF_REGS_H

struct pt_regs;
struct perf_regs;
struct task_struct;

static inline struct pt_regs *task_pt_regs(struct task_struct *tsk)
{
	return NULL;
}

#endif
