/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_SECCOMP_H
#define __ASM_WASM_SECCOMP_H

#include <linux/audit.h>

/* wasm has no native seccomp; the generic seccomp filter will be used.
 * Provide the required syscall numbers and structures. */
#define __NR_seccomp_read		__NR_read
#define __NR_seccomp_write		__NR_write
#define __NR_seccomp_exit		__NR_exit
#define __NR_seccomp_exit_group		__NR_exit_group
#define __NR_seccomp_sigreturn		0

#define SECCOMP_RetMask		SECCOMP_RET_ACTION_FULL
#define SECCOMP_DataMask	0xffffffff

static inline int arch_seccomp_sigreturn(void) { return 0; }

#endif
