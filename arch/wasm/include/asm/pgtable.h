/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_PGTABLE_H
#define __ASM_WASM_PGTABLE_H

/*
 * wasm32 has no MMU; page tables exist only so the generic vm code
 * compiles.  Degenerate 2-level tables; leaf entries map to themselves.
 */

#include <asm/page.h>
#include <linux/mm_types.h>

#define _PAGE_PRESENT	0x001
#define _PAGE_NEEDSYNC	0x000 /* unused on wasm */
#define _PAGE_RW	0x020
#define _PAGE_USER	0x040
#define _PAGE_ACCESSED	0x080
#define _PAGE_DIRTY	0x100
#define _PAGE_PROTNONE	0x010
#define _PAGE_SWP_EXCLUSIVE 0x400

#include <asm/pgtable-2level.h>

extern pgd_t swapper_pg_dir[PTRS_PER_PGD];

/*
 * The wasm linear memory is 128 MiB total (see arch/wasm/Makefile and the
 * browser runtime).  The kernel manages the low 64 MiB as RAM
 * (WASM_MEM_SIZE in setup.c); vmalloc and modules live in the top half so
 * their virtual addresses stay inside the linear memory.
 */
#define VMALLOC_OFFSET	(8 * 1024 * 1024)
#define VMALLOC_START	((((unsigned long)high_memory) + VMALLOC_OFFSET) & ~(VMALLOC_OFFSET - 1))
#define VMALLOC_END	((128UL << 20) - 2 * PAGE_SIZE)
#define MODULES_VADDR	VMALLOC_START
#define MODULES_END	VMALLOC_END

#define TASK_UNMAPPED_BASE	(TASK_SIZE / 3)

#define _PAGE_TABLE	(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER | _PAGE_ACCESSED | _PAGE_DIRTY)
#define _KERNPG_TABLE	(_PAGE_PRESENT | _PAGE_RW | _PAGE_ACCESSED | _PAGE_DIRTY)
#define _PAGE_CHG_MASK	(PAGE_MASK | _PAGE_ACCESSED | _PAGE_DIRTY)

#define PAGE_NONE	__pgprot(_PAGE_PROTNONE | _PAGE_ACCESSED)
#define PAGE_SHARED	__pgprot(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER | _PAGE_ACCESSED)
#define PAGE_COPY	__pgprot(_PAGE_PRESENT | _PAGE_USER | _PAGE_ACCESSED)
#define PAGE_READONLY	__pgprot(_PAGE_PRESENT | _PAGE_USER | _PAGE_ACCESSED)
#define PAGE_KERNEL	__pgprot(_PAGE_PRESENT | _PAGE_RW | _PAGE_ACCESSED | _PAGE_DIRTY)

static inline pte_t pte_mkuptodate(pte_t pte) { return pte; }

#define __virt_to_page(virt)	phys_to_page(__pa(virt))
#define virt_to_page(addr)	__virt_to_page((const unsigned long)(addr))
#define virt_to_pfn(v)		virt_to_phys(v)
#define virt_to_phys(v)		__pa(v)
#define page_to_virt(page)	__va(page_to_phys(page))

static inline void set_pte(pte_t *pteptr, pte_t pteval)
{
	pte_copy(*pteptr, pteval);
}

#define PFN_PTE_SHIFT		PAGE_SHIFT

#define set_ptes set_ptes
static inline void set_ptes(struct mm_struct *mm, unsigned long addr,
			    pte_t *ptep, pte_t pte, int nr)
{
	for (;;) {
		set_pte(ptep, pte);
		if (--nr == 0)
			break;
		ptep++;
		pte = __pte(pte_val(pte) + (1UL << PFN_PTE_SHIFT));
	}
}

#define __HAVE_ARCH_PTE_SAME
static inline int pte_same(pte_t pte_a, pte_t pte_b)
{
	return !!(pte_val(pte_a) == pte_val(pte_b));
}

#define pmd_page_vaddr(pmd) ((unsigned long) __va(pmd_val(pmd) & PAGE_MASK))
#define pte_pfn(x) phys_to_pfn(pte_val(x))
#define pte_page(x) pfn_to_page(pte_pfn(x))
#define pmd_pfn(pmd) (pmd_val(pmd) >> PAGE_SHIFT)
#define pmd_page(pmd) phys_to_page(pmd_val(pmd) & PAGE_MASK)
#define pfn_pmd(pfn, prot) __pmd(pfn_to_phys(pfn) | pgprot_val(prot))

