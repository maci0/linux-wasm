// SPDX-License-Identifier: GPL-2.0
/*
 * arch/wasm/kernel/wasm-exec.c: minimal in-kernel wasm interpreter with
 * WASI syscall shims.
 *
 * Prototype: lets the shell "run" a wasm32-wasi module from the
 * initramfs.  The interpreter is the "CPU" that wasm32 lacks - the
 * kernel becomes the execution engine for foreign bytecode, and the
 * WASI surface is the syscall table.  This first slice covers the
 * instruction subset a hello-world needs (constants, locals, loads and
 * stores, arithmetic, calls, control flow) and the fd_write/proc_exit
 * syscalls; extend the opcode set and the syscall table to grow this
 * into real userspace.
 *
 * The module runs in a fresh 4 MiB guest memory (never the kernel's own
 * linear memory), so a buggy guest cannot corrupt the kernel.  All
 * guest memory accesses are bounds-checked.
 */

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <os-wasm.h>

/* wasm's i32 is a signed 32-bit value */
typedef s32 i32;

#define WASM_MEM_PAGES		64	/* 4 MiB guest memory */
#define WASM_GUEST_BASE		0x03800000u /* boot-reserved, see setup.c */
#define WASM_GUEST_MEM		(WASM_GUEST_BASE)
#define WASM_GUEST_STATE	(WASM_GUEST_BASE + 0x400000)
#define WASM_OP_STACK		1024
#define WASM_CALL_DEPTH		32
#define WASM_MAX_TYPES		64
#define WASM_MAX_IMPORTS	16
#define WASM_MAX_FUNCS		128
#define WASM_MAX_EXPORTS	16
#define WASM_MAX_DATAS		16
#define WASM_LOCALS_MAX		64

struct wasm_type {
	u8 nparams;
	u8 nresults;
	u8 params[8];
	u8 results[2];
};

struct wasm_import {
	u32 typeidx;
	u32 wasi;		/* index into the wasi table, or -1 */
};

struct wasm_func {
	u32 typeidx;
	u32 nlocals;
	u32 bodylen;
	const u8 *body;
};

struct wasm_export {
	const char *name;
	u8 kind;
	u32 idx;
};

struct wasm_data {
	u32 off;
	u32 len;
	const u8 *data;
};

struct wasm_mod {
	const u8 *p;
	u32 size;
	struct wasm_type types[WASM_MAX_TYPES];
	u32 ntypes;
	struct wasm_import imports[WASM_MAX_IMPORTS];
	u32 nimports;
	struct wasm_func funcs[WASM_MAX_FUNCS];
	u32 nfuncs;		/* defined functions */
	u32 nfunc_total;	/* imports + defined */
	struct wasm_export exports[WASM_MAX_EXPORTS];
	u32 nexports;
	struct wasm_data datas[WASM_MAX_DATAS];
	u32 ndatas;
	u32 mempages;
	u8 *memory;
	int start_idx;
};

struct wasm_frame {
	u32 funcidx;		/* index into mod->funcs */
	u32 pc;
	u32 nlocals;
	i32 locals[WASM_LOCALS_MAX];
};

struct wasm_exec {
	struct wasm_mod *m;
	i32 stack[WASM_OP_STACK];
	u32 sp;
	struct wasm_frame frames[WASM_CALL_DEPTH];
	u32 depth;
	int exited;
	i32 exit_code;
};

/* ---- LEB128 ---- */

static u32 uleb(const u8 *p, u32 size, u32 *pos)
{
	u32 v = 0, shift = 0;
	while (*pos < size) {
		u8 b = p[(*pos)++];
		v |= (u32)(b & 0x7f) << shift;
		if (!(b & 0x80))
			break;
		shift += 7;
	}
	return v;
}

static i32 sleb(const u8 *p, u32 size, u32 *pos)
{
	u32 v = 0, shift = 0;
	u8 b = 0;
	while (*pos < size) {
		b = p[(*pos)++];
		v |= (u32)(b & 0x7f) << shift;
		shift += 7;
		if (!(b & 0x80))
			break;
	}
	if (shift < 32 && (b & 0x40))
		v |= (u32)-1 << shift;
	return (i32)v;
}

/* ---- guest memory access (bounds-checked) ---- */

static inline u32 gmem_size(struct wasm_mod *m)
{
	return m->mempages * 65536;
}

static inline int gaddr_ok(struct wasm_mod *m, u32 addr, u32 len)
{
	return addr <= gmem_size(m) && len <= gmem_size(m) - addr;
}

