// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/lib/delay.c: busy-wait delays.
 *
 * The wasm VM has no independent time reference inside a busy loop, so the
 * loops are calibrated against loops_per_jiffy (preset via the lpj=
 * command line on wasm).  The memory clobber keeps the loop from being
 * optimized away.
 */

#include <linux/delay.h>
#include <linux/timex.h>
#include <linux/export.h>

void __delay(unsigned long loops)
{
	while (loops--)
		asm volatile("" ::: "memory");
}
EXPORT_SYMBOL(__delay);

inline void __const_udelay(unsigned long xloops)
{
	__delay((unsigned long)(((unsigned long long)xloops * HZ *
				 loops_per_jiffy) >> 32));
}
EXPORT_SYMBOL(__const_udelay);

void __udelay(unsigned long usecs)
{
	__delay((unsigned long)(((unsigned long long)usecs * HZ *
				 loops_per_jiffy) / USEC_PER_SEC));
}
EXPORT_SYMBOL(__udelay);

void __ndelay(unsigned long nsecs)
{
	__delay((unsigned long)(((unsigned long long)nsecs * HZ *
				 loops_per_jiffy) / NSEC_PER_SEC));
}
EXPORT_SYMBOL(__ndelay);
