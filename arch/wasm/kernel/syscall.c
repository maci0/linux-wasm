// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/syscall.c: empty syscall table.
 * Userspace syscalls come later (initramfs + a wasm-side libc); for now
 * the table is all-default so the kernel links.
 */
#include <linux/syscalls.h>
#include <linux/linkage.h>
#include <asm/syscall.h>

long sys_ni_syscall(void);

#define SYSCALL(ni) ((sys_call_ptr_t)sys_ni_syscall)

/* 64-slot empty table; real syscalls come with the wasm libc work */
const sys_call_ptr_t sys_call_table[64] = {
	[0 ... 63] = (sys_call_ptr_t)sys_ni_syscall,
};
