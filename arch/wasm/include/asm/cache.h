/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_CACHE_H
#define __ASM_WASM_CACHE_H

#define L1_CACHE_SHIFT		5
#define L1_CACHE_BYTES		(1 << L1_CACHE_SHIFT)
#define SMP_CACHE_BYTES		L1_CACHE_BYTES
#define INTERNODE_CACHE_SHIFT	L1_CACHE_SHIFT
#define INTERNODE_CACHE_BYTES	L1_CACHE_BYTES

static inline int cache_line_size(void) { return L1_CACHE_BYTES; }

#endif
