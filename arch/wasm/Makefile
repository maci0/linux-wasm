# SPDX-License-Identifier: GPL-2.0
#
# Architecture makefile for freestanding wasm32.
# Modeled on arch/um but the "host OS" is a WebAssembly runtime (browser).

ARCH_DIR := arch/wasm
HEADER_ARCH := wasm

KBUILD_DEFCONFIG := wasm_defconfig

SHARED_HEADERS := $(ARCH_DIR)/include/shared
ARCH_INCLUDE := -I$(srctree)/$(SHARED_HEADERS)

KBUILD_CFLAGS += $(ARCH_INCLUDE)
KBUILD_AFLAGS += $(ARCH_INCLUDE)

# No subarch: everything is generic wasm32.
core-y += $(ARCH_DIR)/

KBUILD_CFLAGS += -fno-builtin -Wno-unused-command-line-argument
KBUILD_CFLAGS += -mthread-model single -fno-pic -fno-stack-protector
# Chromium's V8 crashes (rather than trapping) on out-of-bounds bulk
# memory operations; emit plain load/store loops instead.
KBUILD_CFLAGS += -mno-bulk-memory
KBUILD_CFLAGS += -fno-unwind-tables -fno-asynchronous-unwind-tables
KBUILD_CFLAGS += -Wl,--no-entry -nostdlib

LDFLAGS_vmlinux += -Wl,--no-entry -Wl,--export-dynamic

head-y := $(ARCH_DIR)/kernel/head.o
libs-y += $(ARCH_DIR)/lib/

all: vmlinux.wasm

# Override the top-level vmlinux link: the "kernel" is a freestanding
# wasm32 module linked by zig cc.  The runtime enters it via the exported
# start_kernel symbol (see arch/wasm/include/shared/os-wasm.h).
# wasm-ld cannot lay out custom sections, so the kernel's section-marker
# symbols come from arch/wasm/kernel/markers.c instead of a linker script.
vmlinux.wasm: vmlinux.a
	$(Q)zig cc --target=wasm32-freestanding -O2 -nostdlib \
		-Wl,--no-entry \
		-Wl,--initial-memory=268435456 \
		-Wl,--export=start_kernel \
		-Wl,--export=wasm_console_input \
		-Wl,--export=wasm_raise_irq \
		-Wl,--export-memory \
		-Wl,--strip-debug \
		-o $@ vmlinux.a
	@echo "  BUILD   $@"

