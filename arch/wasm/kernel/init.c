// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/init.c: machine-specific init hooks and reboot/poweroff.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <os-wasm.h>

void machine_restart(char *__unused) { wasm_exit(0); }
void machine_halt(void) { wasm_exit(0); }
void machine_power_off(void) { wasm_exit(0); }

void machine_crash_shutdown(struct pt_regs *regs)
{
	wasm_console_write("wasmux: crash\n", 15);
	wasm_exit(2);
}
