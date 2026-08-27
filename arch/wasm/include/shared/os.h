/* SPDX-License-Identifier: GPL-2.0 */
/*
 * os.h for wasm: a minimal subset of the UML os.h interface.
 * The full prototypes live in os-wasm.h; this header exists so that
 * kernel code that #includes <os.h> compiles.
 */
#ifndef __OS_H__
#define __OS_H__

#include <linux/types.h>
#include <os-wasm.h>

struct openflags {
	unsigned int r:1, w:1, flags;
};

#define OPENFLAGS_READ	((struct openflags){ .r = 1 })
#define of_read(x)	((x).r)
struct uml_stat {
	unsigned long long ust_size;
	unsigned int ust_mode;
	unsigned long long ust_ino;
	unsigned int ust_dev;
	unsigned int ust_nlink;
	unsigned int ust_uid, ust_gid;
	unsigned long long ust_atime, ust_mtime, ust_ctime;
	unsigned int ust_blksize;
	long long ust_blocks;
};

enum os_type { OS_TYPE_FILE, OS_TYPE_DIR, OS_TYPE_SYMLINK, OS_TYPE_CHARDEV,
	       OS_TYPE_BLOCKDEV, OS_TYPE_FIFO, OS_TYPE_SOCKET };
#endif /* __OS_H__ */
