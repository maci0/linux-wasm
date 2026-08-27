/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_CURRENT_H
#define __ASM_WASM_CURRENT_H

#include <linux/compiler.h>

struct task_struct;
extern struct task_struct init_task;

/* wasm32 is UP: the only running task is init_task (see THREAD_INFO_IN_TASK:
 * linux/thread_info.h derives current_thread_info() from `current`). */
static __always_inline struct task_struct *get_current(void)
{
	return &init_task;
}

#define current get_current()

/* CONFIG_THREAD_INFO_IN_TASK: thread_info lives inside task_struct */
struct thread_info;
#define current_thread_info() (&get_current()->thread_info)

#endif
