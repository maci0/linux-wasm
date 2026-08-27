/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_PTRACE_H
#define __ASM_WASM_PTRACE_H

#include <uapi/asm/ptrace.h>

struct pt_regs {
	u32 pc;
	u32 sp;
	u32 regs[16];
};

#define PTRACE_GETREGS		12
#define PTRACE_SETREGS		13
#define PTRACE_GETFPREGS	14
#define PTRACE_SETFPREGS	15

#define instruction_pointer(regs) ((regs)->pc)
#define user_stack_pointer(regs)  ((regs)->sp)
#define regs_return_value(regs)   ((regs)->regs[0])
#define user_mode(regs)		  (0)
#define profile_pc(regs)	  instruction_pointer(regs)

#endif
