/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_TLBFLUSH_H
#define __ASM_WASM_TLBFLUSH_H

#include <linux/sched.h>

static inline void local_flush_tlb_all(void) { }

struct mm_struct;
struct vm_area_struct;

static inline void flush_tlb_mm(struct mm_struct *mm) { }
static inline void flush_tlb_range(struct vm_area_struct *vma,
				   unsigned long start, unsigned long end) { }
static inline void flush_tlb_page(struct vm_area_struct *vma,
				  unsigned long addr) { }
static inline void flush_tlb_kernel_range(unsigned long start,
					  unsigned long end) { }

#endif
