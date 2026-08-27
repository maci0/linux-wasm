# SPDX-License-Identifier: GPL-2.0
CFLAGS_REMOVE_empty.o += $(CC_FLAGS_LTO)

hostprogs-always-y	+= modpost mk_elfconfig
always-y		+= empty.o

# wasm target: empty.o is a WebAssembly object which mk_elfconfig cannot
# parse.  Build it with the host compiler so elfconfig.h reflects a real
# ELF class; the actual kernel objects stay wasm.
ifeq ($(ARCH),wasm)
$(obj)/empty.o: FORCE
	$(Q)$(HOSTCC) $(KBUILD_HOSTCFLAGS) -c $(srctree)/$(src)/empty.c -o $@
endif

modpost-objs	:= modpost.o file2alias.o sumversion.o symsearch.o

devicetable-offsets-file := devicetable-offsets.h

$(obj)/$(devicetable-offsets-file): $(obj)/devicetable-offsets.s FORCE
	$(call filechk,offsets,__DEVICETABLE_OFFSETS_H__)

targets += $(devicetable-offsets-file) devicetable-offsets.s

# dependencies on generated files need to be listed explicitly

$(obj)/modpost.o $(obj)/file2alias.o $(obj)/sumversion.o $(obj)/symsearch.o: $(obj)/elfconfig.h
$(obj)/file2alias.o: $(obj)/$(devicetable-offsets-file)

quiet_cmd_elfconfig = MKELF   $@
      cmd_elfconfig = $(obj)/mk_elfconfig < $< > $@

$(obj)/elfconfig.h: $(obj)/empty.o $(obj)/mk_elfconfig FORCE
	$(call if_changed,elfconfig)

targets += elfconfig.h
