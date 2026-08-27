/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_EXTABLE_H
#define __ASM_WASM_EXTABLE_H

/*
 * wasm cannot recover from in-kernel faults; the exception table is a
 * compile-time empty stub.
 */
struct exception_table_entry {
	unsigned long insn;
	unsigned long fixup;
};

#endif
