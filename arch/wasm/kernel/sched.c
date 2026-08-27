// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/sched.c: scheduler-class table.
 *
 * wasm-ld cannot lay out the __*_sched_class sections in priority order
 * (it ignores linker-script placement), so the classes are iterated
 * through this explicit table instead of a contiguous array; see
 * kernel/sched/sched.h.
 */

#include <linux/sched.h>

extern const struct sched_class stop_sched_class;
extern const struct sched_class dl_sched_class;
extern const struct sched_class rt_sched_class;
extern const struct sched_class fair_sched_class;
extern const struct sched_class idle_sched_class;

const struct sched_class *wasm_sched_classes[] = {
	&stop_sched_class,
	&dl_sched_class,
	&rt_sched_class,
	&fair_sched_class,
	&idle_sched_class,
};
const unsigned int wasm_sched_class_count = ARRAY_SIZE(wasm_sched_classes);

struct sched_class *__sched_class_highest = (struct sched_class *)&stop_sched_class;
struct sched_class *__sched_class_lowest = (struct sched_class *)&idle_sched_class;
