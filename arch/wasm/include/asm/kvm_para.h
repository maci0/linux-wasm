/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_KVM_PARA_H
#define __ASM_WASM_KVM_PARA_H

#include <uapi/asm/kvm_para.h>

static inline bool kvm_check_and_init_guest_setup(void) { return false; }
static inline unsigned int kvm_arch_para_features(void) { return 0; }
static inline unsigned int kvm_arch_para_hints(void) { return 0; }
static inline bool kvm_para_available(void) { return false; }

#endif
