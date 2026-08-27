/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_PGALLOC_H
#define __ASM_WASM_PGALLOC_H

#include <linux/mm.h>
#include <asm-generic/pgalloc.h>

#define pmd_populate_kernel(mm, pmd, pte) \
	set_pmd(pmd, __pmd(_PAGE_TABLE + (unsigned long) __pa(pte)))

#define pmd_populate(mm, pmd, page) \
	set_pmd(pmd, __pmd(_PAGE_TABLE + (unsigned long) __pa(page_address(page))))

#define pud_populate(mm, pud, pmd) \
	set_pud(pud, __pud(_PAGE_TABLE + (unsigned long) __pa(pmd)))

#define p4d_populate(mm, p4d, pud) \
	set_p4d(p4d, __p4d(_PAGE_TABLE + (unsigned long) __pa(pud)))

extern pgd_t *pgd_alloc(struct mm_struct *mm);

#endif /* __ASM_WASM_PGALLOC_H */
