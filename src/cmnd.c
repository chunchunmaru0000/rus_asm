#include "cmnd.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

const char *const WRONG_FST_OPER =
	"НЕДОСТУПНЫЙ ТИП ПЕРВОГО ОПЕРАНДА НА ДАНЫЙ МОМЕНТ";
const char *const WRONG_FST_OPER_INT =
	"НЕДОСТУПНЫЙ ТИП ПЕРВОГО ОПЕРАНДА, ЧИСЛО НЕ МОЖЕТ БЫТЬ ПЕРВЫМ ОПЕРАНДОМ "
	"ДЛЯ ДАННОЙ КОМАНДЫ";
const char *const WRONG_SND_OPER =
	"НЕДОСТУПНЫЙ ТИП ВТОРОГО ОПЕРАНДА НА ДАНЫЙ МОМЕНТ";
const char *const WRONG_FST_OPER_REG_DWORD =
	"Неверный регистр, для значения размера <чбайт> 4 байта";
const char *const WRONG_FST_OPER_REG_QWORD =
	"Неверный регистр, для значения размера <вбайт> 8 байт";
const char *const WRONG_SND_OPER_SIZE = "Неверный размер второго операнда.";
const char *const WRONG_FPN_OP_SIZE =
	"Недопустимый размер для числа с плавающей "
	"точкой, минимальный - <чбайт> 4 байта, "
	"майсимальный - <вбайт> 8 байт";
const char *const REGS_SIZES_NOT_MATCH =
	"Размеры регистров или памяти не совпадают.";
const char *const MEM_REG_SIZES_NOT_MATCH =
	"Размеры памяти и регистра не совпадают.";
const char *const REG_MEM_IMM_SIZES_NOT_MATCH =
	"Размеры регистра или памяти и значения не совпадают.";
const char *const WRONG_FPN_SZ =
	"Не может вещественное число быть размером меньше, чем 4 байта.";
const char *const MEM_IMM_SIZE_QWORD =
	"Инструкция данного типа не может иметь оба значения с размерами вбайт, "
	"правое выражение должно быть размером чбайт для данного левого выражения.";
const char *const UNKNOWN_LABEL = "Метка не была найдена [%s]";
const char *const WRONG_ADDR_SZ =
	"Не может адрес метки быть размером меньше, чем 4 байта.";
const char *const OPS_CODE_INVALID =
	"Не удалось определисть типы выражений для инструкции, возможно она была "
	"неверной.";

void get_zero_ops_code(struct Ipcd *);
void get_one_ops_code(struct Ipcd *);
void get_two_ops_code(struct Ipcd *);

void get_ops_code(struct Ipcd *i) {
	if (i->in->os->size == 0)
		get_zero_ops_code(i);
	else if (i->in->os->size == 1)
		get_one_ops_code(i);
	else if (i->in->os->size == 2)
		get_two_ops_code(i);
	else
		eeg("йцук\n", i->in);

	if (0b00000010 & i->debug) {
		printf("### команда %ld байт: ", i->cmd->size);
		blist_print(i->cmd);
		printf("### данные  %ld байт: ", i->data->size);
		blist_print(i->data);
	}
}

const struct Cmnd *get_cmnd(struct Ipcd *, enum OpsCode);

enum OpsCode get_one_opscode(struct Inst *in);
void get_one_ops_prefs(struct Ipcd *, enum OpsCode);
void fill_one_ops_cmd_and_data(struct Ipcd *);

enum OpsCode get_two_opscode(struct Inst *in);
void get_two_ops_prefs(struct Ipcd *, enum OpsCode);
void fill_two_ops_cmd_and_data(struct Ipcd *);

void get_one_ops_code(struct Ipcd *i) {
	enum OpsCode code = get_one_opscode(i->in);
	i->c = get_cmnd(i, code);
	get_one_ops_prefs(i, code);
	fill_one_ops_cmd_and_data(i);
}

void get_two_ops_code(struct Ipcd *i) {
	enum OpsCode code = get_two_opscode(i->in);
	i->c = get_cmnd(i, code);
	get_two_ops_prefs(i, code);
	fill_two_ops_cmd_and_data(i);
}

struct Defn *new_not_plov(char *view, uint64_t place, enum UT ut) {
	struct Defn *np = malloc(sizeof(struct Defn));
	np->view = view;
	np->value = new_usage(place, ut);
	return np;
}

#define declare_two_ops(in, l, r)                                              \
	do {                                                                       \
		(l) = plist_get(in->os, 0);                                            \
		(r) = plist_get(in->os, 1);                                            \
	} while (0);
#define get_first_o(in) (plist_get((in)->os, 0))
// doesnt changes ModR/M byte
void add_sib(struct BList *cmd, struct Oper *o) {
	uc sib = o->scale << 6;
	sib += get_reg_field(o->index) << 3;
	sib += get_reg_field(o->base);
	blist_add(cmd, sib);
}

