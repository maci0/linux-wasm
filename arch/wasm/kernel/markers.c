// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/markers.c: section-marker symbols.
 *
 * The kernel normally receives symbols such as _stext, _etext and the
 * __initcall*_start/end set from the linker script.  wasm-ld cannot lay
 * out custom sections, so these markers are plain data instead:
 *
 *  - the *_start/_end pairs are kept adjacent, which makes the generic
 *    code's "end - start" sizes and reservations empty (nothing is freed
 *    or reserved based on them);
 *  - the real kernel-image reservation is done explicitly in setup.c;
 *  - the initcall levels are never iterated on wasm: init/main.c drives
 *    the required initcalls explicitly (wasm-ld cannot order the
 *    .initcall*.init sections).
 */

char _text, _stext, _etext;
/* rodata */
char __start_rodata, __end_rodata;
/* data */
char _sdata, _data, _edata;
/* bss */
char __bss_start, __bss_stop;
/* init */
char _init_begin, __init_begin, _sinittext;
char _init_end, __init_end, _einittext;
/* setup parameters and initcalls (never iterated on wasm) */
char __setup_start, __setup_end;
char __initcall_start, __initcall0_start, __initcall1_start;
char __initcall2_start, __initcall3_start, __initcall4_start;
char __initcall5_start, __initcallrootfs_start, __initcall6_start;
char __initcall7_start, __initcall_end;
char __con_initcall_start, __con_initcall_end;
/* exception table / notes / module versions / builtin firmware */
char __start___ex_table, __stop___ex_table;
char __start_notes, __stop_notes;
char __start___modver, __stop___modver;
char __start_builtin_fw, __end_builtin_fw;
/* end of image */
char _end;
