// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/console.c: console + tty driver.
 *
 * Output goes straight out the wasm_console_write import.  Input comes
 * from the runtime via wasm_console_input() (exported, called by JS).
 */

#include <linux/console.h>
#include <linux/init.h>
#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <os-wasm.h>

/* console.write callback (name differs from the import to avoid a clash) */
static void wasm_con_write(struct console *co, const char *s, unsigned int n)
{
	wasm_console_write(s, n);
}

static struct tty_driver *wasm_console_device(struct console *co, int *index)
{
	*index = co->index;
	return NULL; /* no backing tty driver yet */
}

static int wasm_console_setup(struct console *co, char *options)
{
	return 0;
}

static struct console wasm_console = {
	.name = "wasm0",
	.write = wasm_con_write,
	.setup = wasm_console_setup,
	.flags = CON_PRINTBUFFER | CON_ANYTIME,
	.index = 0,
};

/* ---- early registration: called from setup_arch so the whole boot log
 * is visible on the console instead of waiting for device_initcalls. ---- */

void __init wasm_console_early_init(void)
{
	register_console(&wasm_console);
}

/* ---- input path: exported to the runtime ---- */

void wasm_console_input(const char *data, int len)
{
	/* TODO: wire to the tty layer once a tty driver is registered.
	 * For now drop input on the floor, the shell comes later. */
}

static int __init wasm_console_init(void)
{
	register_console(&wasm_console);
	return 0;
}
device_initcall(wasm_console_init);
