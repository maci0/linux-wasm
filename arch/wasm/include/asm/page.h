/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_PAGE_H
#define __ASM_WASM_PAGE_H

#include <linux/const.h>
#include <vdso/page.h>

#ifndef __ASSEMBLER__

struct page;
#include <linux/pfn.h>
#include <linux/types.h>
#include <asm/vm-flags.h>

#define clear_page(page)	memset((void *)(page), 0, PAGE_SIZE)
#define copy_page(to,from)	memcpy((void *)(to), (void *)(from), PAGE_SIZE)
#define clear_user_page(page, vaddr, pg)	clear_page(page)
#define copy_user_page(to, from, vaddr, pg)	copy_page(to, from)

typedef struct { unsigned long pte; } pte_t;
typedef struct { unsigned long pgd; } pgd_t;

#define pte_val(x)	((x).pte)
#define pgd_val(x)	((x).pgd)

/* UML-style pte flag helpers used by asm/pgtable.h */
#define pte_get_bits(p, bits)	((p).pte & (bits))
#define pte_set_bits(p, bits)	((p).pte |= (bits))
#define pte_clear_bits(p, bits)	((p).pte &= ~(bits))
#define pte_copy(to, from)	((to).pte = (from).pte)
#define pte_is_zero(p)		(!((p).pte & ~_PAGE_PROTNONE))
#define pte_set_val(p, phys, prot) (p).pte = ((phys) | pgprot_val(prot))

typedef struct { unsigned long pgprot; } pgprot_t;
#define pgprot_val(x)	((x).pgprot)
#define __pgprot(x)	((pgprot_t) { (x) })

#define __pte(x) ((pte_t) { (x) })
#define __pgd(x) ((pgd_t) { (x) })
#define __pmd(x) ((pmd_t) { (x) })
#define __pud(x) ((pud_t) { (x) })

typedef unsigned long phys_t;
typedef struct page *pgtable_t;

#define PAGE_OFFSET	0UL
#define KERNELBASE	PAGE_OFFSET

/* flat linear memory: phys == virt */
#define __pa(virt)	((unsigned long)(virt))
#define __va(phys)	((void *)(unsigned long)(phys))
#define virt_addr_valid(v) pfn_valid(virt_to_pfn(v))

#define phys_to_pfn(p) ((p) >> PAGE_SHIFT)
#define pfn_to_phys(pfn) PFN_PHYS(pfn)

static inline unsigned long virt_to_pfn(const void *p)
{
	return __pa(p) >> PAGE_SHIFT;
}
#define virt_to_page(kaddr)	pfn_to_page(virt_to_pfn(kaddr))

#include <asm-generic/getorder.h>
#include <asm-generic/memory_model.h>

#endif /* __ASSEMBLER__ */

#endif /* __ASM_WASM_PAGE_H */
