// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/mem.c: physical memory layout for the flat wasm heap.
 * Follows the UML pattern: the whole linear memory is RAM, the kernel image
 * sits at the bottom, paging_init sets up zones.
 */

#include <linux/stddef.h>
#include <linux/module.h>
#include <linux/memblock.h>
#include <linux/swap.h>
#include <linux/pgtable.h>
#include <linux/printk.h>
#include <asm/pgalloc.h>
#include <linux/mm.h>
#include <linux/init.h>
#include <asm/sections.h>
#include <asm/page.h>

#define WASM_MEM_SIZE	(64UL << 20)	/* 64 MiB (keep in sync with setup.c/runtime) */

extern void __init memblock_free_all(void);

/* init_mm's page-table root; never actually walked on wasm (see pgtable.h) */
pgd_t swapper_pg_dir[PTRS_PER_PGD] __aligned(PAGE_SIZE);

/* the shared zero page, as every arch must provide */
unsigned long *empty_zero_page;
EXPORT_SYMBOL(empty_zero_page);

/* Memory protection map, as arch/um does (6.19 keeps it per-arch). */
static const pgprot_t protection_map[16] = {
	[VM_NONE]					= PAGE_NONE,
	[VM_READ]					= PAGE_READONLY,
	[VM_WRITE]					= PAGE_COPY,
	[VM_WRITE | VM_READ]				= PAGE_COPY,
	[VM_EXEC]					= PAGE_READONLY,
	[VM_EXEC | VM_READ]				= PAGE_READONLY,
	[VM_EXEC | VM_WRITE]				= PAGE_COPY,
	[VM_EXEC | VM_WRITE | VM_READ]			= PAGE_COPY,
	[VM_SHARED]					= PAGE_NONE,
	[VM_SHARED | VM_READ]				= PAGE_READONLY,
	[VM_SHARED | VM_WRITE]				= PAGE_SHARED,
	[VM_SHARED | VM_WRITE | VM_READ]		= PAGE_SHARED,
	[VM_SHARED | VM_EXEC]				= PAGE_READONLY,
	[VM_SHARED | VM_EXEC | VM_READ]			= PAGE_READONLY,
	[VM_SHARED | VM_EXEC | VM_WRITE]		= PAGE_SHARED,
	[VM_SHARED | VM_EXEC | VM_WRITE | VM_READ]	= PAGE_SHARED
};
DECLARE_VM_GET_PAGE_PROT

void __init mem_init(void)
{
	max_mapnr = max_low_pfn;
	high_memory = (void *)__va(max_low_pfn << PAGE_SHIFT);
	memblock_free_all();
}

void __init paging_init(void)
{
	unsigned long max_zone_pfn[MAX_NR_ZONES] = { 0 };

	empty_zero_page = (unsigned long *)memblock_alloc_low(PAGE_SIZE,
							     PAGE_SIZE);
	if (!empty_zero_page)
		panic("%s: Failed to allocate %lu bytes align=%lx\n",
		      __func__, PAGE_SIZE, PAGE_SIZE);

	/* the flat wasm heap is one 64 MiB RAM region (see setup.c) */
	min_low_pfn = 0;
	max_low_pfn = WASM_MEM_SIZE >> PAGE_SHIFT;

	max_zone_pfn[ZONE_NORMAL] = max_low_pfn;
	free_area_init(max_zone_pfn);
}

void free_initmem(void)
{
}

int init_new_context(struct task_struct *task, struct mm_struct *mm)
{
	return 0;
}

void destroy_context(struct mm_struct *mm)
{
}


pgd_t *pgd_alloc(struct mm_struct *mm)
{
	return __pgd_alloc(mm, 0);
}

