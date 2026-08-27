/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_WASM_PTRACE_H
#define _UAPI_ASM_WASM_PTRACE_H

#include <linux/compiler.h>
#include <linux/types.h>

/*
 * wasm32 has no hardware register state visible to userspace ptrace.
 * We expose a minimal synthetic layout so glibc-style debug code compiles.
 */
struct user_pt_regs {
	__u32 pc;
	__u32 sp;
	__u32 regs[16];
};

#endif /* _UAPI_ASM_WASM_PTRACE_H */