static u32 gread32(struct wasm_mod *m, u32 addr)
{
	if (!gaddr_ok(m, addr, 4))
		return 0;
	return m->memory[addr] | (m->memory[addr + 1] << 8) |
	       (m->memory[addr + 2] << 16) | ((u32)m->memory[addr + 3] << 24);
}

static void gwrite32(struct wasm_mod *m, u32 addr, u32 val)
{
	if (!gaddr_ok(m, addr, 4))
		return;
	m->memory[addr] = val;
	m->memory[addr + 1] = val >> 8;
	m->memory[addr + 2] = val >> 16;
	m->memory[addr + 3] = val >> 24;
}

/* ---- WASI syscall table ---- */

enum {
	WASI_FD_WRITE,
	WASI_PROC_EXIT,
	WASI_FD_CLOSE,
	WASI_FD_SEEK,
	WASI_FD_FDSTAT_GET,
	WASI_COUNT,
};

static const struct {
	const char *name;
	u8 nparams;
} wasi_table[WASI_COUNT] = {
	[WASI_FD_WRITE]     = { "fd_write",     4 },
	[WASI_PROC_EXIT]    = { "proc_exit",    1 },
	[WASI_FD_CLOSE]     = { "fd_close",     1 },
	[WASI_FD_SEEK]      = { "fd_seek",      4 },
	[WASI_FD_FDSTAT_GET] = { "fd_fdstat_get", 2 },
};

static int wasi_fd_write(struct wasm_mod *m, i32 fd, i32 iovs, i32 iovs_len,
			 i32 nwritten)
{
	u32 total = 0;
	i32 i;

	for (i = 0; i < iovs_len; i++) {
		u32 p = gread32(m, iovs + i * 8);
		u32 l = gread32(m, iovs + i * 8 + 4);

		if (!gaddr_ok(m, p, l))
			return 8 /* EBADF */;
		if (fd == 1 || fd == 2)
			wasm_console_write((const char *)m->memory + p, l);
		total += l;
	}
	gwrite32(m, nwritten, total);
	return 0;
}

static int wasi_proc_exit(struct wasm_exec *ex, i32 code)
{
	ex->exited = 1;
	ex->exit_code = code;
	return 0;
}

static int wasi_fd_fdstat_get(struct wasm_mod *m, i32 fd, i32 out)
{
	/* filetype=2 (character device), size=0, flags=0, rights as for
	 * a terminal, fs_rights_base bits 0x0c0000040 (fd_write/fd_read) */
	static const u8 fdstat[24] = { 2, 0, 0, 0, 0, 0, 0, 0,
				       0x40, 0x00, 0x00, 0x0c,
				       0x00, 0x00, 0x00, 0x00,
				       0x00, 0x00, 0x00, 0x00,
				       0x00, 0x00, 0x00, 0x00 };
	if (!gaddr_ok(m, out, sizeof(fdstat)))
		return 8;
	memcpy(m->memory + out, fdstat, sizeof(fdstat));
	return 0;
}

/* ---- module parsing ---- */

