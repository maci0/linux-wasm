.. SPDX-License-Identifier: GPL-2.0

===============================
Wasm (WebAssembly) architecture
===============================

The ``wasm`` architecture (``arch/wasm``) builds Linux as a freestanding
WebAssembly module.  The kernel runs inside a WebAssembly virtual machine
(a browser, bun, wasmtime, wasmer, ...) that provides a small host ABI
(console output, a monotonic clock, one-shot timers, randomness, and the
shell input channel) through the imports declared in
``arch/wasm/include/shared/os-wasm.h`` (module ``wasmux``).

The port is modeled on UML (``arch/um``): a host OS layer
(``arch/wasm/os-wasm/os.c``) implements the UML ``os_*`` primitives
against the wasm host ABI instead of host Linux system calls.

The kernel boots ``start_kernel()`` to completion: the early console,
memory setup, the page allocator, SLUB, vmalloc, the scheduler, the VFS
and the unpacking of the built-in initramfs all run, and ``kernel_init()``
finishes with the kernel-resident shell (``arch/wasm/kernel/shell.c``)
standing in for ``/init``.

Subsystems
==========

The following are known to work and are exercised at boot:

- early console (registered from ``setup_arch``, so the whole boot log is
  visible immediately)
- physical memory setup (64 MiB of RAM managed by the kernel, out of the
  256 MiB linear memory; see ``arch/wasm/kernel/setup.c``)
- page allocator, slab (SLUB), vmalloc
- scheduler classes and ``sched_init`` (classes are chained explicitly;
  see ``arch/wasm/kernel/sched.c``)
- the kernel time base (``jiffies`` aliases ``jiffies_64`` in
  ``arch/wasm/kernel/jiffies.c``)
- the built-in initramfs, unpacked by the real ``populate_rootfs()``
  (driven explicitly, see ``init/main.c`` ``do_initcalls()``)
- the kernel-resident shell: ``kernel_init()`` runs ``wasm_shell()``
  instead of exec'ing ``/init``.  The shell blocks in the
  ``wasm_shell_wait`` import for console input (the native hosts read a
  line; the browser unwinds control to the host) and processes lines via
  the exported ``wasm_shell_input``.  Commands: ``help``, ``version``,
  ``free``, ``uptime``, ``tasks``, ``echo``, ``clear``, ``reboot``.
- the same module boots under several runtimes (browser WebAssembly,
  bun, wasmtime, wasmer)

Not implemented / limitations
=============================

- No userspace: ``copy_thread()`` can only start kernel threads, there is
  no syscall ABI (``sys_call_table`` is all ``sys_ni_syscall``), and
  ``/init`` cannot be executed.  The initramfs is unpacked but nothing in
  it runs; the shell is kernel-resident, not a userspace shell.
- Single-threaded cooperative scheduling: the wasm call stack cannot be
  captured or restored, so ``__switch_to()`` starts the first scheduled
  kernel thread as a nested function call that never returns.  There is
  no preemption and no timer-driven tick (``calibrate_delay_is_known()``
  reports a fixed loops-per-jiffy; the host timer import is a stub).
- Atomics: LLVM's ``-mthread-model single`` lowering of the
  ``__atomic_*`` builtins compiles ``cmpxchg`` into a sequence that treats
  the expected value as zero and never installs the new value.
  ``arch/xchg`` and ``arch/cmpxchg`` (and the atomic_t variants) are
  therefore plain volatile load-compare-store, which is exactly correct
  for a single-threaded, non-preemptible target (see
  ``arch/wasm/include/asm/cmpxchg.h`` and ``atomic.h``).
- Several boot paths are adapted for the cooperative model under
  ``CONFIG_WASM``: ``kernel_init`` does not wait for kthreadd, the
  workqueue skips its worker/rescuer/release kthreads, devtmpfs skips its
  daemon, ``flush_delayed_fput()`` drains the list directly instead of
  waiting on a workqueue that can never run, and ``console_init()`` skips
  the empty initcall section.
- wasm-ld limitations: the linker script cannot lay out custom sections
  or define symbols, so the section-marker symbols live in
  ``arch/wasm/kernel/markers.c``, the initcall levels are empty,
  ``kallsyms`` tables are stubs, and the ``jiffies`` alias is a real
  symbol (``arch/wasm/kernel/jiffies.c``).  Zero-initialized statics can
  get addresses that overlap initialized data, and an initialized
  symbol's address can be split from its content; boot-critical data
  (``struct memblock``, the SLUB boot caches) lives in early arch objects
  so wasm-ld cannot misplace it.
- No ``__builtin_return_address()`` on the wasm backend: a handful of
  core call sites use ``_RET_IP_`` instead (identical on every other
  architecture).
- The network device (``CONFIG_WASM_NET``) compiles and the bridge
  plumbing exists in the demo runtime, but there is no user to configure
  an address.
- No ``/dev/console`` node: devtmpfs has no daemon on wasm, so
  ``console_on_rootfs()`` prints a warning and the shell talks to the
  console directly.
- Kconfig capability flags (KASAN, kmemleak, audit, seccomp, syscall
  tracepoints, LTO) are selected but not exercised; without userspace
  there is nothing for them to act on.
- Memory layout is fixed: 256 MiB linear memory (64 MiB managed RAM,
  vmalloc area, 16 MiB wasm stack).  The module is about 60 MB.

Toolchain
=========

The kernel is built with ``zig cc`` (``clang`` targeting
``wasm32-freestanding``); see ``scripts/build-linux.sh`` in the wasmux
project.  A minimal configuration is provided as ``wasm_defconfig``.
The linked module is post-processed by the wasmux project's
``patch-wasm.ts`` relocator, which moves the late data segments to a safe
area and rewrites every reference to them (segment offsets, ``i32.const``
immediates, load/store memarg offsets, data-to-data pointers); without it
the kernel does not boot.
