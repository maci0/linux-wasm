// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/shell.c: kernel-resident init shell.
 *
 * wasm32 cannot exec native binaries, so instead of running /init the
 * port runs this small shell.  It is host-driven: wasm_shell() blocks in
 * the wasm_shell_wait import (the runtime reads a line of console input
 * or, in the browser, unwinds control back to the host), and lines are
 * fed into wasm_shell_input() which stages them in the exported
 * shell_scratch buffer.  Echo and line editing are the runtime's job.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/utsname.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/sched/signal.h>
#include <os-wasm.h>

#define SHELL_LINE_MAX	128

static char shell_line[SHELL_LINE_MAX];
static unsigned int shell_line_len;

/* runtime staging area for input bytes (host writes, then calls input()) */
static char shell_scratch[64] __aligned(8);

static void shell_puts(const char *s)
{
	wasm_console_write(s, strlen(s));
}

static void shell_prompt(void)
{
	shell_puts("wasmux:~$ ");
}

char *wasm_shell_scratch(void)
{
	return shell_scratch;
}

static void cmd_help(void)
{
	shell_puts(
		"wasmux shell commands:\n"
		"  help               show this list\n"
		"  version            kernel version\n"
		"  free               memory usage (si_meminfo)\n"
		"  uptime             seconds since boot\n"
		"  tasks              list kernel threads\n"
		"  echo <text>        print text\n"
		"  clear              clear the terminal\n"
		"  reboot             exit back to the host\n"
	);
}

static void cmd_version(void)
{
	shell_puts(init_utsname()->release);
	shell_puts("\n");
	shell_puts("Linux wasmux ");
	shell_puts(init_utsname()->version);
	shell_puts("\n");
}

static void cmd_free(void)
{
	struct sysinfo si;
	char buf[160];

	si_meminfo(&si);
	snprintf(buf, sizeof(buf),
		 "       total      used      free     shared  buffers\n"
		 "Mem:  %6luK %6luK %6luK %6luK %6luK\n",
		 si.totalram << (PAGE_SHIFT - 10),
		 (si.totalram - si.freeram) << (PAGE_SHIFT - 10),
		 si.freeram << (PAGE_SHIFT - 10),
		 si.sharedram << (PAGE_SHIFT - 10),
		 si.bufferram << (PAGE_SHIFT - 10));
	shell_puts(buf);
}

static void cmd_uptime(void)
{
	char buf[64];

	snprintf(buf, sizeof(buf), "up %llu seconds\n",
		 wasm_time_ns() / 1000000000ULL);
	shell_puts(buf);
}

static void cmd_tasks(void)
{
	struct task_struct *p;
	char buf[80];

	rcu_read_lock();
	for_each_process(p) {
		snprintf(buf, sizeof(buf), "%5d %c %s\n", p->pid,
			 task_state_to_char(p), p->comm);
		shell_puts(buf);
	}
	rcu_read_unlock();
}

static void shell_handle_line(char *line)
{
	char *arg;

	if (*line == '\0')
		return;

	arg = strchr(line, ' ');
	if (arg) {
		*arg++ = '\0';
		while (*arg == ' ')
			arg++;
	}

	if (!strcmp(line, "help"))
		cmd_help();
	else if (!strcmp(line, "version"))
		cmd_version();
	else if (!strcmp(line, "free"))
		cmd_free();
	else if (!strcmp(line, "uptime"))
		cmd_uptime();
	else if (!strcmp(line, "tasks"))
		cmd_tasks();
	else if (!strcmp(line, "echo")) {
		shell_puts(arg ? arg : "");
		shell_puts("\n");
	}
	else if (!strcmp(line, "clear"))
		shell_puts("\x1b[2J\x1b[H");
	else if (!strcmp(line, "reboot") || !strcmp(line, "exit"))
		wasm_exit(0);
	else {
		shell_puts("sh: command not found: ");
		shell_puts(line);
		shell_puts("\n");
	}
}

void wasm_shell_input(const char *data, int len)
{
	/* Echo is the runtime's job (the host tty or the web terminal echoes
	 * as the user types); the kernel only buffers and reacts. */
	while (len-- > 0) {
		char c = *data++;

		if (c == '\n' || c == '\r') {
			shell_line[shell_line_len] = '\0';
			shell_handle_line(shell_line);
			shell_prompt();
			shell_line_len = 0;
		} else if (shell_line_len < SHELL_LINE_MAX - 1) {
			shell_line[shell_line_len++] = c;
		}
	}
}

/* called from kernel_init instead of exec'ing /init; never returns.
 * The shell loop is driven by the wasm_shell_wait import: the runtime
 * either blocks reading a line (wasmtime) or unwinds control back to
 * the host (browser), which then feeds lines via wasm_shell_input(). */
void __init wasm_shell(void)
{
	int n;

	shell_puts("\n");
	shell_puts("  Linux ");
	shell_puts(init_utsname()->release);
	shell_puts(" on wasm32 - kernel-resident init shell\n");
	shell_puts("  (no userspace: wasm32 cannot exec native binaries)\n");
	shell_puts("  type 'help' for available commands.\n");
	shell_prompt();

	for (;;) {
		n = wasm_shell_wait(shell_scratch, sizeof(shell_scratch));
		if (n <= 0) {
			/* EOF from the runtime: nothing left to do */
			wasm_exit(0);
		}
		wasm_shell_input(shell_scratch, n);
	}
}