static int wasm_parse(struct wasm_mod *m, const u8 *p, u32 size)
{
	u32 pos = 8;		/* skip magic + version */

	m->p = p;
	m->size = size;
	m->start_idx = -1;

	if (size < 8 || p[0] != 0x00 || p[1] != 0x61 || p[2] != 0x73 ||
	    p[3] != 0x6d)
		return -EINVAL;

	while (pos < size) {
		u8 id = p[pos++];
		u32 slen = uleb(p, size, &pos);
		u32 s = pos;
		u32 end = pos + slen;

		switch (id) {
		case 1: {	/* type */
			u32 n = uleb(p, end, &s);
			while (n-- > 0 && s < end) {
				struct wasm_type *t;
				u32 k, i;

				if (p[s++] != 0x60)
					return -EINVAL;
				if (m->ntypes >= WASM_MAX_TYPES)
					return -E2BIG;
				t = &m->types[m->ntypes++];
				k = uleb(p, end, &s);
				t->nparams = (u8)k;
				for (i = 0; i < k && i < 8; i++)
					t->params[i] = p[s++];
				k = uleb(p, end, &s);
				t->nresults = (u8)k;
				for (i = 0; i < k && i < 2; i++)
					t->results[i] = p[s++];
			}
			break;
		}
		case 2: {	/* import */
			u32 n = uleb(p, end, &s);
			while (n-- > 0 && s < end) {
				u32 ml, nl, i;
				struct wasm_import *im;
				char mname[32], iname[32];

				ml = uleb(p, end, &s);
				for (i = 0; i < ml && i < sizeof(mname) - 1; i++)
					mname[i] = p[s++];
				mname[i] = '\0';
				nl = uleb(p, end, &s);
				for (i = 0; i < nl && i < sizeof(iname) - 1; i++)
					iname[i] = p[s++];
				iname[i] = '\0';
				if (p[s] != 0)	/* only function imports */
					return -EINVAL;
				s++;
				if (m->nimports >= WASM_MAX_IMPORTS)
					return -E2BIG;
				im = &m->imports[m->nimports++];
				im->typeidx = uleb(p, end, &s);
				im->wasi = -1;
				if (ml == strlen("wasi_snapshot_preview1") &&
				    !memcmp(mname, "wasi_snapshot_preview1", ml)) {
					for (i = 0; i < WASI_COUNT; i++) {
						if (nl == strlen(wasi_table[i].name) &&
						    !memcmp(iname, wasi_table[i].name, nl)) {
							im->wasi = i;
							break;
						}
					}
				}
			}
			break;
		}
		case 3: {	/* function */
			u32 n = uleb(p, end, &s);
			u32 i;

			m->nfunc_total = m->nimports;
			for (i = 0; i < n && s < end; i++) {
				if (m->nfuncs >= WASM_MAX_FUNCS)
					return -E2BIG;
				m->funcs[m->nfuncs++].typeidx = uleb(p, end, &s);
			}
			m->nfunc_total += n;
			break;
		}
		case 5: {	/* memory */
			u32 n = uleb(p, end, &s);

			if (n < 1)
				return -EINVAL;
			(void)uleb(p, end, &s);	/* min pages */
			m->mempages = WASM_MEM_PAGES;
			break;
		}
		case 7: {	/* export */
			u32 n = uleb(p, end, &s);
			while (n-- > 0 && s < end) {
				u32 nl, i;
				struct wasm_export *ex;

				nl = uleb(p, end, &s);
				if (m->nexports >= WASM_MAX_EXPORTS)
					return -E2BIG;
				ex = &m->exports[m->nexports++];
				ex->name = (const char *)p + s;
				for (i = 0; i < nl; i++)
					p[s++];	/* keep name in-place */
				((char *)ex->name)[nl] = '\0';
				ex->kind = p[s++];
				ex->idx = uleb(p, end, &s);
				if (ex->kind == 0 && nl == 6 &&
				    !memcmp(ex->name, "_start", 6))
					m->start_idx = ex->idx;
			}
			break;
		}
		case 10: {	/* code */
			u32 n = uleb(p, end, &s);
			u32 i;

			for (i = 0; i < n && s < end && i < m->nfuncs; i++) {
				u32 bodylen = uleb(p, end, &s);
				u32 bodyend = s + bodylen;
				u32 ngroups = uleb(p, end, &s);
				struct wasm_func *f = &m->funcs[i];

				while (ngroups-- > 0) {
					u32 cnt = uleb(p, end, &s);

					f->nlocals += cnt;
					s++;	/* valtype */
				}
				f->body = p + s;
				f->bodylen = bodyend - s;
				s = bodyend;
			}
			break;
		}
		case 11: {	/* data */
			u32 n = uleb(p, end, &s);
			while (n-- > 0 && s < end) {
				struct wasm_data *d;

				if (m->ndatas >= WASM_MAX_DATAS)
					return -E2BIG;
				d = &m->datas[m->ndatas++];
				(void)uleb(p, end, &s);	/* memidx */
				if (p[s++] != 0x41)	/* i32.const offset */
					return -EINVAL;
				d->off = (u32)sleb(p, end, &s);
				if (p[s++] != 0x0b)
					return -EINVAL;
				d->len = uleb(p, end, &s);
				d->data = p + s;
				s += d->len;
			}
			break;
		}
		default:
			break;	/* skip unknown sections (incl. custom) */
		}
		pos = end;
	}
	return 0;
}

static int wasm_init_mem(struct wasm_mod *m)
{
	u32 i, sz = m->mempages * 65536;

	m->memory = (u8 *)WASM_GUEST_MEM;
	memset(m->memory, 0, sz);
	for (i = 0; i < m->ndatas; i++) {
		if (!gaddr_ok(m, m->datas[i].off, m->datas[i].len))
			return -EFAULT;
		memcpy(m->memory + m->datas[i].off, m->datas[i].data,
		       m->datas[i].len);
	}
	return 0;
}

/* ---- interpreter ---- */