#define update_mmu_cache(vma, address, ptep) do { } while (0)
#define update_mmu_cache_range(vmf, vma, address, ptep, nr) do { } while (0)

static inline int pte_none(pte_t pte) { return pte_is_zero(pte); }
static inline int pmd_none(pmd_t pmd) { return pmd_val(pmd) == 0; }
static inline int pmd_present(pmd_t pmd) { return pmd_val(pmd) & _PAGE_PRESENT; }
static inline void pmd_clear(pmd_t *pmdp) { pmd_val(*pmdp) = 0; }
static inline int pte_present(pte_t x) { return pte_get_bits(x, (_PAGE_PRESENT | _PAGE_PROTNONE)); }
static inline int pte_read(pte_t pte) { return pte_get_bits(pte, _PAGE_USER); }
static inline int pte_exec(pte_t pte) { return pte_get_bits(pte, _PAGE_USER); }
static inline int pte_write(pte_t pte) { return pte_get_bits(pte, _PAGE_RW); }
static inline int pte_dirty(pte_t pte) { return pte_get_bits(pte, _PAGE_DIRTY); }
static inline int pte_young(pte_t pte) { return pte_get_bits(pte, _PAGE_ACCESSED); }

static inline pte_t pte_mkclean(pte_t pte) { pte_clear_bits(pte, _PAGE_DIRTY); return pte; }
static inline pte_t pte_mkold(pte_t pte) { pte_clear_bits(pte, _PAGE_ACCESSED); return pte; }
static inline pte_t pte_wrprotect(pte_t pte) { pte_clear_bits(pte, _PAGE_RW); return pte; }
static inline pte_t pte_mkread(pte_t pte) { pte_set_bits(pte, _PAGE_USER); return pte; }
static inline pte_t pte_mkwrite_novma(pte_t pte) { pte_set_bits(pte, _PAGE_RW); return pte; }
#define pte_mkwrite(pte, vma)	pte_mkwrite_novma(pte)
static inline pte_t pte_mkdirty(pte_t pte) { pte_set_bits(pte, _PAGE_DIRTY); return pte; }
static inline pte_t pte_mkyoung(pte_t pte) { pte_set_bits(pte, _PAGE_ACCESSED); return pte; }

static inline void pte_clear(struct mm_struct *mm, unsigned long addr, pte_t *ptep)
{
	set_pte(ptep, __pte(0));
}


static inline pte_t pfn_pte(unsigned long pfn, pgprot_t pgprot)
{
	return __pte(pfn_to_phys(pfn) | pgprot_val(pgprot));
}

static inline pte_t pte_modify(pte_t pte, pgprot_t newprot)
{
	pte_set_val(pte, pte_val(pte) & _PAGE_CHG_MASK, newprot);
	return pte;
}

#define __HAVE_ARCH_PTE_CLEAR
static inline int pmd_bad(pmd_t pmd) { return 0; }

/* swap PTE encoding (software-only; layout mirrors UML 2-level) */
#define __swp_type(x)			(((x).val >> 5) & 0x1f)
#define __swp_offset(x)			((x).val >> 11)
#define __swp_entry(type, offset) \
	((swp_entry_t) { ((type) << 5) | ((offset) << 11) })
#define __pte_to_swp_entry(pte)		((swp_entry_t) { pte_val(pte) >> 1 })
#define __swp_entry_to_pte(x)		((pte_t) { (x).val << 1 })

static inline bool pte_swp_exclusive(pte_t pte)
{
	return !!(pte_val(pte) & _PAGE_SWP_EXCLUSIVE);
}
static inline pte_t pte_swp_mkexclusive(pte_t pte)
{
	pte_set_bits(pte, _PAGE_SWP_EXCLUSIVE);
	return pte;
}
static inline pte_t pte_swp_clear_exclusive(pte_t pte)
{
	pte_clear_bits(pte, _PAGE_SWP_EXCLUSIVE);
	return pte;
}

/* page-table frees are no-ops (no real TLB) */
static inline void __pte_free_tlb(struct mmu_gather *tlb, pgtable_t pte,
				  unsigned long address) { }
static inline void __pmd_free_tlb(struct mmu_gather *tlb, pmd_t *pmd,
				  unsigned long address) { }
static inline void __pud_free_tlb(struct mmu_gather *tlb, pud_t *pud,
				  unsigned long address) { }

#define KSTK_EIP(tsk)	(0UL)
#define KSTK_ESP(tsk)	(0UL)

#endif /* __ASM_WASM_PGTABLE_H */
