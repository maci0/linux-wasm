/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_VM_FLAGS_H
#define __ASM_WASM_VM_FLAGS_H

/* wasm linear memory is always RW; protection flags are nominal */
#define VM_DATA_DEFAULT_FLAGS	(VM_READ | VM_WRITE | VM_MAYREAD | VM_MAYWRITE)

#endif
