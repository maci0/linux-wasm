/* SPDX-License-Identifier: GPL-2.0 */
#include <asm/vmlinux.lds.h>
#include <asm/page.h>

OUTPUT_FORMAT(ELF_FORMAT)
OUTPUT_ARCH(ELF_ARCH)
ENTRY(_start)
jiffies = jiffies_64;

VERSION {
  {
    local: *;
  };
}

#ifdef CONFIG_LD_IS_LLD
/*
 * Explicit program headers (PHDRS) for lld compatibility.
 *
 * lld creates a new PT_LOAD segment for each group of sections with
 * different flags.  With the UML linker script, this produces 20+
 * LOAD segments, many of which share the same page with different
 * permissions.  UML's ELF loader cannot mmap() overlapping page
 * ranges, causing a crash at boot.
 *
 * GNU ld (bfd) merges adjacent sections aggressively and produces a
 * correct layout without explicit PHDRS, so this block is lld-only.
 *
 * Layout:
 *   headers (R)   - ELF + program headers (needed by glibc static
 *                   init which reads e_phnum at the load base)
 *   text    (R_E) - all executable code
 *   data    (RWE) - everything else (rodata has WA flags in the
 *                   kernel, .exit.text/.altinstr_replacement are AX)
 *   tls            - thread-local storage template (.tdata + .tbss)
 */
PHDRS {
	headers PT_LOAD FILEHDR PHDRS FLAGS(4);  /* PF_R */
	text PT_LOAD FLAGS(5);                   /* PF_R|PF_X */
	data PT_LOAD FLAGS(7);                   /* PF_R|PF_W|PF_X */
	tls  PT_TLS FLAGS(4);                    /* PF_R */
}
#define PHDR_TEXT :text
#define PHDR_DATA :data
#define PHDR_DATA_TLS :data :tls
#define PHDR_AFTER_TLS .after_tls : { BYTE(0) } :data
#else /* !CONFIG_LD_IS_LLD */
#define PHDR_TEXT
#define PHDR_DATA
#define PHDR_DATA_TLS
#define PHDR_AFTER_TLS
#endif

SECTIONS
{
  /* This must contain the right address - not quite the default ELF one.*/
  PROVIDE (__executable_start = START);
  /* Static binaries stick stuff here, like the sigreturn trampoline,
   * invisibly to objdump.  So, just make __binary_start equal to the very
   * beginning of the executable, and if there are unmapped pages after this,
   * they are forever unusable.
   */
  __binary_start = START;

  . = START + SIZEOF_HEADERS;
  . = ALIGN(PAGE_SIZE);

  _text = .;
  INIT_TEXT_SECTION(0) PHDR_TEXT

  .text      :
  {
    _stext = .;
    TEXT_TEXT
    SCHED_TEXT
    LOCK_TEXT
    IRQENTRY_TEXT
    SOFTIRQENTRY_TEXT
    *(.fixup)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)
    *(.gnu.linkonce.t*)
  }

  . = ALIGN(PAGE_SIZE);
  .syscall_stub : {
	__syscall_stub_start = .;
	*(.__syscall_stub*)
	__syscall_stub_end = .;
  }

  /*
   * These are needed even in a static link, even if they wind up being empty.
   * Newer glibc needs these __rel{,a}_iplt_{start,end} symbols.
   */
  .rel.plt : {
	*(.rel.plt)
	PROVIDE_HIDDEN(__rel_iplt_start = .);
	*(.rel.iplt)
	PROVIDE_HIDDEN(__rel_iplt_end = .);
  }
  .rela.plt : {
	*(.rela.plt)
	PROVIDE_HIDDEN(__rela_iplt_start = .);
	*(.rela.iplt)
	/*
	 * lld places IRELATIVE relocations in .rela.dyn rather than
	 * .rela.iplt.  Include .rela.dyn here so __rela_iplt_{start,end}
	 * brackets all IFUNC relocations for glibc's static resolver.
	 * With bfd this section is empty and has no effect.
	 */
	*(.rela.dyn)
	PROVIDE_HIDDEN(__rela_iplt_end = .);
  }

  #include <asm/common.lds.S>

  __init_begin = .;
  init.data : { INIT_DATA }
  __init_end = .;

  .data    :
  {
    INIT_TASK_DATA(KERNEL_STACK_SIZE)
    DATA_DATA
    *(.gnu.linkonce.d*)
    CONSTRUCTORS
  }
  .data1   : { *(.data1) }
  .ctors         :
  {
    *(.ctors)
  }
  .dtors         :
  {
    *(.dtors)
  }

  .got           : { *(.got.plt) *(.got) }
  .eh_frame       : { KEEP (*(.eh_frame)) }
  .dynamic       : { *(.dynamic) }
  /* We want the small data sections together, so single-instruction offsets
     can access them all, and initialized data all before uninitialized, so
     we can shorten the on-disk segment size.  */
  .sdata     : { *(.sdata) }
  /*
   * TLS sections placed last before BSS.  With lld's explicit PHDRS,
   * this ensures PT_TLS covers only .tdata/.tbss (lld extends each
   * PHDR to span all sections assigned to it).  PHDR_AFTER_TLS emits
   * a dummy section to switch back from :data :tls to :data only.
   */
  .tdata	  : { *(.tdata .tdata.* .gnu.linkonce.td.*) } PHDR_DATA_TLS
  .tbss		  : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) } PHDR_DATA_TLS
  PHDR_AFTER_TLS
  _edata  =  .;
  PROVIDE (edata = .);
  . = ALIGN(PAGE_SIZE);
  __bss_start = .;
  PROVIDE(_bss_start = .);
  SBSS(0)
  BSS(0)
   __bss_stop = .;
  _end = .;
  PROVIDE (end = .);

  STABS_DEBUG
  DWARF_DEBUG
  ELF_DETAILS

  DISCARDS
}