static int wasm_call_import(struct wasm_exec *ex, u32 idx)
{
	struct wasm_import *im = &ex->m->imports[idx];
	struct wasm_type *t = &ex->m->types[im->typeidx];
	i32 args[8];
	int i, rc = 0;

	if (ex->sp < t->nparams)
		return -EINVAL;
	for (i = t->nparams - 1; i >= 0; i--)
		args[i] = ex->stack[--ex->sp];

	switch (im->wasi) {
	case WASI_FD_WRITE:
		rc = wasi_fd_write(ex->m, args[0], args[1], args[2], args[3]);
		break;
	case WASI_PROC_EXIT:
		rc = wasi_proc_exit(ex, args[0]);
		break;
	case WASI_FD_CLOSE:
		rc = 0;
		break;
	case WASI_FD_SEEK:
		rc = 0;
		break;
	case WASI_FD_FDSTAT_GET:
		rc = wasi_fd_fdstat_get(ex->m, args[0], args[1]);
		break;
	default:
		rc = -ENOSYS;
		break;
	}
	if (t->nresults)
		ex->stack[ex->sp++] = rc;
	return 0;
}

int wasm_run(const u8 *image, u32 size)
{
	struct wasm_mod m = { 0 };
	struct wasm_exec *ex;
	struct wasm_frame *f;
	int rc;

	ex = (struct wasm_exec *)WASM_GUEST_STATE;
	memset(ex, 0, sizeof(*ex));

	rc = wasm_parse(&m, image, size);
	if (rc)
		return rc;
	rc = wasm_init_mem(&m);
	if (rc)
		return rc;
	ex->m = &m;

	if (m.start_idx < 0 || (u32)m.start_idx < m.nimports) {
		rc = -ENOENT;	/* no _start */
		goto out;
	}

	/* enter _start */
	f = &ex->frames[ex->depth++];
	f->funcidx = m.start_idx - m.nimports;
	f->pc = 0;
	f->nlocals = m.funcs[f->funcidx].nlocals;
	memset(f->locals, 0, sizeof(f->locals));
	ex->exited = 0;

	while (ex->depth > 0 && !ex->exited) {
		struct wasm_func *fn;
		u8 op;
		i32 a, b;

		f = &ex->frames[ex->depth - 1];
		fn = &m.funcs[f->funcidx];
		if (f->pc >= fn->bodylen) {
			rc = -EPROTO;
			goto out;
		}
		op = fn->body[f->pc++];

		switch (op) {
		case 0x00:	/* unreachable */
			rc = -EFAULT;
			goto out;
		case 0x0b:	/* end */
			ex->depth--;
			break;
		case 0x0f:	/* return */
			ex->depth--;
			break;
		case 0x1a:	/* drop */
			ex->sp--;
			break;
		case 0x10: {	/* call */
			u32 idx = uleb(fn->body, fn->bodylen, &f->pc);
			u32 i;

			if (idx < m.nimports) {
				if (wasm_call_import(ex, idx)) {
					rc = -EINVAL;
					goto out;
				}
			} else {
				u32 fi = idx - m.nimports;
				struct wasm_type *t =
					&m.types[m.funcs[fi].typeidx];

				if (ex->depth >= WASM_CALL_DEPTH) {
					rc = -E2BIG;
					goto out;
				}
				if (ex->sp < t->nparams) {
					rc = -EINVAL;
					goto out;
				}
				for (i = t->nparams; i > 0; i--)
					ex->frames[ex->depth].locals[i - 1] =
						ex->stack[ex->sp - t->nparams + i - 1];
				ex->sp -= t->nparams;
				for (i = t->nparams; i < m.funcs[fi].nlocals; i++)
					ex->frames[ex->depth].locals[i] = 0;
				ex->frames[ex->depth].nlocals = m.funcs[fi].nlocals;
				ex->frames[ex->depth].funcidx = fi;
				ex->frames[ex->depth].pc = 0;
				ex->depth++;
			}
			break;
		}
		case 0x20: {	/* local.get */
			u32 i = uleb(fn->body, fn->bodylen, &f->pc);

			if (i < f->nlocals)
				ex->stack[ex->sp++] = f->locals[i];
			break;
		}
		case 0x21: {	/* local.set */
			u32 i = uleb(fn->body, fn->bodylen, &f->pc);

			if (i < f->nlocals)
				f->locals[i] = ex->stack[--ex->sp];
			break;
		}
		case 0x41:	/* i32.const */
			ex->stack[ex->sp++] = sleb(fn->body, fn->bodylen, &f->pc);
			break;
		case 0x28: {	/* i32.load */
			u32 a = uleb(fn->body, fn->bodylen, &f->pc);
			u32 o = uleb(fn->body, fn->bodylen, &f->pc);
			u32 addr = (u32)ex->stack[--ex->sp] + o;

			if (!gaddr_ok(&m, addr, 4)) {
				rc = -EFAULT;
				goto out;
			}
			ex->stack[ex->sp++] = (i32)gread32(&m, addr);
			break;
		}
		case 0x2d: {	/* i32.load8_u */
			u32 a = uleb(fn->body, fn->bodylen, &f->pc);
			u32 o = uleb(fn->body, fn->bodylen, &f->pc);
			u32 addr = (u32)ex->stack[--ex->sp] + o;

			if (!gaddr_ok(&m, addr, 1)) {
				rc = -EFAULT;
				goto out;
			}
			ex->stack[ex->sp++] = m.memory[addr];
			break;
		}
		case 0x36: {	/* i32.store */
			u32 a = uleb(fn->body, fn->bodylen, &f->pc);
			u32 o = uleb(fn->body, fn->bodylen, &f->pc);
			u32 addr = (u32)ex->stack[ex->sp - 2] + o;
			u32 val = (u32)ex->stack[ex->sp - 1];

			if (!gaddr_ok(&m, addr, 4)) {
				rc = -EFAULT;
				goto out;
			}
			gwrite32(&m, addr, val);
			ex->sp -= 2;
			break;
		}
		case 0x3a: {	/* i32.store8 */
			u32 a = uleb(fn->body, fn->bodylen, &f->pc);
			u32 o = uleb(fn->body, fn->bodylen, &f->pc);
			u32 addr = (u32)ex->stack[ex->sp - 2] + o;
			u32 val = (u32)ex->stack[ex->sp - 1];

			if (!gaddr_ok(&m, addr, 1)) {
				rc = -EFAULT;
				goto out;
			}
			m.memory[addr] = val;
			ex->sp -= 2;
			break;
		}
		case 0x45:	/* i32.eqz */
			ex->stack[ex->sp - 1] = ex->stack[ex->sp - 1] == 0;
			break;
		case 0x46:	/* i32.ne */
			b = ex->stack[--ex->sp];
			a = ex->stack[ex->sp - 1];
			ex->stack[ex->sp - 1] = a != b;
			break;
		case 0x47:	/* i32.lt_s */
			b = ex->stack[--ex->sp];
			a = ex->stack[ex->sp - 1];
			ex->stack[ex->sp - 1] = a < b;
			break;
		case 0x4a:	/* i32.gt_s */
			b = ex->stack[--ex->sp];
			a = ex->stack[ex->sp - 1];
			ex->stack[ex->sp - 1] = a > b;
			break;
		case 0x50:	/* i32.eq */
			b = ex->stack[--ex->sp];
			a = ex->stack[ex->sp - 1];
			ex->stack[ex->sp - 1] = a == b;
			break;
		case 0x6a:	/* i32.add */
			b = ex->stack[--ex->sp];
			ex->stack[ex->sp - 1] += b;
			break;
		case 0x6b:	/* i32.sub */
			b = ex->stack[--ex->sp];
			ex->stack[ex->sp - 1] -= b;
			break;
		case 0x6c:	/* i32.mul */
			b = ex->stack[--ex->sp];
			ex->stack[ex->sp - 1] *= b;
			break;
		case 0x02:	/* block: skip blocktype */
		case 0x03:	/* loop */
			if (fn->body[f->pc] == 0x40)
				f->pc++;
			else {
				i32 bt = sleb(fn->body, fn->bodylen, &f->pc);
				(void)bt;
			}
			break;
		case 0x04:	/* if */
			/* condition is on the stack; treat as always-taken
			 * block (no else support yet) */
			ex->sp--;
			if (fn->body[f->pc] == 0x40)
				f->pc++;
			else {
				i32 bt = sleb(fn->body, fn->bodylen, &f->pc);
				(void)bt;
			}
			break;
		case 0x0c:	/* br */
		case 0x0d:	/* br_if */
			{
				u32 l = uleb(fn->body, fn->bodylen, &f->pc);

				if (op == 0x0d && !ex->stack[--ex->sp])
					break;
				/* prototype: branches to a single forward
				 * label are not supported; treat as end */
				if (l == 0)
					ex->depth--;
			}
			break;
		default:
			rc = -ENOSYS;
			goto out;
		}
		if (ex->sp >= WASM_OP_STACK) {
			rc = -EOVERFLOW;
			goto out;
		}
	}

	rc = ex->exited ? (int)ex->exit_code : 0;
out:
	return rc;
}
