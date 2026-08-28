#define COMPILE_OFFSETS
// SPDX-License-Identifier: GPL-2.0
#include <linux/kbuild.h>
#include <linux/sched.h>
#include <asm/thread_info.h>

static void __used common(void)
{
	OFFSET(TI_FLAGS, thread_info, flags);
	OFFSET(TI_PREEMPT, thread_info, preempt_count);
	BLANK();

	OFFSET(TASK_TI, task_struct, thread_info);
	DEFINE(THREAD_SIZE, THREAD_SIZE);
}
