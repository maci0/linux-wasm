/* SPDX-License-Identifier: GPL-2.0 */
/*
 * arch/wasm/kernel/head.S: wasm has no real entry asm; the runtime calls
 * start_kernel directly (exported from the module).  This file exists so
 * head.o satisfies the build.
 */
	.text
	.globl	wasm_entry_stub
wasm_entry_stub:
	unreachable
