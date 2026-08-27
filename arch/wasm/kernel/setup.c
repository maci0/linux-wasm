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
	KERN_INFO "wasmux: Linux 6.19 on freestanding wasm32\n";

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
