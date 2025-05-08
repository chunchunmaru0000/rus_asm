#include "gner.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void eeg(const char *msg, struct Inst *i) {
	fprintf(stderr, "%s:%ld:%ld %s\n", i->file, i->line, i->col, msg);
	exit(1);
}

struct Gner *new_gner(struct PList *is, enum Target t, uc debug) {
	struct Gner *g = malloc(sizeof(struct Gner));
	g->t = t;
	g->debug = debug;
	g->is = is;
	g->pos = 0;
	g->prol = new_blist(100);
	g->text = new_blist(100);

	g->pie = 0x400000;
	g->lps = new_plist(5);
	g->phs = new_plist(3);
	g->shs = new_plist(1);
	return g;
}

void gen_Linux_ELF_86_64_prolog(struct Gner *);
void gen_Linux_ELF_86_64_text(struct Gner *);
struct ELFPH *new_ph(int, int, uint64_t, uint64_t, uint64_t);
struct ELFH *new_elfh(struct Gner *, long, long, short, long, short);
struct Plov *new_label(struct Gner *, struct Inst *);

void gen(struct Gner *g) {
	switch (g->t) {
	case Linux_ELF_86_64:
		gen_Linux_ELF_86_64_prolog(g);
		gen_Linux_ELF_86_64_text(g);

		// is it should be here or do separate function like bze_prolog
		blat(g->prol, (uc *)g->elfh, sizeof(struct ELFH));
		for (int i = 0; i < g->phs->size; i++)
			blat(g->prol, plist_get(g->phs, i), sizeof(struct ELFPH));
		for (int i = 0; i < g->shs->size; i++)
			blat(g->prol, plist_get(g->shs, i), sizeof(struct ELFSH));
		break;
	case Linux_OBJ_86_64:
		break;
	case Windows_EXE_86_64:
		break;
	}
}