void add_disp(struct Ipcd *i, struct Oper *o, uc bytes) {
	if (o->disp_is_rel_flag) {
		// TODO: rel8 also can exist
		if (o->sz < DWORD)
			eeg(WRONG_ADDR_SZ, i->in);

		struct Defn *np = new_not_plov(o->rel_view, i->data->size, REL_ADDR);
		plist_add(i->not_plovs, np);
		uint64_t some_value = 0x72656c; // rel
		blat(i->data, (uc *)&some_value, o->sz);
	} else
		blat(i->data, (uc *)&o->disp, bytes);
}

void add_imm_data(struct Ipcd *i, struct Oper *o) {
	if (o->code == OREL) {
		if (o->sz < DWORD)
			eeg(WRONG_ADDR_SZ, i->in);

		enum UT ut = is_rel(i->c->opsc) ? REL_ADDR : ADDR;
		struct Defn *np = new_not_plov(o->t->view, i->data->size, ut);
		plist_add(i->not_plovs, np);
		uint64_t some_value = 0x72656c; // rel
		blat(i->data, (uc *)&some_value, o->sz);
	} else if (o->code == OINT)
		blat(i->data, (uc *)&o->t->number, o->sz);
	else if (o->code == OFPN) {
		if (o->sz == DWORD) {
			float value = o->t->fpn;
			blat(i->data, (uc *)&value, DWORD);
		} else if (o->sz == QWORD)
			blat(i->data, (uc *)&o->t->fpn, QWORD);
		else
			eeg(WRONG_FPN_SZ, i->in);
	}
}

void add_mem(struct Ipcd *i, struct Oper *m) {
	if (m->mod == MOD_MEM) {
		// REMEMBER: mod = 00, rm = 101 == [RIP+disp32]
		if (m->rm == R_RBP)
			add_disp(i, m, DWORD);
		else if (m->rm == R_RSP) {
			add_sib(i->cmd, m);
			// mod = 00, rm = 100, base = 101 == no base register
			// and REMEMBER: do disp 32 although mod is 00
			if (m->base == R_RBP)
				add_disp(i, m, DWORD);
		}
		// else nothing because doesnt care about modrm byte
	} else if (m->mod == MOD_MEM_D8) {
		if (m->rm == R_RSP)
			add_sib(i->cmd, m);
		add_disp(i, m, BYTE);
	} else { // l->mod == MOD_MEM_D32
		if (m->rm == R_RSP)
			add_sib(i->cmd, m);
		add_disp(i, m, DWORD);
	}
}

void fill_two_ops_cmd_and_data(struct Ipcd *i) {
	struct Oper *l, *r;
	const struct Cmnd *c = i->c;
	declare_two_ops(i->in, l, r);
	uc modrm = 0;

	blat(i->cmd, (uc *)c->cmd, c->len);
	// o Register/ Opcode Field
	if (c->o == NOT_FIELD) {
		//   0. NOT_FIELD just op code
		if (is_imm(r))
			add_imm_data(i, r);
	} else if (c->o == NUM_FIELD) {
		//   1. NUM_FIELD The value of the opcode extension values from 0
		//   through 7
		//   - like ModR/M byte where Reg field is for o_num
		//   - - primary used with imm or ?const regs?
		//   - this "ModR/M" byte also have mod and if its just reg and imm then
		//   - - mod = 11 and R/M field means just reg code
		//   - if mod != 11 then it behaves as
		//   - - just usual mod and R/M fields with SIB if needed
		if (is_imm(r)) {
			modrm = (c->o_num) << 3; // r
			modrm += l->mod << 6;
			modrm += get_reg_field(l->rm);
			blist_add(i->cmd, modrm);
			if (is_mem(l))
				add_mem(i, l);
			add_imm_data(i, r);
		} else
			eeg("только числа пока", i->in);
	} else if (c->o == REG_FIELD) {
		//   2. REG_FIELD r indicates that the ModR/M byte contains a register
		//   - operand and an r/m operand. 00 ADD
		//   - - primary used with
		//   - - - r__rm or rm__r, lea uses r__m
		//   - - - also sreg__rm or rm__sreg
		//   - - - also xmm, CRn, DRn and maybe more shit
		if (is_rm__r(c->opsc)) {
			modrm += l->mod << 6;				// mod
			modrm += get_reg_field(r->rm) << 3; // r
			modrm += get_reg_field(l->rm);		// rm
			blist_add(i->cmd, modrm);
			if (is_mem(l))
				add_mem(i, l);
		} else if (is_r__rm(c->opsc)) {
			modrm += r->mod << 6;				// mod
			modrm += get_reg_field(l->rm) << 3; // r
			modrm += get_reg_field(r->rm);		// rm
			blist_add(i->cmd, modrm);
			if (is_mem(r))
				add_mem(i, r);
		} else
			eeg("только р__рм и рм__р пока", i->in);
	} else if (c->o == PLUS_REGF) {
		//   3. PLUS_REGF When just op code + reg code
		//   - BSWAP, PUSH r64/16, POP r64/16, XCHG r16/32/64 rAX
		//   - B0+r MOV r8 imm8, B8+r MOV r16/32/64 imm16/32/64
		*(i->cmd->st + i->cmd->size - 1) += get_reg_field(l->rm);
		if (i->in->code == IMOV)
			add_imm_data(i, r);
	} else
		eeg("че не так то", i->in);
}

