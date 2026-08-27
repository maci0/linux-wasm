/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __IRQ_USER_H__
#define __IRQ_USER_H__

#include <linux/threads.h>

struct pt_regs;

enum { IRQ_READ, IRQ_WRITE };

extern void sigio_handler(int irq, struct pt_regs *regs);

#endif