const uc ELFH_MAG[] = {0x7f, 0x45, 0x4c, 0x46};
const uc ELFH_INDENT[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uc ELFH_TYPE_EXEC[] = {0x02, 0x00};
const uc ELFH_TYPE_DYN[] = {0x03, 0x00}; // not sure about this
const uc ELFH_MACHINE_AMD_x86_64[] = {0x3e, 0x00};
const uc ELFH_FORMAT_VERSION[] = {0x01, 0x00, 0x00, 0x00}; // only possible
const uc ELFH_SHSTRNDX[] = {0x00, 0x00};
const uc ELFH_FLAGS[] = {0x00, 0x00, 0x00, 0x00};
const uc ELFH_EHSIZE[] = {0x40, 0x00}; // 64 for 64 bit
const uc ELFH_PHSIZE[] = {0x38, 0x00}; // 56 for 64 bit
const uc ELFH_SHSIZE[] = {0x40, 0x00}; // 64 for 64 bit

struct ELFH *new_elfh(struct Gner *g, long entrytoff, long phoff, short phn,
					  long shoff, short shn) {
	struct ELFH *h = malloc(sizeof(struct ELFH));

	memcpy(h->mag, ELFH_MAG, 4);
	h->elf_class = 2;	// 2 for 64 bit
	h->data = 1;		// little endian
	h->elf_version = 1; // only possible
	h->osabi = 0;		// UNIX System V ABI
	memcpy(h->indent, ELFH_INDENT, 8);

	memcpy(h->machine, ELFH_MACHINE_AMD_x86_64, 2);
	memcpy(h->format_version, ELFH_FORMAT_VERSION, 4);
	h->entry = entrytoff;
	memcpy(h->phoff, &phoff, sizeof(long));
	memcpy(h->shoff, &shoff, sizeof(long));
	memcpy(h->flags, ELFH_FLAGS, 4);
	memcpy(h->ehsize, ELFH_EHSIZE, 2);
	memcpy(h->phsize, ELFH_PHSIZE, 2);
	memcpy(h->phnum, &phn, 2);
	memcpy(h->shsize, ELFH_SHSIZE, 2);
	memcpy(h->shnum, &shn, 2);
	memcpy(h->shstrndx, ELFH_SHSTRNDX, 2);

	switch (g->t) {
	case Linux_ELF_86_64:
		memcpy(h->type, ELFH_TYPE_EXEC, 2);
		break;
	case Linux_OBJ_86_64:
		memcpy(h->type, ELFH_TYPE_DYN, 2);
		break;
	default:
		eeg("НЕ ДОСТУПНАЯ ЦЕЛЬ КОМПИЛЯЦИИ НА ДАННЫЙ МОМЕНТ", g->is->st[0]);
	}
	return h;
}

struct ELFPH *new_ph(int t, int flags, uint64_t off, uint64_t addr,
					 uint64_t sz) {
	struct ELFPH *ph = malloc(sizeof(struct ELFPH));
	ph->type = t;
	ph->flags = flags;
	ph->offset = off;
	ph->vaddr = addr;
	ph->paddr = addr;
	ph->filesz = sz;
	ph->memsz = sz;
	ph->align = 0x1000;

	return ph;
}

struct Plov *new_label(struct Gner *g, struct Inst *in) {
	struct Plov *p = malloc(sizeof(struct Plov));

	int segment_place = g->phs->size - 1;
	p->l = ((struct Token *)plist_get(in->os, 0))->view;
	p->si = segment_place;
	p->us = new_plist(4);
	// p->a = g->pie; // + first ph memsz;

	return p;
}

// machine codes
const uc MLESYSCALL[] = {0x0f, 0x05};

void gen_Linux_ELF_86_64_prolog(struct Gner *g) {
	int *flags;
	struct Inst *in;
	struct ELFPH *ph;
	// struct ELFSH *sh;

	for (long i = 0; i < g->is->size; i++) {
		in = plist_get(g->is, i);

		if (in->code == ISEGMENT) {
			flags = plist_get(in->os, 0);
			// type =   1,  flags, offset,     adress,     size
			ph = new_ph(1, *flags, 0x7366666f, 0x72646461, 0x657a6973);
			plist_add(g->phs, ph);
		} else if (in->code == ILABEL || in->code == ILET)
			plist_add(g->lps, new_label(g, in));
	}
	// entry = 0 but will be editet
	g->elfh = new_elfh(g, 0, 0x40, g->phs->size, 0x00, g->shs->size);
}

void *alloc_len(uint32_t len, uint32_t *buf_len) {
	*buf_len = len;
	return malloc(len);
}

void alloc_cpy(void **buf, uint32_t *buf_len, uint32_t alc_len, void *data) {
	*buf = alloc_len(alc_len, buf_len);
	memcpy(*buf, data, alc_len);
}

void alloc_cpy_int(void **buf, uint32_t *buf_len, uint32_t alc_len,
				   uint64_t data) {
	*buf = alloc_len(alc_len, buf_len);
	memcpy(*buf, &data, alc_len);
}

void cpy_len(uc *buf, long value, int len) {
	long tmp = value;
	memcpy(buf, &tmp, len);
}

const char *UNKNOWN_LABEL = "Неизвестная метка [%s]";
struct Plov *find_label(struct Gner *g, char *s) {
	struct Plov *l;
	for (long i = 0; i < g->lps->size; i++) {
		l = g->lps->st[i];
		if (sc(l->l, s))
			return l;
	}
	char *err;
	asprintf(&err, UNKNOWN_LABEL, s);
	eeg(err, plist_get(g->is, g->pos));
	return 0;
}

#define REL_SIZE 4

struct Usage *new_usage(uint64_t place, enum UT type) {
	struct Usage *u = malloc(sizeof(struct Usage));
	u->place = place;
	u->type = type;
	return u;
}

const char *WRONG_FST_OPER = "НЕДОСТУПНЫЙ ТИП ПЕРВОГО ОПЕРАНДА НА ДАНЫЙ МОМЕНТ";
const char *WRONG_FST_OPER_INT =
	"НЕДОСТУПНЫЙ ТИП ПЕРВОГО ОПЕРАНДА, ЧИСЛО НЕ МОЖЕТ БЫТЬ ПЕРВЫМ ОПЕРАНДОМ "
	"ДЛЯ ДАННОЙ КОМАНДЫ";
const char *WRONG_SND_OPER = "НЕДОСТУПНЫЙ ТИП ВТОРОГО ОПЕРАНДА НА ДАНЫЙ МОМЕНТ";

// sizes
const char *WRONG_FST_OPER_REG_DWORD =
	"Неверный регистр, для значения размера <чбайт> 4 байта";
const char *WRONG_FST_OPER_REG_QWORD =
	"Неверный регистр, для значения размера <вбайт> 8 байт";
const char *WRONG_SND_OPER_SIZE = "Неверный размер второго операнда";
const char *WRONG_FPN_OP_SIZE = "Недопустимый размер для числа с плавающей "
								"точкой, минимальный - <чбайт> 4 байта, "
								"майсимальный - <вбайт> 8 байт";
const char *REGS_SIZES_NOT_MATCH = "Размеры регистров не совпадают";
const char *MEM_REG_SIZES_NOT_MATCH = "Размеры памяти и регистра не совпадают";
const char *REG_MEM_IMM_SIZES_NOT_MATCH =
	"Размеры регистра или памяти и значения не совпадают";

uint64_t reg2reg(enum RegCode l, enum RegCode r, int size) {
	uint64_t v = 0;
	enum RegCode lmin, rmin;

	switch (size) {
	case QWORD:
		lmin = l >= R_RAX && l <= R_RDI ? R_RAX : R_R8;
		rmin = r >= R_RAX && r <= R_RDI ? R_RAX : R_R8;
		v = (l - lmin) + (r - rmin) * 8;
		break;
	case DWORD:

		break;
	case WORD:
		break;
	case BYTE:
		break;
	}
	return v;
}

uc get_REX(struct Oper *l, struct Oper *r) {
	uc res = 0x00;
	return res;
}

#define is_reg(o) ((o)->code == OREG)
#define is_mem(o) ((o)->code == OMEM_REG)
#define is_imm(o) ((o)->code == OINT || (o)->code == OFPN)
#define is_seg(o) ((o)->code == OSREG)
#define is_r8(o) ((o)->rcode >= R_AH && (o)->rcode <= R15B)
#define is_r16(o) ((o)->rcode >= R_AX && (o)->rcode <= R_R15W)
#define is_r32(o) ((o)->rcode >= R_EAX && (o)->rcode <= R_R15D)
#define is_r64(o) ((o)->rcode >= R_RAX && (o)->rcode <= R_R15)
#define is_8(o) ((o)->sz == BYTE)
#define is_16(o) ((o)->sz == WORD)
#define is_32(o) ((o)->sz == DWORD)
#define is_64(o) ((o)->sz == QWORD)
#define is_al(o) ((o)->code = OREG && (o)->rcode == R_AL)
#define is_rA(o)                                                               \
	((o)->code = OREG && ((o)->rcode == R_AX || (o)->rcode == R_EAX ||         \
						  (o)->rcode == R_RAX))
#define is_rm(o) (is_reg((o)) || is_mem((o)))
#define is_moffs(o) ((o)->code == OMOFFS)
#define is_rex()

// http://ref.x86asm.net/coder64.html
// add r8w, 128 does 6641 81c0 8000,adress prefix > so 16-bit prefix > 64-bit

// - prefix 67 adress prefix does give you a way of taking value from adress of
// 		32 bit register like [r8d]
// - prefix 66 is used with all 16 bit ops like add ax, bx or
//		add word [rax], 255
// - prefix REX are prefixes
enum OpsCode get_ops_code(struct Inst *in, uint64_t *prf, uint32_t *prf_len) {
	*prf = 0;
	*prf_len = 0;
	struct Oper *l, *r; //, *t, *f;
	enum OpsCode code = OPC_INVALID;
	uc rex;

	switch (in->code) {
	case IADD:
	case IOR:
	case IADC:
	case ISBB:
	case IAND:
	case ISUB:
	case IXOR:
	case ICMP:
	case ITEST:
		l = plist_get(in->os, 0);
		r = plist_get(in->os, 1);
		// RM_8__R_8,
		// RM_16_32_64__R16_32_64,
		if (is_rm(l) && is_reg(r)) {
			if (l->sz != r->sz)
				eeg(REGS_SIZES_NOT_MATCH, in);
			code = is_8(l) ? RM_8__R_8 : RM_16_32_64__R16_32_64;
			// R_8__RM_8,
			// R_16_32_64__RM_16_32_64,
		} else if (is_reg(l) && is_rm(r)) {
			if (l->sz != r->sz)
				eeg(MEM_REG_SIZES_NOT_MATCH, in);
			code = is_8(l) ? R_8__RM_8 : R_16_32_64__RM_16_32_64;
		} else if (is_rm(l) && is_imm(r)) {
			if (l->sz != r->sz && !(is_64(l) && is_32(r)))
				eeg(REG_MEM_IMM_SIZES_NOT_MATCH, in);
			// 	AL__IMM_8,
			if (is_al(l))
				code = AL__IMM_8;
			// 	RAX__IMM_16_32,
			else if (is_rA(l))
				code = RAX__IMM_16_32;
			// 	RM_8__IMM_8,
			else if (is_8(l))
				code = RM_8__IMM_8;
			// 	RM_16_32_64__IMM_8,
			// 	RM_16_32_64__IMM_16_32,
			else if (is_8(r))
				code = RM_16_32_64__IMM_8;
			else
				code = RM_16_32_64__IMM_16_32;
		}
		// PREFIXES ASSIGN goes in the reverse order
		// because of little endian

		// {lock} fs repne scas word [edi] ->
		// 		{f0} [64 fs][f2 scas] [67 is_r32(r)][66 is_16(r)] af
		// mov word[r8d], 255 -> 67 6641 c700 ff00

		// REX prefixes
		if (some) {
			rex = 0b01000000;

			*prf_len++;
			*prf = (*prf << 8) + rex;
		}
		// 66 16-bit Operand-size OVERRIRE prefix
		if (!is_seg(l) && is_16(l)) {
			*prf_len++;
			*prf = (*prf << 8) + 0x66;
		}
		// 67 Address-size OVERRIRE prefix, when adress 32-bit like [eax]
		if (is_mem(l) && is_r32(l)) {
			*prf_len++;
			*prf = (*prf << 8) + 0x67;
		}
		// here for example in mov word[r8d], 255
		// rex for r8d
		// 16-bit mem override
		// 32-bit adress-size
		break;
	case IMOV:
		l = plist_get(in->os, 0);
		r = plist_get(in->os, 1);
		// RM_8__R_8,
		// RM_16_32_64__R16_32_64,
		if (is_rm(l) && is_reg(r)) {
			if (l->sz != r->sz)
				eeg(REGS_SIZES_NOT_MATCH, in);
			code = is_8(l) ? RM_8__R_8 : RM_16_32_64__R16_32_64;
			// R_8__RM_8,
			// R_16_32_64__RM_16_32_64,
		} else if (is_reg(l) && is_rm(r)) {
			if (l->sz != r->sz)
				eeg(MEM_REG_SIZES_NOT_MATCH, in);
			code = is_8(l) ? R_8__RM_8 : R_16_32_64__RM_16_32_64;
			// M_16__SREG,
			// R_16_32_64__SREG,
		} else if (is_seg(r)) {
			if (is_mem(l) && is_16(l))
				code = M_16__SREG;
			else if (is_reg(l) && !is_8(l))
				code = R_16_32_64__SREG;
			// SREG__RM_16,
		} else if (is_seg(l) && is_16(r) && is_rm(r))
			code = SREG__RM_16;
		// AL__MOFFS_8,
		else if (is_al(l) && is_moffs(r) && is_8(r))
			code = AL__MOFFS_8;
		// RAX__MOFFS_16_32_64,
		else if (is_rA(l) && is_moffs(r) && !is_8(r))
			code = RAX__MOFFS_16_32_64;
		// MOFFS_8__AL,
		else if (is_moffs(l) && is_8(l) && is_al(r))
			code = MOFFS_8__AL;
		// MOFFS_16_32_64__RAX,
		else if (is_moffs(l) && !is_8(l) && is_rA(r))
			code = MOFFS_16_32_64__RAX;
		// M_8__M_8,
		// M_16_32_64__M_16_32_64,
		else if (is_mem(l) && is_mem(r)) {
			if (is_8(l) && is_8(r))
				code = M_8__M_8;
			if (!is_8(l) && !is_8(r))
				code = M_16_32_64__M_16_32_64;
		} else if (is_imm(r)) {
			// R_8__IMM_8,
			// R_16_32_64__IMM_16_32_64,
			if (is_reg(l)) {
				if (is_8(l) && is_8(r))
					code = R_8__IMM_8;
				else if (!is_8(l) && !is_8(r))
					code = R_16_32_64__IMM_16_32_64;
				// RM_8__IMM_8,
				// RM_16_32_64__IMM_16_32,
			} else if (is_mem(l)) {
				if (is_8(l) && is_8(r))
					code = RM_8__IMM_8;
				else if (!is_8(l) && is_16(r) && is_32(r))
					code = RM_16_32_64__IMM_16_32;
			}
		}
		break;
	default:
		eeg("нет пока а вот", in);
	}

	if (code == OPC_INVALID)
		eeg("а вот вот да", in);
	return code;
}

// PREFIXES ASSIGN goes in the reverse order
// because of little endian
void get_prefs(uint64_t *prf, uint32_t *prf_len) {
	// TODO: VEX/EVEX prefixes
	prf_len++;
	// REX prefixes
	prf_len++;
	// 66 16-bit Operand-size override prefix | Precision-size override prefix
	prf_len++;
	// 67 Address-size override prefix, when adress 32-bit like [eax] not [rax]
	prf_len++;
	// 			2E 36 3E 26 Null Prefix in 64-bit Mode
	// F3 Repeat String Operation Prefix | Scalar Single-precision Prefix
	if (0)
		prf_len++;
	// F2 Repeat String Operation Prefix | Scalar Double-precision Prefix
	if (0)
		prf_len++;
	// 64 65 FS GS Segment Override
	// TODO: when do scas or other do something like FS_M_16__AX
	if (0)
		prf_len++;
	// F0 lock

	// lock fs repne scas word [edi] -> f0 64f2 6766 af
	// mov word[r8d], 255 -> 67 6641 c700 ff00
}

void get_cmd(enum OpsCode opsCode, struct Inst *in, uint64_t *cmd,
			 uint32_t *cmd_len) {
	// first do registers encoding
	// then do cmd op code
	switch (opsCode) {
	default:
		eeg("эээ че за инструкция", in);
	}
}

void get_data(enum OpsCode opsCode, struct Inst *in, uint64_t *data,
			  uint32_t *data_len) {
	switch (opsCode) {
	default:
		*data = 0;
		*data_len = 0;
	}
}

void gen_Linux_ELF_86_64_text(struct Gner *g) {
	long i, j, last_text_sz;
	uc *ibuff;
	void **ibufp = (void **)&ibuff;
	uint64_t cmd, data, prf;
	uint32_t cmd_len, data_len, prf_len, buf_len, *blp = &buf_len;
	// long buf_len, *blp = &buf_len; // buf len ptr
	int all_h_sz = sizeof(struct ELFH) + g->phs->size * sizeof(struct ELFPH);
	enum ICode code;
	enum OpsCode opsCode;
	// take
	struct Inst *in;
	struct Token *tok;
	struct Oper * or, *ol;
	struct BList *data_bl;
	struct ELFPH *ph, *phl;
	// make
	struct Plov *l;
	struct Usage *usage;
	long phs_counter = 0, phs_cur_sz;
	// struct ELFSH *sh;

	for (i = 0; i < g->is->size; i++) {
		in = plist_get(g->is, i);
		g->pos = i;
		buf_len = 0;
		code = in->code;

		if (code == IADD) {
			opsCode = get_ops_code(in, &prf, &prf_len);

			// get_prefs(opsCode, &prf, &prf_len);
			get_cmd(opsCode, in, &cmd, &cmd_len);
			get_data(opsCode, in, &data, &data_len);

			ibuff = alloc_len(prf_len + cmd_len + data_len, blp);

			cpy_len(ibuff, prf, prf_len);
			cpy_len(ibuff + prf_len, cmd, cmd_len);
			cpy_len(ibuff + prf_len + cmd_len, data, data_len);

			if (buf_len) {
				blat(g->text, ibuff, buf_len);
				phs_cur_sz += buf_len;
				free(ibuff);
			}
			continue;
		} else if (code == IMOV) {
			ol = plist_get(in->os, 0);
			or = plist_get(in->os, 1);
			switch (ol->code) {
			case OREG:
				switch (or->code) {
				case OREG:
					if (ol->sz != or->sz)
						eeg(REGS_SIZES_NOT_MATCH, in);
					cmd = reg2reg(ol->rcode, or->rcode, ol->sz);
					switch (ol->sz) {
					case QWORD:
						cmd_len = 3;
						cmd = (cmd << 16) + 0xc08948;
						break;
					case DWORD:
						break;
					case WORD:
						break;
					case BYTE:
						break;
					}
					ibuff = alloc_len(cmd_len, blp);
					cpy_len(ibuff, cmd, cmd_len);
					break;
				case OINT:
				case OFPN:
				case OREL:
					if (or->sz == DWORD) {
						if (ol->rcode >= R_EAX && ol->rcode <= R_EDI) {
							cmd_len = 1;
							cmd = 0xb8 + ol->rcode - R_EAX;
						} else if (ol->rcode >= R_R8D && ol->rcode <= R_R15D) {
							cmd_len = 2;
							cmd = ((0xb8 + ol->rcode - R_R8D) << 8) + 0x41;
						} else if (ol->rcode >= R_RAX && ol->rcode <= R_RDI) {
							cmd_len = 3;
							cmd = ((0xc0 + ol->rcode - R_RAX) << 16) + 0xc748;
						} else if (ol->rcode >= R_R8 && ol->rcode <= R_R15) {
							cmd_len = 3;
							cmd = ((0xc0 + ol->rcode - R_R8) << 16) + 0xc749;
						} else
							eeg(WRONG_FST_OPER_REG_DWORD, in);
					} else if (or->sz == QWORD) {
						if (ol->rcode >= R_RAX && ol->rcode <= R_RDI) {
							cmd_len = 2;
							cmd = ((0xb8 + ol->rcode - R_RAX) << 8) + 0x48;
						} else if (ol->rcode >= R_R8 && ol->rcode <= R_R15) {
							cmd_len = 2;
							cmd = ((0xb8 + ol->rcode - R_R8) << 8) + 0x49;
						} else
							eeg(WRONG_FST_OPER_REG_QWORD, in);
					} else
						eeg(WRONG_SND_OPER_SIZE, in);

					tok = or->t;
					if (or->code == OINT)
						data = tok->number;
					else if (or->code == OFPN) {
						if (or->sz == 4)
							data = (float)tok->fpn;
						else if (or->sz == 8)
							data = tok->fpn;
						else
							eeg(WRONG_FPN_OP_SIZE, in);
					} else {
						data = 0x766f6d6c;
						l = find_label(g, tok->view);
						usage = new_usage((uint64_t)(g->text->size) + cmd_len,
										  ADDR);
						plist_add(l->us, usage);
					}

					ibuff = alloc_len(cmd_len + or->sz, blp);
					cpy_len(ibuff, cmd, cmd_len);
					cpy_len(ibuff + cmd_len, data, or->sz);
					break;
				default:
					if (g->debug)
						printf("or->code = %d\t", or->code);
					eeg(WRONG_SND_OPER, in);
				}
				break;
			default:
				if (g->debug)
					printf("ol->code = %d\t", ol->code);
				eeg(WRONG_FST_OPER, in);
			}
			if (buf_len) {
				blat(g->text, ibuff, buf_len);
				phs_cur_sz += buf_len;
				free(ibuff);
			}
			continue;
		}

		switch (code) {
		//  TODO: near jmp
		case IJMP:
			data = 0;
			ol = plist_get(in->os, 0);
			//  TODO: jmp for qword
			if (ol->sz == DWORD) {
				cmd = 0xe9;
				cmd_len = 1;

				if (ol->code == OREL) {
					l = find_label(g, ol->t->view);
					// text size + 1 is place in text to where need to put rel
					usage = new_usage((uint64_t)(g->text->size) + 1, REL_ADDR);
					plist_add(l->us, usage);
					data = 0x706d6a;
				} else if (ol->code == OINT)
					data = ol->t->number;
				else if (ol->code == OFPN) {
					if (or->sz == 4)
						data = (float)ol->t->fpn;
					else if (ol->sz == 8)
						data = ol->t->fpn;
					else
						eeg(WRONG_FPN_OP_SIZE, in);
				}
				//  TODO: jmp for reg
				else {
					if (g->debug)
						printf("ol->code = %d\t", ol->code);
					eeg(WRONG_FST_OPER, in);
				}
			}
			// TODO: macro oper cmd cmd_len data &ibuff blp
			ibuff = alloc_len(cmd_len + ol->sz, blp);
			cpy_len(ibuff, cmd, cmd_len);
			cpy_len(ibuff + cmd_len, data, ol->sz);
			break;
		case ILABEL:
			tok = plist_get(in->os, 0);
			l = find_label(g, tok->view);

			ph = plist_get(g->phs, phs_counter - 1);
			l->a = phs_cur_sz + ph->vaddr;
			l->ra = g->text->size;
			if (g->debug)
				printf("label[%s]\t[0x%08lx]\n", l->l, l->a);
			break;
		case IDATA:
			data_bl = plist_get(in->os, 0);
			alloc_cpy(ibufp, blp, data_bl->size, data_bl->st);
			break;
		case ILET:
			tok = plist_get(in->os, 0);
			// printf("try find [%s], i = %ld, code = %d\n", tok->view, i,
			// code);
			l = find_label(g, tok->view);

			ph = plist_get(g->phs, phs_counter - 1);
			l->a = phs_cur_sz + ph->vaddr;
			l->ra = g->text->size;
			if (g->debug)
				printf(" var [%s]\t[0x%08lx]\n", l->l, l->a);

			data_bl = plist_get(in->os, 1);
			alloc_cpy(ibufp, blp, data_bl->size, data_bl->st);
			break;
		case ISYSCALL:
			alloc_cpy(ibufp, blp, 2, (void *)MLESYSCALL);
			break;
		case INOP:
			alloc_cpy_int(ibufp, blp, 1, 0x90);
			break;
		case ISEGMENT:
			if (phs_counter == 0)
				phs_cur_sz = all_h_sz;
			else
				phs_cur_sz = 0;
			/*
			 * 0 offset = 0
			 * 0 addr = 0 + pie
			 * 0 size = all p h + elf h + segment size
			 * i offset = last segment size + last segment offset
			 * i addr = offset + align * (i - 1) + pie
			 * i size = segment size
			 */
			//  do segment adress and offset and size for prev segment
			ph = g->phs->st[phs_counter];
			if (phs_counter == 0) {
				ph->offset = 0;		  // 0 offset
				ph->vaddr = g->pie;	  // 0 addr
				ph->memsz = all_h_sz; // + segment size // 0 size
			} else {
				phl = g->phs->st[phs_counter - 1];
				// because for first its size of all data that is only first
				// data for the moment
				if (phs_counter == 1)
					phl->memsz += g->text->size; // 0 size
				else
					phl->memsz = g->text->size - last_text_sz; // i size
				phl->filesz = phl->memsz;

				ph->offset = phl->memsz + phl->offset; // i offset
				ph->vaddr =
					ph->offset + ph->align * (phs_counter) + g->pie; // i addr
			}
			ph->filesz = ph->memsz;
			ph->paddr = ph->vaddr;
			phs_counter++;
			last_text_sz = g->text->size;
			break;
		case IEOI:
			phl = g->phs->st[phs_counter - 1];
			phl->memsz = g->phs->size == 1 ? phl->memsz + g->text->size
										   : g->text->size - last_text_sz;
			phl->filesz = phl->memsz;
			break;
			//	default:
			//		eeg("НЕИЗВЕСТНАЯ ИНСТРУКЦИЯ", in);
		}
		if (buf_len) {
			phs_cur_sz += buf_len;
			blat(g->text, ibuff, buf_len);
			free(ibuff);
		}
	}

	phs_counter = 0;
	for (i = 0; i < g->is->size; i++) {
		in = plist_get(g->is, i);

		if (in->code == ILABEL || in->code == ILET) {
			tok = plist_get(in->os,
							0); // in both cases name is first opperand
			l = find_label(g, tok->view);
			for (j = 0; j < l->us->size; j++) {
				usage = plist_get(l->us, j);

				void *textptr = ((uc *)g->text->st) + usage->place;
				if (usage->type == ADDR) {
					memcpy(textptr, &l->a, REL_SIZE);
				} else if (usage->type == REL_ADDR) {
					long text_pos = l->ra - usage->place - REL_SIZE;
					memcpy(textptr, &text_pos, REL_SIZE);
				}
			}
		} else if (in->code == IENTRY) {
			tok = in->os->st[0];
			l = find_label(g, tok->view);
			g->elfh->entry = l->a;
		} else if (in->code == ISEGMENT)
			phs_counter++;
	}
}
