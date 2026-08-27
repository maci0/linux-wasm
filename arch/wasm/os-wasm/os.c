// SPDX-License-Identifier: GPL-2.0
/*
 * os-wasm implementation of the os_* primitive layer.
 *
 * The kernel proper never calls these directly; it goes through
 * arch/wasm/kernel glue, same as UML going through os-Linux.  Here every
 * file-descriptor concept maps to a small in-memory table backed by the
 * JS runtime; process/ptrace concepts are single-threaded no-ops.
 */

#include <os.h>
#include <os-wasm.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/errno.h>

/* ---- console / stdout ---------------------------------------------------- */

void os_flush_stdout(void)
{
	/* wasm_console_write is unbuffered */
}

/* ---- time ----------------------------------------------------------------- */

long long os_nsecs(void)
{
	return (long long)wasm_time_ns();
}

void os_persistent_clock_emulation(long long *nsecs)
{
	*nsecs = (long long)wasm_time_ns();
}

/* ---- timers ---------------------------------------------------------------
 * The runtime arms a real timer and raises the timer "irq" when it fires. */

int os_timer_create(clockid_t unused)
{
	return 0;
}

int os_timer_set_interval(int tid, void *sig, unsigned long long ms,
			  long long *interval_p)
{
	if (interval_p)
		*interval_p = (long long)ms * 1000000ULL;
	return 0;
}

int os_timer_one_shot(int tid, unsigned long long ns)
{
	wasm_timer_arm(ns);
	return 0;
}

int os_timer_disable(int tid)
{
	return 0;
}

/* ---- randomness ------------------------------------------------------------ */

void os_get_random(unsigned char *buf, int len)
{
	wasm_random(buf, len);
}

/* ---- memory management ------------------------------------------------------
 * The wasm linear memory is one flat region handed to us at boot; there is
 * nothing to mmap.  map/unmap/protect are no-ops that report success so the
 * generic vm code proceeds. */

int os_map_memory(void *virt, int fd, unsigned long long off,
		  unsigned long long len, int r, int w, int x)
{
	return 0;
}

int os_unmap_memory(void *addr, int len) { return 0; }
int os_protect_memory(void *addr, unsigned long long len, int r, int w, int x) { return 0; }
int os_drop_memory(void *addr, unsigned long long length) { return 0; }
int os_mremap_rw_shared(void *from_addr, unsigned long from_len, void *to_addr,
			unsigned long to_len) { return 0; }
int os_mmap_rw_shared(int fd, void *addr, unsigned long len) { return 0; }

/* ---- processes / signals ----------------------------------------------------
 * Single-threaded wasm: no fork, no ptrace, no signals. */

int os_getpid(void) { return 1; }

int os_kill_process(int pid, int sig) { return -EINVAL; }

int os_kill_ptraced_process(int pid, int reap) { return -EINVAL; }

int os_reap_child(int pid) { return -EINVAL; }

int os_alarm_process(int pid) { return -EINVAL; }

int os_start_cpu_thread(int thread, int cpu_id) { return -EINVAL; }

void os_start_secondary(void *arg) { }

int os_init_smp(void) { return 0; }

int os_send_ipi(int cpu, int signal) { return -EINVAL; }

void os_local_ipi_disable(void) { }
void os_local_ipi_enable(void) { }

int os_futex_wait(u32 *uaddr, u32 val) { return -ENOSYS; }
int os_futex_wake(u32 *uaddr) { return -ENOSYS; }

int os_helper_thread(int fd) { return -ENOSYS; }
void os_run_helper_thread(void) { }
int os_kill_helper_thread(void) { return 0; }
int os_set_pdeathsig(const char *pid_str) { return 0; }
int os_fix_helper_thread_signals(void) { return 0; }
int os_fix_helper_signals(void) { return 0; }

/* ---- idle -------------------------------------------------------------------
 * Yield to the host event loop.  kernel_poll() in the runtime calls into us. */

void os_idle_sleep(unsigned long long nsecs)
{
	/* busy-yield: the host drives progress via its own poll loop */
}

void os_idle_prepare(void) { }

/* ---- early checks / info ----------------------------------------------------- */

int os_early_checks(char **argv) { return 0; }

void os_check_bugs(void) { }

void os_info(void) { }

/* ---- files: an fd-table over the JS side ---------------------------------------
 * fd 0/1/2 are the console.  Others are provided by the runtime (e.g. the
 * initramfs is pushed as a set of named blobs).  All I/O funnels through two
 * imports on slot-based descriptors. */

struct wasm_fd {
	const char *name;   /* NULL = closed */
	unsigned char *data;
	unsigned long size;
	unsigned long pos;
	int writable;
};
static struct wasm_fd fd_table[64];
static int next_fd = 3; /* 0-2 reserved for stdio */

void wasm_fd_register(int fd, const char *name, unsigned char *data,
		      unsigned long size);

int os_open_file(const char *path, struct openflags flags)
{
	int i;
	for (i = 3; i < 64; i++) {
		if (!fd_table[i].name)
			continue;
		if (strcmp(fd_table[i].name, path) == 0) {
			fd_table[i].pos = 0;
			return i;
		}
	}
	return -ENOENT;
}

int os_close_file(int fd)
{
	if (fd < 3 || fd >= 64 || !fd_table[fd].name)
		return -EBADF;
	fd_table[fd].name = NULL;
	return 0;
}

int os_dup_file(int fd) { return -ENOSYS; }

