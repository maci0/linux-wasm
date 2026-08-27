/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_WASM_PARAM_H
#define __ASM_WASM_PARAM_H

#include <uapi/asm/param.h>

# define HZ		CONFIG_HZ
# define USER_HZ	100
# define __USER_HZ	100
# define CLOCKS_PER_SEC	(USER_HZ)

#endif