const enum OpsCode RM_L[] = {RM_8__R_8, RM_16_32_64__R_16_32_64, RM_8__IMM_8,
							 RM_16_32_64__IMM_16_32, RM_16_32_64__IMM_8};
const enum OpsCode RM_R[] = {R_8__RM_8, R_16_32_64__RM_16_32_64, SREG__RM_16};
const enum OpsCode IMM_R[] = {AL__IMM_8,
							  RAX__IMM_16_32,
							  RM_8__IMM_8,
							  RM_16_32_64__IMM_16_32,
							  RM_16_32_64__IMM_8,
							  R_8__IMM_8,
							  R_16_32_64__IMM_16_32_64};

int is_rm_l(enum OpsCode c) {
	for (size_t i = 0; i < lenofarr(RM_L); i++)
		if (c == RM_L[i])
			return 1;
	return 0;
}
int is_rm_r(enum OpsCode c) {
	for (size_t i = 0; i < lenofarr(RM_R); i++)
		if (c == RM_R[i])
			return 1;
	return 0;
}
int is_imm_r(enum OpsCode c) {
	for (size_t i = 0; i < lenofarr(IMM_R); i++)
		if (c == IMM_R[i])
			return 1;
	return 0;
}

// http://ref.x86asm.net/coder64.html

// add r8w, 128 does 6641 81c0 8000,adress prefix > so 16-bit prefix > 64-bit
// lock fs repne scas word [edi] -> f0 64f2 6766 af
// mov word[r8d], 255 -> 67 6641 c700 ff00

// F0 lock
// 64 65 FS GS Segment Override
// TODO: when do scas or other do something like FS_M_16__AX
// F2 Repeat String Operation Prefix | Scalar Double-precision Prefix
// F3 Repeat String Operation Prefix | Scalar Single-precision Prefix
// 			2E 36 3E 26 Null Prefix in 64-bit Mode
// 67 Address-size override prefix, when adress 32-bit like [eax] not [rax]
// - prefix 67 adress prefix does give you a way of taking value from adress of
// 		32 bit register like [r8d]
// 66 16-bit Operand-size override prefix | Precision-size override prefix
// - prefix 66 is used with all 16 bit ops like add ax, bx or
//		add word [rax], 255
// REX prefixes
// TODO: VEX/EVEX prefixes