int os_read_file(int fd, void *buf, int len)
{
	struct wasm_fd *f;

	if (fd < 3 || fd >= 64 || !fd_table[fd].name)
		return -EBADF;
	f = &fd_table[fd];
	if (f->pos >= f->size)
		return 0;
	if (len > f->size - f->pos)
		len = f->size - f->pos;
	memcpy(buf, f->data + f->pos, len);
	f->pos += len;
	return len;
}

int os_write_file(int fd, const void *buf, int len)
{
	if (fd < 0 || fd > 2)
		return len; /* pretend success for non-console writes */
	wasm_console_write(buf, len);
	return len;
}

int os_pread_file(int fd, void *buf, int len, unsigned long long offset)
{
	struct wasm_fd *f;

	if (fd < 3 || fd >= 64 || !fd_table[fd].name)
		return -EBADF;
	f = &fd_table[fd];
	if (offset >= f->size)
		return 0;
	if (len > f->size - offset)
		len = f->size - offset;
	memcpy(buf, f->data + offset, len);
	return len;
}

int os_pwrite_file(int fd, const void *buf, int count,
		   unsigned long long offset) { return count; }

int os_seek_file(int fd, unsigned long long offset)
{
	struct wasm_fd *f;

	if (fd < 3 || fd >= 64 || !fd_table[fd].name)
		return -EBADF;
	f = &fd_table[fd];
	f->pos = offset < f->size ? offset : f->size;
	return 0;
}

int os_stat_file(const char *fname, struct uml_stat *ubuf)
{
	int i;

	memset(ubuf, 0, sizeof(*ubuf));
	for (i = 3; i < 64; i++) {
		if (!fd_table[i].name || strcmp(fd_table[i].name, fname) != 0)
			continue;
		ubuf->ust_size = fd_table[i].size;
		ubuf->ust_mode = 0100000 | 0644;
		return 0;
	}
	return -ENOENT;
}

int os_stat_fd(int fd, struct uml_stat *ubuf) { return -ENOSYS; }

int os_access(const char *file, int mode)
{
	int i;

	for (i = 3; i < 64; i++) {
		if (fd_table[i].name && strcmp(fd_table[i].name, file) == 0)
			return 0;
	}
	return -ENOENT;
}

int os_file_mode(const char *file, struct openflags *mode_out) { return 0; }
int os_lock_file(int fd, int excl) { return 0; }
int os_falloc_punch(int fd, unsigned long long offset, unsigned long long length) { return 0; }
int os_falloc_zeroes(int fd, unsigned long long offset, unsigned long long length) { return 0; }
int os_sync_file(int fd) { return 0; }
unsigned long long os_file_size(int fd) { return 0; }
int os_file_modtime(const char *file, long long *modtime) { return 0; }
int os_file_type(const char *path) { return OS_TYPE_FILE; }
int os_ioctl_generic(int fd, unsigned int cmd, unsigned long arg) { return -ENOTTY; }
int os_set_fd_block(int fd, int blocking) { return 0; }
int os_mode_fd(int fd, int *r, int *w, int *x) { return 0; }

dev_t os_major_dev_t = 0;
int os_major(dev_t dev) { return 0; }
int os_minor(dev_t dev) { return 0; }
u32 os_makedev(int major, int minor) { return 0; }

int os_event_mask(int irq) { return 0; }
int os_set_fd_async(int fd) { return 0; }
int os_clear_fd_async(int fd) { return 0; }
int os_set_ioignore(int fd) { return 0; }
int os_set_exec_close(int fd, int closeonexec) { return 0; }

/* ---- pipes / sockets / epoll --------------------------------------------------
 * UML uses these for its IRQ delivery and mconsole.  In wasm, irqs come from
 * the runtime's timer + net queue, so all of this degenerates. */

int os_pipe(int *fds, int stream, int close_on_exec)
{
	fds[0] = fds[1] = -1;
	return -ENOSYS;
}

int os_create_unix_socket(const char *file, int len, int close_on_exec) { return -ENOSYS; }
int os_connect_socket(const char *name) { return -ENOSYS; }
int os_shutdown_socket(int fd, int r, int w) { return -ENOSYS; }
void os_warn(const char *fmt, ...) { }

int os_setup_epoll(void) { return -ENOSYS; }
int os_add_epoll_fd(int events, int fd, void *data) { return -ENOSYS; }
int os_mod_epoll_fd(unsigned int events, int fd) { return -ENOSYS; }
int os_del_epoll_fd(int fd) { return -ENOSYS; }
void os_epoll_triggered(int i, int events) { }
int os_waiting_for_events_epoll(void) { return 0; }
void *os_epoll_get_data_pointer(int i) { return NULL; }
int os_eventfd(unsigned long long initval, int flags) { return -ENOSYS; }

int os_rcv_fd_msg(int fd, int *fd_in, void *buf, int size) { return -ENOSYS; }
int os_sendmsg_fds(int fd, const void *buf, int len, const int *fds, int num_fds) { return -ENOSYS; }
int os_accept_connection(int fd, int new_fd) { return -ENOSYS; }
int os_get_ifname(int fd, char *namebuf) { return -ENOSYS; }

/* ---- core dump ------------------------------------------------------------------ */

void os_dump_core(const char *why)
{
	wasm_console_write("wasmux: os_dump_core: ", 22);
	wasm_console_write(why, strlen(why));
	wasm_console_write("\n", 1);
	wasm_exit(1);
}
