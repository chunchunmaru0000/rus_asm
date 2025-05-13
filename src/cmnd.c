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
const char *const WRONG_SND_OPER_SIZE = "Неверный размер второго операнда";
const char *const WRONG_FPN_OP_SIZE =
	"Недопустимый размер для числа с плавающей "
	"точкой, минимальный - <чбайт> 4 байта, "
	"майсимальный - <вбайт> 8 байт";
const char *const REGS_SIZES_NOT_MATCH = "Размеры регистров не совпадают";
const char *const MEM_REG_SIZES_NOT_MATCH =
	"Размеры памяти и регистра не совпадают";
const char *const REG_MEM_IMM_SIZES_NOT_MATCH =
	"Размеры регистра или памяти и значения не совпадают";

void get_two_ops_code(struct Inst *, struct BList *, struct BList *);

void get_ops_code(struct Inst *in, struct BList *cmd, struct BList *data) {
	if (in->os->size == 2)
		get_two_ops_code(in, cmd, data);
	else
		eeg("йцук\n", in);

	printf("### команда %ld байт: ", cmd->size);
	blist_print(cmd);
	printf("### данные  %ld байт: ", data->size);
	blist_print(data);

	eeg("эээ\n", in);
}

enum OpsCode get_two_opscode(struct Inst *);
void get_two_ops_prefs(struct Inst *, struct BList *, enum OpsCode);
const struct Cmnd *get_cmnd(struct Inst *, enum OpsCode);
#define declare_two_ops(in, l, r)                                              \
	do {                                                                       \
		(l) = plist_get(in->os, 0);                                            \
		(r) = plist_get(in->os, 1);                                            \
	} while (0);

// doesnt changes ModR/M byte
void add_sib(struct BList *cmd, struct Oper *o) {
	uc sib = o->scale << 6;
	sib += get_reg_field(o->index) << 3;
	sib += get_reg_field(o->base);
	blist_add(cmd, sib);
}
void add_disp(struct BList *cmd, struct Oper *o, uc bytes) {
	if (o->disp_is_rel_flag) {
		printf("ПОКА НЕЛЬЗЯ В смещении ИСПОЛЬЗОВАТЬ МЕТКИ");
		exit(1);
	}
	blat(cmd, (uc *)&o->disp, bytes);
}
void add_imm_data(struct BList *data, struct Oper *o) {
	if (o->code == OREL) {
		printf("ПОКА НЕЛЬЗЯ В imm ИСПОЛЬЗОВАТЬ МЕТКИ");
		exit(1);
	} else if (o->code == OINT)
		blat(data, (uc *)&o->t->number, o->sz);
	else if (o->code == OFPN) {
		if (o->sz == DWORD) {
			float value = o->t->fpn;
			blat(data, (uc *)&value, DWORD);
		} else if (o->sz == QWORD)
			blat(data, (uc *)&o->t->fpn, QWORD);
		else {
			printf("Не может вещественное число быть "
				   "размером меньше, чем 4 байта.");
			exit(1);
		}
	}
}

