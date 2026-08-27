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
 * Explicit program headers for lld compatibility (dynamic build).
 * See uml.lds.S for the full rationale.
 */
PHDRS {
	headers PT_LOAD FILEHDR PHDRS FLAGS(4);  /* PF_R */
	interp PT_INTERP FLAGS(4);               /* PF_R */
	text PT_LOAD FLAGS(5);                   /* PF_R|PF_X */
	data PT_LOAD FLAGS(7);                   /* PF_R|PF_W|PF_X */
	dynamic PT_DYNAMIC FLAGS(6);             /* PF_R|PF_W */
	tls  PT_TLS FLAGS(4);                    /* PF_R */
}
#define PHDR_INTERP :headers :interp
#define PHDR_TEXT :text
#define PHDR_DATA :data
#define PHDR_DYNAMIC :data :dynamic
#define PHDR_DATA_TLS :data :tls
#define PHDR_AFTER_TLS .after_tls : { BYTE(0) } :data
#else /* !CONFIG_LD_IS_LLD */
#define PHDR_INTERP
#define PHDR_TEXT
#define PHDR_DATA
#define PHDR_DYNAMIC
#define PHDR_DATA_TLS
#define PHDR_AFTER_TLS
#endif

SECTIONS
{
  PROVIDE (__executable_start = START);
  . = START + SIZEOF_HEADERS;
  .interp         : { *(.interp) } PHDR_INTERP
  __binary_start = .;
  . = ALIGN(4096);		/* Init code and data */
  _text = .;
  INIT_TEXT_SECTION(PAGE_SIZE) PHDR_TEXT

  . = ALIGN(PAGE_SIZE);

  /* Read-only sections, merged into text segment: */
  .hash           : { *(.hash) }
  .gnu.hash       : { *(.gnu.hash) }
  .dynsym         : { *(.dynsym) }
  .dynstr         : { *(.dynstr) }
  .gnu.version    : { *(.gnu.version) }
  .gnu.version_d  : { *(.gnu.version_d) }
  .gnu.version_r  : { *(.gnu.version_r) }
  .rel.init       : { *(.rel.init) }
  .rela.init      : { *(.rela.init) }
  .rel.text       : { *(.rel.text .rel.text.* .rel.gnu.linkonce.t.*) }
  .rela.text      : { *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*) }
  .rel.fini       : { *(.rel.fini) }
  .rela.fini      : { *(.rela.fini) }
  .rel.rodata     : { *(.rel.rodata .rel.rodata.* .rel.gnu.linkonce.r.*) }
  .rela.rodata    : { *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*) }
  .rel.data       : { *(.rel.data .rel.data.* .rel.gnu.linkonce.d.*) }
  .rela.data      : { *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*) }
  .rel.tdata	  : { *(.rel.tdata .rel.tdata.* .rel.gnu.linkonce.td.*) }
  .rela.tdata	  : { *(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*) }
  .rel.tbss	  : { *(.rel.tbss .rel.tbss.* .rel.gnu.linkonce.tb.*) }
  .rela.tbss	  : { *(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*) }
  .rel.ctors      : { *(.rel.ctors) }
  .rela.ctors     : { *(.rela.ctors) }
  .rel.dtors      : { *(.rel.dtors) }
  .rela.dtors     : { *(.rela.dtors) }
  .rel.got        : { *(.rel.got) }
  .rela.got       : { *(.rela.got) }
  .rel.bss        : { *(.rel.bss .rel.bss.* .rel.gnu.linkonce.b.*) }
  .rela.bss       : { *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*) }
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
	PROVIDE_HIDDEN(__rela_iplt_end = .);
  }
  .init           : {
    KEEP (*(.init))
  } =0x90909090
  .plt            : { *(.plt) }
  .text           : {
    _stext = .;
    TEXT_TEXT
    SCHED_TEXT
    LOCK_TEXT
    IRQENTRY_TEXT
    SOFTIRQENTRY_TEXT
    *(.fixup)
    *(.stub .text.* .gnu.linkonce.t.*)
    /* .gnu.warning sections are handled specially by elf32.em.  */
    *(.gnu.warning)

    . = ALIGN(PAGE_SIZE);
  } =0x90909090
  . = ALIGN(PAGE_SIZE);
  .syscall_stub : {
	__syscall_stub_start = .;
	*(.__syscall_stub*)
	__syscall_stub_end = .;
  }
  .fini           : {
    KEEP (*(.fini))
  } =0x90909090

  .kstrtab : { *(.kstrtab) }

  #include <asm/common.lds.S>

  __init_begin = .;
  init.data : { INIT_DATA }
  __init_end = .;

  /* Ensure the __preinit_array_start label is properly aligned.  We
     could instead move the label definition inside the section, but
     the linker would then create the section even if it turns out to
     be empty, which isn't pretty.  */
  . = ALIGN(32 / 8);
  .preinit_array     : { *(.preinit_array) }
  .init_array     : {
    *(.kasan_init)
    *(.init_array.*)
    *(.init_array)
  }
  .fini_array     : { *(.fini_array) }
  .data           : {
    INIT_TASK_DATA(KERNEL_STACK_SIZE)
    DATA_DATA
    *(.data.* .gnu.linkonce.d.*)
    SORT(CONSTRUCTORS)
  }
  .data1          : { *(.data1) }
  .eh_frame       : { KEEP (*(.eh_frame)) }
  .gcc_except_table   : { *(.gcc_except_table) }
  .dynamic        : { *(.dynamic) } PHDR_DYNAMIC
  .ctors          : {
    /* gcc uses crtbegin.o to find the start of
       the constructors, so we make sure it is
       first.  Because this is a wildcard, it
       doesn't matter if the user does not
       actually link against crtbegin.o; the
       linker won't look for a file to match a
       wildcard.  The wildcard also means that it
       doesn't matter which directory crtbegin.o
       is in.  */
    KEEP (*crtbegin.o(.ctors))
    /* We don't want to include the .ctor section from
       from the crtend.o file until after the sorted ctors.
       The .ctor section from the crtend file contains the
       end of ctors marker and it must be last */
    KEEP (*(EXCLUDE_FILE (*crtend.o ) .ctors))
    KEEP (*(SORT(.ctors.*)))
    KEEP (*(.ctors))
  }
  .dtors          : {
    KEEP (*crtbegin.o(.dtors))
    KEEP (*(EXCLUDE_FILE (*crtend.o ) .dtors))
    KEEP (*(SORT(.dtors.*)))
    KEEP (*(.dtors))
  }
  .jcr            : { KEEP (*(.jcr)) }
  .got            : { *(.got.plt) *(.got) }
  /*
   * TLS sections placed last before BSS.  See uml.lds.S for rationale.
   */
  .tdata	  : { *(.tdata .tdata.* .gnu.linkonce.td.*) } PHDR_DATA_TLS
  .tbss		  : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) } PHDR_DATA_TLS
  PHDR_AFTER_TLS
  _edata = .;
  PROVIDE (edata = .);
  .bss            : {
   __bss_start = .;
   *(.dynbss)
   *(.bss .bss.* .gnu.linkonce.b.*)
   *(COMMON)
   /* Align here to ensure that the .bss section occupies space up to
      _end.  Align after .bss to ensure correct alignment even if the
      .bss section disappears because there are no input sections.  */
   . = ALIGN(32 / 8);
  . = ALIGN(32 / 8);
  }
   __bss_stop = .;
  _end = .;
  PROVIDE (end = .);

  STABS_DEBUG
  DWARF_DEBUG
  ELF_DETAILS

  DISCARDS
}

ASSERT(__syscall_stub_end - __syscall_stub_start <= PAGE_SIZE,
       "STUB code must not be larger than one page");
