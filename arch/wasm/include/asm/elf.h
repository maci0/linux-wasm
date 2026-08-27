/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_ELF_H
#define __ASM_WASM_ELF_H

#include <linux/utsname.h>
#include <uapi/linux/elf.h>
#include <uapi/asm/ptrace.h>

/*
 * wasm32 cannot load ELF userspace; the definitions exist so the kernel
 * core (binfmt hooks, core dumps) compiles.
 */
#define ELF_CLASS	ELFCLASS32
#define ELF_DATA	ELFDATA2LSB
#define ELF_ARCH	EM_NONE

#define ELF_EXEC_PAGESIZE	PAGE_SIZE

#define elf_check_arch(x)	(1) /* accept anything; wasm has no real ELF userspace */

struct task_struct;
struct pt_regs;
extern void start_thread(struct pt_regs *regs, unsigned long eip,
			 unsigned long esp);

#define ELF_PLATFORM		NULL
#define ELF_HWCAP		0
#define ELF_HWCAP2		0
#define COMPAT_ELF_PLATFORM	NULL

/* flat memory, no user/kernel split: everything loads at a fixed base */
#define STACK_TOP		(TASK_SIZE - 16 * 1024 * 1024)
#define STACK_TOP_MAX		STACK_TOP
#define ELF_ET_DYN_BASE		(STACK_TOP / 3 * 2)
#define ELF_MIN_ALIGN		PAGE_SIZE

typedef struct user_pt_regs elf_greg_t;
#define ELF_NGREG (sizeof(struct user_pt_regs) / sizeof(elf_greg_t))
typedef elf_greg_t elf_gregset_t[ELF_NGREG];
typedef struct { } elf_fpregset_t;
typedef struct { } elf_prstatus;
typedef struct { } elf_prpsinfo;

#define R_386_NONE	0
#define ELF_PLAT_INIT(_r, load_addr)	do { } while (0)

#endif