void get_two_ops_code(struct Inst *in, struct BList *cmd, struct BList *data) {
	struct Oper *l, *r;
	declare_two_ops(in, l, r);

	enum OpsCode code = get_two_opscode(in);
	get_two_ops_prefs(in, cmd, code);
	const struct Cmnd *c = get_cmnd(in, code);
	// o Register/ Opcode Field
	//   0. NOT_FIELD just op code
	//   1. NUM_FIELD The value of the opcode extension values from 0 through 7
	//   - like ModR/M byte where Reg field is for o_num
	//   - - primary used with imm or ?const regs?
	//   - this "ModR/M" byte also have mod and if its just reg and imm then
	//   - - mod = 11 and R/M field means just reg code
	//   - if mod != 11 then it behaves as
	//   - - just usual mod and R/M fields with SIB if needed
	//   2. REG_FIELD r indicates that the ModR/M byte contains a register
	//   operand and an r/m operand. 00 ADD
	//   3. PLUS_REGF When just op code + reg code like B0+r
	uc modrm = 0;
	blat(cmd, (uc *)c->cmd, c->len);
	if (c->o == NOT_FIELD)
		;
	else if (c->o == NUM_FIELD) {
		if (is_imm(r)) {
			modrm = (c->o_num) << 3; // reg
			modrm += l->mod << 6;
			modrm += get_reg_field(l->rm);
			blist_add(cmd, modrm);

			if (is_mem(l)) {
				if (l->mod == MOD_MEM) {
					// REMEMBER: mod = 00, rm = 101 == [RIP+disp32]
					if (l->rm == R_RBP)
						add_disp(cmd, l, DWORD);
					else if (l->rm == R_RSP) {
						add_sib(cmd, l);
						// mod = 00, rm = 100, base = 101 == no base register
						// and REMEMBER: do disp 32 although mod is 00
						if (l->base == R_RBP)
							add_disp(cmd, l, DWORD);
					}
					// else nothing because already blist_add(cmd, modrm) before
				} else if (l->mod == MOD_MEM_D8) {
				} else { // l->mod == MOD_MEM_D32
				}
			}

			add_imm_data(data, r);
		} else
			eeg("только числа пока", in);
	} else if (c->o == REG_FIELD) {

	} else if (c->o == PLUS_REGF) {
	} else
		eeg("че не так то", in);
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
	//		sregs
	{IMOV, {0x8c}, 1, REG_FIELD, 0, M_16__SREG},
	{IMOV, {0x8c}, 1, REG_FIELD, 0, R_16_32_64__SREG},
	{IMOV, {0x8e}, 1, REG_FIELD, 0, SREG__RM_16},
	//		moffs
	{IMOV, {0xa0}, 1, NOT_FIELD, 0, AL__MOFFS_8},
	{IMOV, {0xa1}, 1, NOT_FIELD, 0, RAX__MOFFS_16_32_64},
	{IMOV, {0xa2}, 1, NOT_FIELD, 0, MOFFS_8__AL},
	{IMOV, {0xa3}, 1, NOT_FIELD, 0, MOFFS_16_32_64__RAX},
	//		imms
	{IMOV, {0xb0}, 1, PLUS_REGF, 0, R_8__IMM_8},
	{IMOV, {0xb8}, 1, PLUS_REGF, 0, R_16_32_64__IMM_16_32_64},
	{IMOV, {0xc6}, 1, NUM_FIELD, 0, RM_8__IMM_8},
	{IMOV, {0xc7}, 1, NUM_FIELD, 0, RM_16_32_64__IMM_16_32},
};

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
			if (l->sz != r->sz)
				eeg(REGS_SIZES_NOT_MATCH, in);
			code = is_8(l) ? RM_8__R_8 : RM_16_32_64__R_16_32_64;
		} else if (is_reg(l) && is_rm(r)) {
			if (l->sz != r->sz)
				eeg(MEM_REG_SIZES_NOT_MATCH, in);
			code = is_8(l) ? R_8__RM_8 : R_16_32_64__RM_16_32_64;
		} else if (is_rm(l) && is_imm(r)) {
			// TODO: check this out
			if (l->sz < r->sz) // l->sz != r->sz && !(is_64(l) && is_32(r)))
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
			if (l->sz != r->sz)
				eeg(REGS_SIZES_NOT_MATCH, in);
			code = is_8(l) ? RM_8__R_8 : RM_16_32_64__R_16_32_64;
		} else if (is_reg(l) && is_rm(r)) {
			if (l->sz != r->sz)
				eeg(MEM_REG_SIZES_NOT_MATCH, in);
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
			if (is_reg(l)) {
				if (is_8(l) && is_8(r))
					code = R_8__IMM_8;
				else if (!is_8(l) && !is_8(r))
					code = R_16_32_64__IMM_16_32_64;
			} else if (is_mem(l)) {
				if (is_8(l) && is_8(r))
					code = RM_8__IMM_8;
				else if (!is_8(l) && is_16(r) && is_32(r))
					code = RM_16_32_64__IMM_16_32;
			}
		}
		break;
	default:
		eeg("нет пока для такой инструкции\n", in);
	}
	return code;
}

void get_two_ops_prefs(struct Inst *in, struct BList *cmd, enum OpsCode code) {
	struct Oper *l, *r;
	declare_two_ops(in, l, r);
	// {lock}     fs repne scas   word [edi] ->
	// {f0}   [64 fs][f2   scas] [67   is_r32(r)][66 is_16(r)] af
	// mov word[r8d], 255 -> 67 6641 c700 ff00

	// 67 Address-size OVERRIRE prefix, when adress 32-bit like [eax]
	if (is_mem32(l) || is_mem32(r))
		blist_add(cmd, 0x67);
	// 66 16-bit Operand-size OVERRIRE prefix
	// TODO: check if its possible for r to be 16-bit
	if (!is_seg(l) && is_16(l))
		blist_add(cmd, 0x66);
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
				rex |= REX_B;	// Extension of ModR/M r/m
		} else if (is_r_new(l)) // SUPPOSED TO BE REG
			rex |= REX_R;		// Extension of ModR/M reg
	}
	if (rex != 0b01000000)
		blist_add(cmd, rex);
}

const struct Cmnd *get_cmnd(struct Inst *in, enum OpsCode code) {
	const struct Cmnd *c = 0, *ct;
	for (size_t i = 0; i < lenofarr(cmnds); i++) {
		ct = cmnds + i; //&cmnds[i];
		if (ct->inst == in->code && ct->opsc == code) {
			c = ct;
			break;
		}
	}
	if (c == 0) {
		printf("инструкция %d, типы операндов %d\n", in->code, code);
		eeg("НЕ НАЙДЕН КОД ИНСТРУКЦИИ", in);
	}
	return c;
}
