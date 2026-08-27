/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_SEGMENT_H
#define __ASM_WASM_SEGMENT_H

#define KERNEL_DS	MAKE_MM_SEG(0)
#define USER_DS		MAKE_MM_SEG(-1UL)
#define get_ds()	(KERNEL_DS)
#define get_fs()	(USER_DS)

#define MAKE_MM_SEG(x) ((mm_segment_t){ .seg = (x) })
#define segment_eq(a, b) ((a).seg == (b).seg)

#endif
