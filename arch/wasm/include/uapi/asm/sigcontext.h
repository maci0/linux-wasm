/* SPDX-License-Identifier: GPL-2.0 WITH Linux-syscall-note */
#ifndef _UAPI_ASM_WASM_SIGCONTEXT_H
#define _UAPI_ASM_WASM_SIGCONTEXT_H

#include <linux/types.h>

struct sigcontext {
	__u32 regs[16];
	__u32 pc;
	__u32 sp;
};

#endif /* _UAPI_ASM_WASM_SIGCONTEXT_H */
