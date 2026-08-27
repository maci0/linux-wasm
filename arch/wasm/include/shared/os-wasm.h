/* SPDX-License-Identifier: GPL-2.0 */
/*
 * os-wasm: the "host OS" layer for the wasm port.
 *
 * Every os_* primitive that UML implements against host Linux syscalls
 * becomes either a WebAssembly import (module "wasmux") or a stub.
 * The browser runtime provides console, timers, and the network bridge;
 * everything process/ptrace-related is meaningless in a single-threaded
 * wasm module and becomes a no-op.
 */
#ifndef __OS_WASM_H
#define __OS_WASM_H

#include <linux/types.h>

/*
 * The functions below are provided by the wasm host runtime, so they are
 * declared as WebAssembly imports (module "wasmux").  Declaring them in
 * the object lets the linker resolve them as imports instead of undefined
 * symbols (wasm-ld rejects undefined symbols in this configuration).
 */
#ifdef CONFIG_WASM
#define WASM_HOST_IMPORT(name) \
	__attribute__((import_module("wasmux"), import_name(name)))
#else
#define WASM_HOST_IMPORT(name)
#endif

WASM_HOST_IMPORT("wasm_console_write")
void wasm_console_write(const char *buf, int len);
WASM_HOST_IMPORT("wasm_net_send")
void wasm_net_send(const void *buf, int len);
WASM_HOST_IMPORT("wasm_net_recv")
int  wasm_net_recv(void *buf, int max_len);
WASM_HOST_IMPORT("wasm_exit")
void wasm_exit(int code);
WASM_HOST_IMPORT("wasm_time_ms")
unsigned long long wasm_time_ms(void);
WASM_HOST_IMPORT("wasm_time_ns")
unsigned long long wasm_time_ns(void);
WASM_HOST_IMPORT("wasm_timer_arm")
void wasm_timer_arm(unsigned long long ns);  /* one-shot; fires irq 0 */
WASM_HOST_IMPORT("wasm_random")
int  wasm_random(void *buf, int len);

/* os-layer prototypes (implemented in arch/wasm/os-wasm/os.c) */
void os_idle_sleep(unsigned long long nsecs);
void os_idle_prepare(void);

#endif /* __OS_WASM_H */
