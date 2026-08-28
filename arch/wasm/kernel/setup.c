// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/setup.c: boot entry for the freestanding wasm port.
 *
 * Modeled on UML's um_arch.c.  The runtime hands us one flat 64 MiB linear
 * memory: the kernel image at [0, WASM_KERNEL_IMAGE_SIZE), free RAM above.
 */

#include <linux/init.h>
#include <linux/mm.h>
#include <linux/memblock.h>
#include <linux/console.h>
#include <linux/seq_file.h>
#include <linux/random.h>
#include <linux/string.h>
#include <generated/utsrelease.h>
#include <linux/sched.h>
#include <linux/sched/task.h>
#include <asm/page.h>
#include <asm/sections.h>
#include <asm/setup.h>
#include <asm/pgtable.h>
#include <os-wasm.h>

#define WASM_MEM_SIZE		(64UL << 20)	/* 64 MiB of linear memory */

/*
 * wasm-ld cannot lay out custom sections, so the section-marker symbols
 * (_end and friends, see markers.c) are not placed at real image
 * boundaries.  Reserve a generous fixed window for the kernel image
 * instead; the actual image is a few MiB.
 */
#define WASM_KERNEL_IMAGE_SIZE	(16UL << 20)

char __initdata command_line[COMMAND_LINE_SIZE] =
	"console=wasm0 lpj=1000000 panic=1";

static const char wasm_banner[] __initconst =
	KERN_INFO "wasmux: Linux " UTS_RELEASE " on freestanding wasm32\n";

void paging_init(void);
void wasm_console_early_init(void);

void __init wasm_mem_setup(void)
{
	memblock_add(0, WASM_MEM_SIZE);
	memblock_reserve(0, WASM_KERNEL_IMAGE_SIZE);
}

static int show_cpuinfo(struct seq_file *m, void *v)
{
	seq_puts(m, "processor\t: Wasm Virtual Machine v1\n");
	seq_puts(m, "vendor_id\t: wasmux\n");
	return 0;
}

const struct seq_operations cpuinfo_op = {
	.start	= NULL,
	.next	= NULL,
	.stop	= NULL,
	.show	= show_cpuinfo,
};

void __init setup_arch(char **cmdline_p)
{
	u8 rng_seed[32];

	/* fork_init() sizes the task_struct cache from this (the port selects
	 * CONFIG_ARCH_WANTS_DYNAMIC_TASK_STRUCT like UML); without an explicit
	 * value the cache is created with a zero object size. */
	arch_task_struct_size = sizeof(struct task_struct);

	/* Print the whole boot log as it happens, not at device_initcall */
	wasm_console_early_init();

	printk("%s", wasm_banner);
	wasm_mem_setup();
	paging_init();

	strscpy(boot_command_line, command_line, COMMAND_LINE_SIZE);
	*cmdline_p = command_line;

	wasm_random(rng_seed, sizeof(rng_seed));
	add_bootloader_randomness(rng_seed, sizeof(rng_seed));
	memzero_explicit(rng_seed, sizeof(rng_seed));
}

void __init arch_cpu_finalize_init(void)
{
}

#ifdef CONFIG_WASM
/*
 * Node data for the freestanding port.  Defined here as initialized data
 * in an early-compiled arch/wasm object: wasm-ld assigns zero-initialized
 * statics addresses that collide with the module's data segments (the
 * generic contig_page_data in .ref.data overlaps the boot-parameter
 * strings), which corrupts the page allocator.  The "wasm" zone name is
 * a placeholder that free_area_init() overwrites.
 */
struct pglist_data wasm_node_data = {
	.node_zones[0].name = "wasm",
};
#endif

#ifdef CONFIG_WASM
/*
 * The memblock region arrays (see memblock.c) must live away from the
 * memblock struct: wasm-ld placed them at the same address inside
 * .init.data.  The non-zero initializer keeps them in the data segment
 * (early area); the contents are filled in by memblock_add().
 */
struct memblock_region memblock_memory_init_regions[128] = { { 0, 0, 1 } };
struct memblock_region memblock_reserved_init_regions[128] = { { 0, 0, 1 } };

/*
 * The memblock struct itself has the same problem as the arrays: wasm-ld
 * splits its symbol address from its initializer content inside the late
 * .init.data segments, so no post-link remap can fix it.  Define it here
 * as early data instead (see mm/memblock.c).
 */
struct memblock memblock = {
	.memory.regions		= memblock_memory_init_regions,
	.memory.max		= 128, /* INIT_MEMBLOCK_REGIONS */
	.memory.name		= "memory",

	.reserved.regions	= memblock_reserved_init_regions,
	.reserved.max		= 128, /* INIT_MEMBLOCK_REGIONS */
	.reserved.name		= "reserved",

	.bottom_up		= false,
	.current_limit		= MEMBLOCK_ALLOC_ANYWHERE,
};

/*
 * The SLUB boot caches (kmem_cache, kmem_cache_node).  wasm-ld splits
 * these symbols from their initializer content, and the kernel's
 * references end up pointing at two different copies, which desyncs the
 * slab accounting.  Early data gives them one stable address; see
 * mm/slub.c (kmem_cache_init).
 */
#include "../../../mm/slab.h"
struct kmem_cache boot_kmem_cache = {
	.name = "boot_kmem_cache",
};
struct kmem_cache boot_kmem_cache_node = {
	.name = "boot_kmem_cache_node",
};
#endif
