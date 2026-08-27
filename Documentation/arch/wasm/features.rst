.. SPDX-License-Identifier: GPL-2.0

===============================
Wasm (WebAssembly) architecture
===============================

The ``wasm`` architecture (``arch/wasm``) builds Linux as a freestanding
WebAssembly module.  The kernel runs inside a WebAssembly virtual machine
(a browser, bun, wasmtime, wasmer, ...) that provides a small host
ABI (console output, a monotonic clock, one-shot timers, and randomness)
through the imports declared in ``arch/wasm/include/shared/os-wasm.h``
(module ``wasmux``).

The port is modeled on UML (``arch/um``): a host OS layer
(``arch/wasm/os-wasm/os.c``) implements the UML ``os_*`` primitives
against the wasm host ABI instead of host Linux system calls.

Subsystems
==========

The following subsystems are known to work:

- early console (registered from ``setup_arch``, so the whole boot log is
  visible immediately)
- physical memory setup (64 MiB of RAM managed by the kernel, out of the
  256 MiB linear memory; see ``arch/wasm/kernel/setup.c``)
- page allocator, slab (SLUB), vmalloc
- scheduler classes and ``sched_init`` (classes are chained explicitly;
  see ``arch/wasm/kernel/sched.c``)
- the kernel time base (``jiffies`` aliases ``jiffies_64`` in
  ``arch/wasm/kernel/jiffies.c``)

Not implemented / limitations
=============================

- No userspace: ``copy_thread()`` can only start kernel threads, there is
  no syscall ABI and ``/init`` cannot be executed.  The initramfs is
  unpacked by ``populate_rootfs()`` (driven explicitly, see
  ``init/main.c`` ``do_initcalls()``), but the boot ends with
  "No working init found".
- Single-threaded cooperative scheduling: the wasm call stack cannot be
  captured, so ``__switch_to()`` runs the first scheduled kernel thread as
  a nested call and never returns from it.
- wasm-ld limitations: the linker script cannot lay out custom sections,
  so the section-marker symbols live in ``arch/wasm/kernel/markers.c``,
  the initcall levels are empty and ``kallsyms`` tables are stubs.
- Network device (``CONFIG_WASM_NET``) compiles but has no user to
  configure it.
- The kernel does not yet boot all the way to ``start_kernel()``
  completion: a memory-corruption bug in the late boot path is under
  investigation.

Toolchain
=========

The kernel is built with ``zig cc`` (``clang`` targeting
``wasm32-freestanding``); see ``scripts/build-linux.sh`` in the wasmux
project.  A minimal configuration is provided as ``wasm_defconfig``.