const struct Cmnd cmnds[] = {
	{INOP, {0x90}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ISYSCALL, {0x0f, 0x05}, 2, NOT_FIELD, 0, OPC_INVALID},

	{IJO, {0x70}, 1, NOT_FIELD, 0, __REL_8},
	{INO, {0x71}, 1, NOT_FIELD, 0, __REL_8},
	{IJB, {0x72}, 1, NOT_FIELD, 0, __REL_8},
	{IJNB, {0x73}, 1, NOT_FIELD, 0, __REL_8},
	{IJE, {0x74}, 1, NOT_FIELD, 0, __REL_8},
	{IJBE, {0x75}, 1, NOT_FIELD, 0, __REL_8},
	{IJA, {0x76}, 1, NOT_FIELD, 0, __REL_8},
	{IJS, {0x77}, 1, NOT_FIELD, 0, __REL_8},
	{IJNS, {0x78}, 1, NOT_FIELD, 0, __REL_8},
	{IJP, {0x79}, 1, NOT_FIELD, 0, __REL_8},
	{IJNP, {0x7a}, 1, NOT_FIELD, 0, __REL_8},
	{IJL, {0x7b}, 1, NOT_FIELD, 0, __REL_8},
	{IJNL, {0x7c}, 1, NOT_FIELD, 0, __REL_8},
	{IJLE, {0x7d}, 1, NOT_FIELD, 0, __REL_8},
	{IJG, {0x7f}, 1, NOT_FIELD, 0, __REL_8},
	{IJO, {0x0f, 0x80}, 2, NOT_FIELD, 0, __REL_32},
	{INO, {0x0f, 0x81}, 2, NOT_FIELD, 0, __REL_32},
	{IJB, {0x0f, 0x82}, 2, NOT_FIELD, 0, __REL_32},
	{IJNB, {0x0f, 0x83}, 2, NOT_FIELD, 0, __REL_32},
	{IJE, {0x0f, 0x84}, 2, NOT_FIELD, 0, __REL_32},
	{IJBE, {0x0f, 0x85}, 2, NOT_FIELD, 0, __REL_32},
	{IJA, {0x0f, 0x86}, 2, NOT_FIELD, 0, __REL_32},
	{IJS, {0x0f, 0x87}, 2, NOT_FIELD, 0, __REL_32},
	{IJNS, {0x0f, 0x88}, 2, NOT_FIELD, 0, __REL_32},
	{IJP, {0x0f, 0x89}, 2, NOT_FIELD, 0, __REL_32},
	{IJNP, {0x0f, 0x8a}, 2, NOT_FIELD, 0, __REL_32},
	{IJL, {0x0f, 0x8b}, 2, NOT_FIELD, 0, __REL_32},
	{IJNL, {0x0f, 0x8c}, 2, NOT_FIELD, 0, __REL_32},
	{IJLE, {0x0f, 0x8d}, 2, NOT_FIELD, 0, __REL_32},
	{IJG, {0x0f, 0x8f}, 2, NOT_FIELD, 0, __REL_32},
	{IPUSH, {0x50}, 1, PLUS_REGF, 0, __R_16_64},
	{IPOP, {0x58}, 1, PLUS_REGF, 0, __R_16_64},
	{ICALL, {0xe8}, 1, NOT_FIELD, 0, __REL_32},
	{IJMP, {0xe9}, 1, NOT_FIELD, 0, __REL_32},
	{IJMP, {0xeb}, 1, NOT_FIELD, 0, __REL_8},
	{IINC, {0xfe}, 1, NUM_FIELD, 0, __RM_8},
	{IDEC, {0xfe}, 1, NUM_FIELD, 1, __RM_8},
	{IINC, {0xff}, 1, NUM_FIELD, 0, __RM_16_32_64},
	{IDEC, {0xff}, 1, NUM_FIELD, 1, __RM_16_32_64},
	{ICALL, {0xff}, 1, NUM_FIELD, 2, __RM_16_64},
	// {ICALLF, {0xff}, 1, NUM_FIELD, 3, __RM_16_64},
	{IJMP, {0xff}, 1, NUM_FIELD, 4, __RM_16_64},
	// {IJMPF, {0xff}, 1, NUM_FIELD, 5, __RM_16_64},
	{IPOP, {0x8f}, 1, NUM_FIELD, 0, __RM_16_64},
	{IPUSH, {0xff}, 1, NUM_FIELD, 6, __RM_16_64},
	{IPUSH, {0x0f, 0xa1}, 2, NOT_FIELD, 0, __GS},
	{IPOP, {0x0f, 0xa2}, 2, NOT_FIELD, 0, __GS},
	{IPUSH, {0x0f, 0xa8}, 2, NOT_FIELD, 0, __FS},
	{IPOP, {0x0f, 0xa9}, 2, NOT_FIELD, 0, __FS},
	// add
	{IADD, {0x00}, 1, REG_FIELD, 0, RM_8__R_8},
	{IADD, {0x01}, 1, REG_FIELD, 0, RM_16_32_64__R_16_32_64},
	{IADD, {0x02}, 1, REG_FIELD, 0, R_8__RM_8},
	{IADD, {0x03}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{IADD, {0x04}, 1, NOT_FIELD, 0, AL__IMM_8},
	{IADD, {0x05}, 1, NOT_FIELD, 0, RAX__IMM_16_32},
	{IADD, {0x80}, 1, NUM_FIELD, 0, RM_8__IMM_8},
	{IADD, {0x81}, 1, NUM_FIELD, 0, RM_16_32_64__IMM_16_32},
	{IADD, {0x83}, 1, NUM_FIELD, 0, RM_16_32_64__IMM_8},
	// or
	{IOR, {0x08}, 1, REG_FIELD, 0, RM_8__R_8},
	{IOR, {0x09}, 1, REG_FIELD, 0, RM_16_32_64__R_16_32_64},
	{IOR, {0x0a}, 1, REG_FIELD, 0, R_8__RM_8},
	{IOR, {0x0b}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{IOR, {0x0c}, 1, NOT_FIELD, 0, AL__IMM_8},
	{IOR, {0x0d}, 1, NOT_FIELD, 0, RAX__IMM_16_32},
	{IOR, {0x80}, 1, NUM_FIELD, 1, RM_8__IMM_8},
	{IOR, {0x81}, 1, NUM_FIELD, 1, RM_16_32_64__IMM_16_32},
	{IOR, {0x83}, 1, NUM_FIELD, 1, RM_16_32_64__IMM_8},
	// adc
	{IADC, {0x10}, 1, REG_FIELD, 0, RM_8__R_8},
	{IADC, {0x11}, 1, REG_FIELD, 0, RM_16_32_64__R_16_32_64},
	{IADC, {0x12}, 1, REG_FIELD, 0, R_8__RM_8},
	{IADC, {0x13}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{IADC, {0x14}, 1, NOT_FIELD, 0, AL__IMM_8},
	{IADC, {0x15}, 1, NOT_FIELD, 0, RAX__IMM_16_32},
	{IADC, {0x80}, 1, NUM_FIELD, 2, RM_8__IMM_8},
	{IADC, {0x81}, 1, NUM_FIELD, 2, RM_16_32_64__IMM_16_32},
	{IADC, {0x83}, 1, NUM_FIELD, 2, RM_16_32_64__IMM_8},
	// sbb
	{ISBB, {0x18}, 1, REG_FIELD, 0, RM_8__R_8},
	{ISBB, {0x19}, 1, REG_FIELD, 0, RM_16_32_64__R_16_32_64},
	{ISBB, {0x1a}, 1, REG_FIELD, 0, R_8__RM_8},
	{ISBB, {0x1b}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ISBB, {0x1c}, 1, NOT_FIELD, 0, AL__IMM_8},
	{ISBB, {0x1d}, 1, NOT_FIELD, 0, RAX__IMM_16_32},
	{ISBB, {0x80}, 1, NUM_FIELD, 3, RM_8__IMM_8},
	{ISBB, {0x81}, 1, NUM_FIELD, 3, RM_16_32_64__IMM_16_32},
	{ISBB, {0x83}, 1, NUM_FIELD, 3, RM_16_32_64__IMM_8},
	// and
	{IAND, {0x20}, 1, REG_FIELD, 0, RM_8__R_8},
	{IAND, {0x21}, 1, REG_FIELD, 0, RM_16_32_64__R_16_32_64},
	{IAND, {0x22}, 1, REG_FIELD, 0, R_8__RM_8},
	{IAND, {0x23}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{IAND, {0x24}, 1, NOT_FIELD, 0, AL__IMM_8},
	{IAND, {0x25}, 1, NOT_FIELD, 0, RAX__IMM_16_32},
	{IAND, {0x80}, 1, NUM_FIELD, 4, RM_8__IMM_8},
	{IAND, {0x81}, 1, NUM_FIELD, 4, RM_16_32_64__IMM_16_32},
	{IAND, {0x83}, 1, NUM_FIELD, 4, RM_16_32_64__IMM_8},
	// sbb
	{ISUB, {0x28}, 1, REG_FIELD, 0, RM_8__R_8},
	{ISUB, {0x29}, 1, REG_FIELD, 0, RM_16_32_64__R_16_32_64},
	{ISUB, {0x2a}, 1, REG_FIELD, 0, R_8__RM_8},
	{ISUB, {0x2b}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ISUB, {0x2c}, 1, NOT_FIELD, 0, AL__IMM_8},
	{ISUB, {0x2d}, 1, NOT_FIELD, 0, RAX__IMM_16_32},
	{ISUB, {0x80}, 1, NUM_FIELD, 5, RM_8__IMM_8},
	{ISUB, {0x81}, 1, NUM_FIELD, 5, RM_16_32_64__IMM_16_32},
	{ISUB, {0x83}, 1, NUM_FIELD, 5, RM_16_32_64__IMM_8},
	// xor
	{IXOR, {0x30}, 1, REG_FIELD, 0, RM_8__R_8},
	{IXOR, {0x31}, 1, REG_FIELD, 0, RM_16_32_64__R_16_32_64},
	{IXOR, {0x32}, 1, REG_FIELD, 0, R_8__RM_8},
	{IXOR, {0x33}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{IXOR, {0x34}, 1, NOT_FIELD, 0, AL__IMM_8},
	{IXOR, {0x35}, 1, NOT_FIELD, 0, RAX__IMM_16_32},
	{IXOR, {0x80}, 1, NUM_FIELD, 6, RM_8__IMM_8},
	{IXOR, {0x81}, 1, NUM_FIELD, 6, RM_16_32_64__IMM_16_32},
	{IXOR, {0x83}, 1, NUM_FIELD, 6, RM_16_32_64__IMM_8},
	// cmp
	{ICMP, {0x38}, 1, REG_FIELD, 0, RM_8__R_8},
	{ICMP, {0x39}, 1, REG_FIELD, 0, RM_16_32_64__R_16_32_64},
	{ICMP, {0x3a}, 1, REG_FIELD, 0, R_8__RM_8},
	{ICMP, {0x3b}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMP, {0x3c}, 1, NOT_FIELD, 0, AL__IMM_8},
	{ICMP, {0x3d}, 1, NOT_FIELD, 0, RAX__IMM_16_32},
	{ICMP, {0x80}, 1, NUM_FIELD, 7, RM_8__IMM_8},
	{ICMP, {0x81}, 1, NUM_FIELD, 7, RM_16_32_64__IMM_16_32},
	{ICMP, {0x83}, 1, NUM_FIELD, 7, RM_16_32_64__IMM_8},
	// mov
	{IMOV, {0x88}, 1, REG_FIELD, 0, RM_8__R_8},
	{IMOV, {0x89}, 1, REG_FIELD, 0, RM_16_32_64__R_16_32_64},
	{IMOV, {0x8a}, 1, REG_FIELD, 0, R_8__RM_8},
	{IMOV, {0x8b}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{IMOV, {0xb0}, 1, PLUS_REGF, 0, R_8__IMM_8},
	{IMOV, {0xb8}, 1, PLUS_REGF, 0, R_16_32_64__IMM_16_32_64},
	{IMOV, {0xc6}, 1, NUM_FIELD, 0, RM_8__IMM_8},
	{IMOV, {0xc7}, 1, NUM_FIELD, 0, RM_16_32_64__IMM_16_32},
	//		sregs
	{IMOV, {0x8c}, 1, REG_FIELD, 0, M_16__SREG},
	{IMOV, {0x8c}, 1, REG_FIELD, 0, R_16_32_64__SREG},
	{IMOV, {0x8e}, 1, REG_FIELD, 0, SREG__RM_16},
	//		moffs
	{IMOV, {0xa0}, 1, NOT_FIELD, 0, AL__MOFFS_8},
	{IMOV, {0xa1}, 1, NOT_FIELD, 0, RAX__MOFFS_16_32_64},
	{IMOV, {0xa2}, 1, NOT_FIELD, 0, MOFFS_8__AL},
	{IMOV, {0xa3}, 1, NOT_FIELD, 0, MOFFS_16_32_64__RAX},
	// test
	{ITEST, {0x84}, 1, REG_FIELD, 0, RM_8__R_8},
	{ITEST, {0x85}, 1, REG_FIELD, 0, RM_16_32_64__R_16_32_64},
	{ITEST, {0xa8}, 1, NOT_FIELD, 0, AL__IMM_8},
	{ITEST, {0xa9}, 1, NOT_FIELD, 0, RAX__IMM_16_32},
	{ITEST, {0xf6}, 1, NUM_FIELD, 0, RM_8__IMM_8},
	{ITEST, {0xf7}, 1, NUM_FIELD, 0, RM_16_32_64__IMM_16_32},
};

const char *const WARN_IMM_SIZE_WILL_BE_CHANGED =
	"ПРЕДУПРЕЖДЕНИЕ: Размер числа был вбайт, но данный тип инструкций не "
	"поддерживает числа таких размеров, поэтому скорее всего размер числа "
	"будет урезан.";
const char *const WARN_CHANGE_MEM_SIZE =
	"ПРЕДУПРЕЖДЕНИЕ: Размер адресанта не был равен размеру регистра, но при "
	"этом был явно указан, его размер будет изменен под размер регистра.";

void change_m_sz(struct Inst *in, struct Oper *r, struct Oper *rm) {
	if (is_mem(rm) && rm->sz != r->sz) {
		if (rm->forsed_sz)
			pwi(COLOR_PURPLE, WARN_CHANGE_MEM_SIZE, in);
		rm->sz = r->sz;
	}
	if (rm->sz != r->sz)
		eeg(REGS_SIZES_NOT_MATCH, in);
}

enum OpsCode get_two_opscode(struct Inst *in) {
	struct Oper *l, *r;
	declare_two_ops(in, l, r);

	enum OpsCode code = OPC_INVALID;
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
		if (is_rm(l) && is_reg(r)) {
			change_m_sz(in, r, l);
			code = is_8(l) ? RM_8__R_8 : RM_16_32_64__R_16_32_64;
		} else if (is_reg(l) && is_rm(r)) {
			change_m_sz(in, l, r);
			code = is_8(l) ? R_8__RM_8 : R_16_32_64__RM_16_32_64;
		} else if (is_rm(l) && is_imm(r)) {
			// TODO: better warnings or even errors?
			if (r->sz == QWORD)
				pwi(COLOR_PURPLE, WARN_IMM_SIZE_WILL_BE_CHANGED, in);

			if (l->sz < r->sz)
				r->sz = l->sz;
			else if (l->sz != r->sz && !(l->sz == QWORD && r->sz == DWORD))
				r->sz = l->sz;

			if (is_imm_can_be_a_byte(r))
				r->sz = BYTE;
			// TODO: check this out more
			if (l->sz != r->sz && !(is_64(l) && is_32(r)) &&
				!(!is_8(l) && is_8(r)))
				eeg(REG_MEM_IMM_SIZES_NOT_MATCH, in);
			if (is_al(l))
				code = AL__IMM_8;
			else if (is_rA(l))
				code = RAX__IMM_16_32;
			else if (is_8(l))
				code = RM_8__IMM_8;
			else if (is_8(r))
				code = RM_16_32_64__IMM_8;
			else
				code = RM_16_32_64__IMM_16_32;
		}
		break;
	case IMOV:
		if (is_rm(l) && is_reg(r)) {
			change_m_sz(in, r, l);
			code = is_8(l) ? RM_8__R_8 : RM_16_32_64__R_16_32_64;
		} else if (is_reg(l) && is_rm(r)) {
			change_m_sz(in, l, r);
			code = is_8(l) ? R_8__RM_8 : R_16_32_64__RM_16_32_64;
		} else if (is_seg(r)) {
			if (is_mem(l) && is_16(l))
				code = M_16__SREG;
			else if (is_reg(l) && !is_8(l))
				code = R_16_32_64__SREG;
		} else if (is_seg(l) && is_16(r) && is_rm(r))
			code = SREG__RM_16;
		else if (is_al(l) && is_moffs(r) && is_8(r))
			code = AL__MOFFS_8;
		else if (is_rA(l) && is_moffs(r) && !is_8(r))
			code = RAX__MOFFS_16_32_64;
		else if (is_moffs(l) && is_8(l) && is_al(r))
			code = MOFFS_8__AL;
		else if (is_moffs(l) && !is_8(l) && is_rA(r))
			code = MOFFS_16_32_64__RAX;
		else if (is_imm(r)) {
			if (l->sz < r->sz)
				r->sz = l->sz;
			if (l->sz != r->sz && !(is_64(l) && is_32(r)))
				eeg(REG_MEM_IMM_SIZES_NOT_MATCH, in);

			if (is_reg(l)) {
				if (is_8(l))
					code = R_8__IMM_8;
				else if (!is_8(l)) {
					// if (is_64(r)) // what was this for, wtf is this
					// else TODO: check why i did it
					// 	code = RM_16_32_64__IMM_16_32;
					if (l->sz == r->sz)
						code = R_16_32_64__IMM_16_32_64;
					else
						code = RM_16_32_64__IMM_16_32;
				}
			} else if (is_mem(l)) {
				if (is_64(l) && is_64(r))
					eeg(MEM_IMM_SIZE_QWORD, in);

				if (is_8(l))
					code = RM_8__IMM_8;
				else if (!is_8(l))
					code = RM_16_32_64__IMM_16_32;
			}
		}
		break;
	default:
		eeg("нет пока для такой инструкции\n", in);
	}
	if (code == OPC_INVALID)
		eeg(OPS_CODE_INVALID, in);
	return code;
}

void get_two_ops_prefs(struct Ipcd *i, enum OpsCode code) {
	struct Oper *l, *r;
	declare_two_ops(i->in, l, r);
	// {lock}     fs repne scas   word [edi] ->
	// {f0}   [64 fs][f2   scas] [67   is_r32(r)][66 is_16(r)] af
	// mov word[r8d], 255 -> 67 6641 c700 ff00

	// 67 Address-size OVERRIRE prefix, when adress 32-bit like [eax]
	if (is_mem32(l) || is_mem32(r))
		blist_add(i->cmd, 0x67);
	// 66 16-bit Operand-size OVERRIRE prefix
	// TODO: check if its possible for r to be 16-bit
	if (!is_seg(l) && is_16(l))
		blist_add(i->cmd, 0x66);
	// REX prefixes
	uc rex = 0b01000000;
	if (is_64(l) || is_64(r))
		rex |= REX_W;
	if (is_rm_l(code)) {
		if (is_mem(l))
			rex |= l->rex; // get mem REX's
		else if (is_r_new(l))
			rex |= REX_B; // Extension of ModR/M r/m
		if (is_r_new(r))
			rex |= REX_R; // Extension of ModR/M reg
	} else if (is_rm_r(code)) {
		if (is_mem(r))
			rex |= r->rex; // get mem REX's
		else if (is_r_new(r))
			rex |= REX_B; // Extension of ModR/M r/m
		if (is_r_new(l))
			rex |= REX_R; // Extension of ModR/M reg
	} else if (is_imm_r(code)) {
		if (is_rm_l(code)) {
			if (is_mem(l))
				rex |= l->rex; // get mem REX's
			else if (is_r_new(l))
				rex |= REX_B;	  // Extension of ModR/M r/m
		} else if (is_r_new(l)) { // SUPPOSED TO BE REG
			if (i->c->o == PLUS_REGF)
				rex |= REX_B; // Extension of ModR/M r/m
			else
				rex |= REX_R; // Extension of ModR/M reg
		}
	}
	if (rex != 0b01000000)
		blist_add(i->cmd, rex);
}

const char *const WRONG_INST_OPS =
	"Для данной инструкции и выражений не мыбо найдено подходящего кода, "
	"возможно выражения были неверные.";

const struct Cmnd *get_cmnd(struct Ipcd *i, enum OpsCode code) {
	const struct Cmnd *c = 0, *ct;
	for (size_t j = 0; j < lenofarr(cmnds); j++) {
		ct = cmnds + j; //&cmnds[j];
		if (ct->inst == i->in->code && ct->opsc == code) {
			c = ct;
			break;
		}
	}
	if (c == 0) {
		if (i->debug)
			printf("### инструкция %d, типы операндов %d\n", i->in->code, code);
		eeg(WRONG_INST_OPS, i->in);
	}
	return c;
}

void get_zero_ops_code(struct Ipcd *i) {
	const struct Cmnd *c;
	for (size_t j = 0; j < lenofarr(cmnds); j++) {
		c = cmnds + j;
		if (c->inst == i->in->code) {
			blat(i->cmd, (uc *)c->cmd, c->len);
			break;
		}
	}
}

const char *const WARN_CHANGE_IMM_32_SIZE =
	"ПРЕДУПРЕЖДЕНИЕ: Размер выражения не был равен чбайт, но при "
	"этом был явно указан, его размер будет изменен под чбайт.";
const char *const ERR_REG_NOT_16_OR_64 =
	"Для данного типа инструкций поддерживаются только регистры размеров дбайт "
	"и вбайт.";

enum OpsCode get_one_opscode(struct Inst *in) {
	enum OpsCode code = OPC_INVALID;
	struct Oper *o = get_first_o(in);
	switch (in->code) {
	case IJO:
	case INO:
	case IJB:
	case IJNB:
	case IJE:
	case IJBE:
	case IJA:
	case IJS:
	case IJNS:
	case IJP:
	case IJNP:
	case IJL:
	case IJNL:
	case IJLE:
	case IJG:
	case IJMP:
	case IINT:
	case ICALL:
		if (is_imm(o)) {
			if (in->code == ICALL) {
				if (o->sz != DWORD && o->forsed_sz)
					pwi(COLOR_PURPLE, WARN_CHANGE_IMM_32_SIZE, in);
				o->sz = DWORD;
				code = __REL_32;
			} else if (is_8(o))
				code = __REL_8;
			else if (is_32(o))
				code = __REL_32;
			else {
				pwi(COLOR_PURPLE, WARN_CHANGE_IMM_32_SIZE, in);
				o->sz = DWORD;
				code = __REL_32;
			}
		} else if (is_rm(o) && (is_16(o) || is_64(o)))
			code = __RM_16_64;
		break;
	case IPUSH:
	case IPOP:
		if (is_reg(o)) {
			if (is_fs(o))
				code = __FS;
			else if (is_gs(o))
				code = __GS;
			else if (is_16(o) || is_64(o))
				code = __R_16_64;
			else
				eeg(ERR_REG_NOT_16_OR_64, in);
		} else if (is_mem(o)) {
			if (is_16(o) || is_64(o))
				code = __RM_16_64;
		} else if (is_imm(o)) {
			if (is_32(o))
				code = __IMM_32;
			else if (is_8(o))
				code = __IMM_8;
			else {
				pwi(COLOR_PURPLE, WARN_CHANGE_IMM_32_SIZE, in);
				o->sz = DWORD;
				code = __IMM_32;
			}
		}
		break;
	case IINC:
	case IDEC:
		if (is_rm(o))
			code = is_8(o) ? __RM_8 : __RM_16_32_64;
		break;
	default:
		eeg("нет пока для такой инструкции\n", in);
	}
	if (code == OPC_INVALID)
		eeg(OPS_CODE_INVALID, in);
	return code;
}

void get_one_ops_prefs(struct Ipcd *i, enum OpsCode ops) {
	struct Oper *o = get_first_o(i->in);
	// 67 Address-size OVERRIRE prefix, when adress 32-bit like [eax]
	if (is_mem32(o))
		blist_add(i->cmd, 0x67);
	// 66 16-bit Operand-size OVERRIRE prefix
	// TODO: check if its possible for r to be 16-bit
	if (!is_seg(o) && is_16(o))
		blist_add(i->cmd, 0x66);
	// REX prefixes
	uc rex = 0b01000000;
	if (is_mem(o))
		rex |= o->rex; // get mem REX's
	else if (is_r_new(o))
		rex |= REX_B; // Extension of ModR/M r/m

	if (rex != 0b01000000)
		blist_add(i->cmd, rex);
}

void fill_one_ops_cmd_and_data(struct Ipcd *i) {
	const struct Cmnd *c = i->c;
	struct Oper *o = get_first_o(i->in);
	uc modrm = 0;

	blat(i->cmd, (uc *)c->cmd, c->len);
	// o Register/ Opcode Field
	if (c->o == NOT_FIELD) {
		//   0. NOT_FIELD just op code
		if (is_imm(o))
			add_imm_data(i, o);
	} else if (c->o == NUM_FIELD) {
		// 1. NUM_FIELD The value of the opcode extension values from 0 to 7
		// - like ModR/M byte where Reg field is for o_num
		// - - primary used with imm or ?const regs?
		// - this "ModR/M" byte also have mod and if its just reg and imm then
		// - - mod = 11 and R/M field means just reg code
		// - if mod != 11 then it behaves as
		// - - just usual mod and R/M fields with SIB if needed
		if (is_rm(o)) {
			modrm = (c->o_num) << 3; // r
			modrm += o->mod << 6;
			modrm += get_reg_field(o->rm);
			blist_add(i->cmd, modrm);
			if (is_mem(o))
				add_mem(i, o);
		} else
			eeg("а э ээ ээээ ээ да", i->in);
	} else if (c->o == REG_FIELD) {
		// 2. REG_FIELD r indicates that the ModR/M byte contains a register
		// - operand and an r/m operand
		eeg("REG_FIELD наверно не существует для операций с одним выражением",
			i->in);
	} else if (c->o == PLUS_REGF) {
		// 3. PLUS_REGF When just op code + reg code: PUSH r64/16, POP r64/16
		*(i->cmd->st + i->cmd->size - 1) += get_reg_field(o->rm);
	} else
		eeg("у меня муха щас над столом летает", i->in);
}
