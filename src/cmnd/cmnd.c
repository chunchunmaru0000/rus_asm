#include "cmnd.h"
#include <stdint.h>
#include <stdio.h>

const char *const WRONG_FST_OPER =
	"НЕДОСТУПНЫЙ ТИП ПЕРВОГО ОПЕРАНДА НА ДАНЫЙ МОМЕНТ.";
const char *const WRONG_FST_OPER_INT =
	"НЕДОСТУПНЫЙ ТИП ПЕРВОГО ОПЕРАНДА, ЧИСЛО НЕ МОЖЕТ БЫТЬ ПЕРВЫМ ОПЕРАНДОМ "
	"ДЛЯ ДАННОЙ КОМАНДЫ.";
const char *const WRONG_SND_OPER =
	"НЕДОСТУПНЫЙ ТИП ВТОРОГО ОПЕРАНДА НА ДАНЫЙ МОМЕНТ.";
const char *const WRONG_FST_OPER_REG_DWORD =
	"Неверный регистр, для значения размера <чбайт> 4 байта.";
const char *const WRONG_FST_OPER_REG_QWORD =
	"Неверный регистр, для значения размера <вбайт> 8 байт.";
const char *const WRONG_SND_OPER_SIZE = "Неверный размер второго операнда.";
const char *const WRONG_FPN_OP_SIZE =
	"Недопустимый размер для числа с плавающей "
	"точкой, минимальный - <чбайт> 4 байта, "
	"майсимальный - <вбайт> 8 байт.";
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
const char *const UNKNOWN_LABEL = "Метка не была найдена [%s].";
const char *const WRONG_ADDR_SZ =
	"Не может адрес метки быть размером меньше, чем 4 байта.";
const char *const OPS_CODE_INVALID =
	"Не удалось определить типы выражений для инструкции, возможно она была "
	"неверной.";
const char *const OPS_SIZE_WRONG =
	"Неверное количество выражений для инструкции.";
const char *const EXPEXTED_DWORD_OR_QWORD =
	"Ожидалось выражения размера <чбайт> или <вбайт>.";
const char *const _HERE_CANT_BE_USED_AS_REL =
	"Операнд _ЗДЕСЬ не может использоваться как относительный адрес, потому "
	"что логически он будет просто равен всегда 0, используйте просто 0.";
const char *const _TUT_CANT_BE_USED_AS_REL =
	"Операнд _ТУТ не может использоваться как относительный адрес, потому "
	"что логически он будет просто равен всегда 0, используйте просто 0.";
constr MOVZX_CANT_USE_AH_TO_BH_REGS =
	"Инструкция 'movzx', которая в этом ассембелре равна мнемонике 'быть', не "
	"может в качестве второго операнда использовать регистры 'аг', 'дг', 'сг', "
	"'бг', потому что это не имеет смысла, так как эти регистры являются "
	"старшими частями 16-битных регистров, и расширять их не расширяя "
	"при этом младшие байты - смысла не имеет.";

void (*gets[])(struct Ipcd *) = {
	get_zero_ops_code,
	get_one_ops_code,
	get_two_ops_code,
	get_tri_ops_code,
};

void get_ops_code(struct Ipcd *i) {
	if (i->in->os->size >= (long)lenofarr(gets))
		ee(i->in->f, i->in->p, OPS_SIZE_WRONG);
	(gets[i->in->os->size])(i);

	if (0b10 & i->debug) {
		printf("### команда %d байт: ", i->cmd->size);
		blist_print(i->cmd);
		printf("### данные  %d байт: ", i->data->size);
		blist_print(i->data);
	}
}

// doesnt changes ModR/M byte
void add_sib(struct BList *cmd, struct Oper *o) {
	uc sib = o->scale << 6;
	sib += get_reg_field(o->index) << 3;
	sib += get_reg_field(o->base);
	blist_add(cmd, sib);
}

void add_disp(struct Ipcd *i, struct Oper *o, uc bytes) {
	struct Defn *np;
	uint64_t some_value;
	struct Oper *lbel_o;

	if (o->disp_is_rel_flag == OREL) {
		if (o->rel_flags == RF_HERE)
			ee(i->in->f, o->t->p, _HERE_CANT_BE_USED_AS_REL);
		if (o->rel_flags == RF_TUT)
			ee(i->in->f, o->t->p, _TUT_CANT_BE_USED_AS_REL);

		np = new_not_plov(o->rel_view, i->data->size, 0, REL_ADDR);
		plist_add(i->not_plovs, np);

		some_value = 0x6c6572; // rel
		blat(i->data, (uc *)&some_value, bytes);
	} else if (o->disp_is_rel_flag == OBIN) { // meand OBIN

		// o->t->num is disp struct Oper *
		lbel_o = find_any_label_in_bin_tree((struct Oper *)o->t->num);
		// cmd_end here is struct Oper *
		np = new_not_plov(lbel_o->t->view, i->data->size, o->t->num, BIN_OP_ADDR);
		plist_add(i->not_plovs, np);

		// REMEMBER: FIRST ADD new_not_plov WITH IT i->data->size THEN WRITE
		some_value = 0x4e49424f; // OBIN
		blat(i->data, (uc *)&some_value, bytes);
	} else
		blat(i->data, (uc *)&o->disp, bytes);
}

void add_imm_data(struct Ipcd *i, struct Oper *o) {
	enum UT usage_type;
	struct Defn *np;
	uint64_t some_value;
	struct Oper *lbel_o;

	if (o->code == OREL) {
		if (is_rel(i->c->opsc)) {
			if (o->rel_flags == RF_HERE)
				ee(i->in->f, o->t->p, _HERE_CANT_BE_USED_AS_REL);
			if (o->rel_flags == RF_TUT)
				ee(i->in->f, o->t->p, _TUT_CANT_BE_USED_AS_REL);

			usage_type = o->sz == BYTE ? REL_ADDR_8 : REL_ADDR;
			np = new_not_plov(o->t->view, i->data->size, 0, usage_type);
		} else {
			if (o->rel_flags == RF_HERE) {
				usage_type = HERE_ADDR;
			} else if (o->rel_flags == RF_TUT) {
				usage_type = TUT_ADDR;
			} else
				usage_type = ADDR;
			np = new_not_plov(o->t->view, i->data->size, 0, usage_type);
		}
		plist_add(i->not_plovs, np);

		// REMEMBER: FIRST ADD new_not_plov WITH IT i->data->size THEN WRITE
		some_value = 0x6c6572; // rel
		blat(i->data, (uc *)&some_value, o->sz);
	} else if (o->code == OINT)
		blat(i->data, (uc *)&o->t->num, o->sz);
	else if (o->code == OFPN) {
		if (o->sz == DWORD) {
			float value = o->t->real;
			blat(i->data, (uc *)&value, DWORD);
		} else if (o->sz == QWORD)
			blat(i->data, (uc *)&o->t->real, QWORD);
		else
			ee(i->in->f, i->in->p, WRONG_FPN_SZ);
	} else if (o->code == OBIN) {
		lbel_o = find_any_label_in_bin_tree(o);
		// cmd_end here is struct Oper *
		np = new_not_plov(lbel_o->t->view, i->data->size, (long)o, BIN_OP_ADDR);
		plist_add(i->not_plovs, np);

		// REMEMBER: FIRST ADD new_not_plov WITH IT i->data->size THEN WRITE
		some_value = 0x4e49424f; // OBIN
		blat(i->data, (uc *)&some_value, o->sz);
	} else if (o->code == OMOFFS)
		blat(i->data, (uc *)&o->t->num, o->mem_sz);
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

// ССЫЛКИ:
// * http://ref.x86asm.net/coder64.html
// * https://www.felixcloutier.com/x86/

// add r8w, 128 does 6641 81c0 8000,adress prefix > so 16-bit prefix > 64-bit
// lock fs    repne  scas  word  [edi] -> f0 64f2 6766 af
// атом испфс повтнр осмсд       (еди)
// mov word[r8d], 255 -> 67 6641 c700 ff00

// F0 lock
// 64 65 FS GS Segment Override
// 67 Address-size override prefix, when adress 32-bit like [eax] not [rax]
// - prefix 67 adress prefix does give you a way of taking value from adress of
// 		32 bit register like [r8d]
// F2 Repeat String Operation Prefix | Scalar Double-precision Prefix
// F3 Repeat String Operation Prefix | Scalar Single-precision Prefix
// 			2E 36 3E 26 Null Prefix in 64-bit Mode
// 66 16-bit Operand-size override prefix | Precision-size override prefix
// - prefix 66 is used with all 16 bit ops like add ax, bx or
//		add word [rax], 255
// REX prefixes
const struct Cmnd cmnds0[] = {
	{INOP, {0x90}, 1, NOT_FIELD, 0, OPC_INVALID},
	//
	{IRET, {0xc3}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IRETF, {0x48, 0xcb}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ILOCK, {0xf0}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ISYSCALL, {0x0f, 0x05}, 2, NOT_FIELD, 0, OPC_INVALID},
	{IINSB, {0x6c}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IINSW, {0x66, 0x6d}, 2, NOT_FIELD, 0, OPC_INVALID},
	{IINSD, {0x6d}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IOUTSB, {0x6e}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IOUTSW, {0x66, 0x6f}, 2, NOT_FIELD, 0, OPC_INVALID},
	{IOUTSD, {0x6f}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IPAUSE, {0xf3, 0x90}, 2, NOT_FIELD, 0, OPC_INVALID},
	{IWAIT, {0x9b}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ICBW, {0x66, 0x98}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ICWDE, {0x98}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ICDQE, {0x48, 0x98}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ICWD, {0x66, 0x99}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ICDQ, {0x99}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ICQO, {0x48, 0x99}, 2, NOT_FIELD, 0, OPC_INVALID},
	{IPUSHF, {0x9c}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IPOPF, {0x9d}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ISAHF, {0x9e}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ILAHF, {0x9f}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IMOVSB, {0xa4}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IMOVSW, {0x66, 0xa5}, 2, NOT_FIELD, 0, OPC_INVALID},
	{IMOVSD, {0xa5}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IMOVSQ, {0x48, 0xa5}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ICMPSB, {0xa6}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ICMPSW, {0x66, 0xa7}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ICMPSD, {0xa7}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ICMPSQ, {0x48, 0xa7}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ISTOSB, {0xaa}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ISTOSW, {0x66, 0xab}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ISTOSD, {0xab}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ISTOSQ, {0x48, 0xab}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ILODSB, {0xac}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ILODSW, {0x66, 0xad}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ILODSD, {0xad}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ILODSQ, {0x48, 0xad}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ISCASB, {0xae}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ISCASW, {0x66, 0xaf}, 2, NOT_FIELD, 0, OPC_INVALID},
	{ISCASD, {0xaf}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ISCASQ, {0x48, 0xaf}, 2, NOT_FIELD, 0, OPC_INVALID},
	{IINT3, {0xcc}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ILEAVE, {0xc9}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IRETFQ, {0x48, 0xcf}, 2, NOT_FIELD, 0, OPC_INVALID},
	{IINT1, {0xf1}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IHLT, {0xf4}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ICMC, {0xf5}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ICLC, {0xf8}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ISTC, {0xf9}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ICLI, {0xfa}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ISTI, {0xfb}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ICLD, {0xfc}, 1, NOT_FIELD, 0, OPC_INVALID},
	{ISTD, {0xfd}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IUSEFS, {0x64}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IUSEGS, {0x65}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IREPNZ, {0xf2}, 1, NOT_FIELD, 0, OPC_INVALID},
	{IREP, {0xf3}, 1, NOT_FIELD, 0, OPC_INVALID},
};
const struct Cmnd cmnds1[] = {
	{INOT, {0xf6}, 1, NUM_FIELD, 2, __RM_8},
	{INEG, {0xf6}, 1, NUM_FIELD, 3, __RM_8},
	{INOT, {0xf7}, 1, NUM_FIELD, 2, __RM_16_32_64},
	{INEG, {0xf7}, 1, NUM_FIELD, 3, __RM_16_32_64},
	{IRET, {0xc2}, 1, NOT_FIELD, 0, __IMM_16},
	{IRETF, {0x48, 0xca}, 2, NOT_FIELD, 0, __IMM_16},
	{IINT, {0xcd}, 1, NOT_FIELD, 0, __IMM_8},
	{IJO, {0x70}, 1, NOT_FIELD, 0, __REL_8},
	{IJNO, {0x71}, 1, NOT_FIELD, 0, __REL_8},
	{IJB, {0x72}, 1, NOT_FIELD, 0, __REL_8},
	{IJNB, {0x73}, 1, NOT_FIELD, 0, __REL_8},
	{IJE, {0x74}, 1, NOT_FIELD, 0, __REL_8},
	{IJNE, {0x75}, 1, NOT_FIELD, 0, __REL_8},
	{IJBE, {0x76}, 1, NOT_FIELD, 0, __REL_8},
	{IJA, {0x77}, 1, NOT_FIELD, 0, __REL_8},
	{IJS, {0x78}, 1, NOT_FIELD, 0, __REL_8},
	{IJNS, {0x79}, 1, NOT_FIELD, 0, __REL_8},
	{IJP, {0x7a}, 1, NOT_FIELD, 0, __REL_8},
	{IJNP, {0x7b}, 1, NOT_FIELD, 0, __REL_8},
	{IJL, {0x7c}, 1, NOT_FIELD, 0, __REL_8},
	{IJNL, {0x7d}, 1, NOT_FIELD, 0, __REL_8},
	{IJLE, {0x7e}, 1, NOT_FIELD, 0, __REL_8},
	{IJG, {0x7f}, 1, NOT_FIELD, 0, __REL_8},
	{IJO, {0x0f, 0x80}, 2, NOT_FIELD, 0, __REL_32},
	{IJNO, {0x0f, 0x81}, 2, NOT_FIELD, 0, __REL_32},
	{IJB, {0x0f, 0x82}, 2, NOT_FIELD, 0, __REL_32},
	{IJNB, {0x0f, 0x83}, 2, NOT_FIELD, 0, __REL_32},
	{IJE, {0x0f, 0x84}, 2, NOT_FIELD, 0, __REL_32},
	{IJNE, {0x0f, 0x85}, 2, NOT_FIELD, 0, __REL_32},
	{IJBE, {0x0f, 0x86}, 2, NOT_FIELD, 0, __REL_32},
	{IJA, {0x0f, 0x87}, 2, NOT_FIELD, 0, __REL_32},
	{IJS, {0x0f, 0x88}, 2, NOT_FIELD, 0, __REL_32},
	{IJNS, {0x0f, 0x89}, 2, NOT_FIELD, 0, __REL_32},
	{IJP, {0x0f, 0x8a}, 2, NOT_FIELD, 0, __REL_32},
	{IJNP, {0x0f, 0x8b}, 2, NOT_FIELD, 0, __REL_32},
	{IJL, {0x0f, 0x8c}, 2, NOT_FIELD, 0, __REL_32},
	{IJNL, {0x0f, 0x8d}, 2, NOT_FIELD, 0, __REL_32},
	{IJLE, {0x0f, 0x8e}, 2, NOT_FIELD, 0, __REL_32},
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
	{ICALLF, {0xff}, 1, NUM_FIELD, 3, __RM_16_64},
	{IJMP, {0xff}, 1, NUM_FIELD, 4, __RM_16_64},
	{IJMPF, {0xff}, 1, NUM_FIELD, 5, __RM_16_64},
	{IPUSH, {0xff}, 1, NUM_FIELD, 6, __RM_16_64},
	{IPOP, {0x8f}, 1, NUM_FIELD, 0, __RM_16_64},
	{IPUSH, {0x0f, 0xa1}, 2, NOT_FIELD, 0, __GS},
	{IPOP, {0x0f, 0xa2}, 2, NOT_FIELD, 0, __GS},
	{IPUSH, {0x0f, 0xa8}, 2, NOT_FIELD, 0, __FS},
	{IPOP, {0x0f, 0xa9}, 2, NOT_FIELD, 0, __FS},
	{ILOOPNZ, {0xe0}, 1, NOT_FIELD, 0, __REL_8},
	{ILOOPZ, {0xe1}, 1, NOT_FIELD, 0, __REL_8},
	{ILOOP, {0xe2}, 1, NOT_FIELD, 0, __REL_8},
	{IJECXZ, {0x67, 0xe3}, 2, NOT_FIELD, 0, __REL_8},
	{IJRCXZ, {0xe3}, 1, NOT_FIELD, 0, __REL_8},
	{IMUL, {0xf6}, 1, NUM_FIELD, 4, __RM_8},
	{IIMUL, {0xf6}, 1, NUM_FIELD, 5, __RM_8},
	{IDIV, {0xf6}, 1, NUM_FIELD, 6, __RM_8},
	{IIDIV, {0xf6}, 1, NUM_FIELD, 7, __RM_8},
	{IMUL, {0xf7}, 1, NUM_FIELD, 4, __RM_16_32_64},
	{IIMUL, {0xf7}, 1, NUM_FIELD, 5, __RM_16_32_64},
	{IDIV, {0xf7}, 1, NUM_FIELD, 6, __RM_16_32_64},
	{IIDIV, {0xf7}, 1, NUM_FIELD, 7, __RM_16_32_64},
	{IROL1, {0xd0}, 1, NUM_FIELD, 0, __RM_8},
	{IROR1, {0xd0}, 1, NUM_FIELD, 1, __RM_8},
	{IRCL1, {0xd0}, 1, NUM_FIELD, 2, __RM_8},
	{IRCR1, {0xd0}, 1, NUM_FIELD, 3, __RM_8},
	{ISHL1, {0xd0}, 1, NUM_FIELD, 4, __RM_8},
	{ISHR1, {0xd0}, 1, NUM_FIELD, 5, __RM_8},
	{ISAR1, {0xd0}, 1, NUM_FIELD, 7, __RM_8},
	{IROL1, {0xd1}, 1, NUM_FIELD, 0, __RM_16_32_64},
	{IROR1, {0xd1}, 1, NUM_FIELD, 1, __RM_16_32_64},
	{IRCL1, {0xd1}, 1, NUM_FIELD, 2, __RM_16_32_64},
	{IRCR1, {0xd1}, 1, NUM_FIELD, 3, __RM_16_32_64},
	{ISHL1, {0xd1}, 1, NUM_FIELD, 4, __RM_16_32_64},
	{ISHR1, {0xd1}, 1, NUM_FIELD, 5, __RM_16_32_64},
	{ISAR1, {0xd1}, 1, NUM_FIELD, 7, __RM_16_32_64},
	{IROL, {0xd2}, 1, NUM_FIELD, 0, __RM_8},
	{IROR, {0xd2}, 1, NUM_FIELD, 1, __RM_8},
	{IRCL, {0xd2}, 1, NUM_FIELD, 2, __RM_8},
	{IRCR, {0xd2}, 1, NUM_FIELD, 3, __RM_8},
	{ISHL, {0xd2}, 1, NUM_FIELD, 4, __RM_8},
	{ISHR, {0xd2}, 1, NUM_FIELD, 5, __RM_8},
	{ISAR, {0xd2}, 1, NUM_FIELD, 7, __RM_8},
	{IROL, {0xd3}, 1, NUM_FIELD, 0, __RM_16_32_64},
	{IROR, {0xd3}, 1, NUM_FIELD, 1, __RM_16_32_64},
	{IRCL, {0xd3}, 1, NUM_FIELD, 2, __RM_16_32_64},
	{IRCR, {0xd3}, 1, NUM_FIELD, 3, __RM_16_32_64},
	{ISHL, {0xd3}, 1, NUM_FIELD, 4, __RM_16_32_64},
	{ISHR, {0xd3}, 1, NUM_FIELD, 5, __RM_16_32_64},
	{ISAR, {0xd3}, 1, NUM_FIELD, 7, __RM_16_32_64},
	{ISETO, {0x0f, 0x90}, 2, NUM_FIELD, 0, __RM_8},
	{ISETNO, {0x0f, 0x91}, 2, NUM_FIELD, 0, __RM_8},
	{ISETB, {0x0f, 0x92}, 2, NUM_FIELD, 0, __RM_8},
	{ISETNB, {0x0f, 0x93}, 2, NUM_FIELD, 0, __RM_8},
	{ISETE, {0x0f, 0x94}, 2, NUM_FIELD, 0, __RM_8},
	{ISETNE, {0x0f, 0x95}, 2, NUM_FIELD, 0, __RM_8},
	{ISETBE, {0x0f, 0x96}, 2, NUM_FIELD, 0, __RM_8},
	{ISETA, {0x0f, 0x97}, 2, NUM_FIELD, 0, __RM_8},
	{ISETS, {0x0f, 0x98}, 2, NUM_FIELD, 0, __RM_8},
	{ISETNS, {0x0f, 0x99}, 2, NUM_FIELD, 0, __RM_8},
	{ISETP, {0x0f, 0x9a}, 2, NUM_FIELD, 0, __RM_8},
	{ISETNP, {0x0f, 0x9b}, 2, NUM_FIELD, 0, __RM_8},
	{ISETL, {0x0f, 0x9c}, 2, NUM_FIELD, 0, __RM_8},
	{ISETNL, {0x0f, 0x9d}, 2, NUM_FIELD, 0, __RM_8},
	{ISETLE, {0x0f, 0x9e}, 2, NUM_FIELD, 0, __RM_8},
	{ISETG, {0x0f, 0x9f}, 2, NUM_FIELD, 0, __RM_8},
};
const struct Cmnd cmnds2[] = {
	{IIMUL, {0x0f, 0xaf}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
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
	{IMOV, {0x63}, 1, REG_FIELD, 0, R_64__RM_32},
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
	// shift rotate
	{IROL, {0xc0}, 1, NUM_FIELD, 0, RM_8__IMM_8},
	{IROR, {0xc0}, 1, NUM_FIELD, 1, RM_8__IMM_8},
	{IRCL, {0xc0}, 1, NUM_FIELD, 2, RM_8__IMM_8},
	{IRCR, {0xc0}, 1, NUM_FIELD, 3, RM_8__IMM_8},
	{ISHL, {0xc0}, 1, NUM_FIELD, 4, RM_8__IMM_8},
	{ISHR, {0xc0}, 1, NUM_FIELD, 5, RM_8__IMM_8},
	{ISAR, {0xc0}, 1, NUM_FIELD, 7, RM_8__IMM_8},
	{IROL, {0xc1}, 1, NUM_FIELD, 0, RM_16_32_64__IMM_8},
	{IROR, {0xc1}, 1, NUM_FIELD, 1, RM_16_32_64__IMM_8},
	{IRCL, {0xc1}, 1, NUM_FIELD, 2, RM_16_32_64__IMM_8},
	{IRCR, {0xc1}, 1, NUM_FIELD, 3, RM_16_32_64__IMM_8},
	{ISHL, {0xc1}, 1, NUM_FIELD, 4, RM_16_32_64__IMM_8},
	{ISHR, {0xc1}, 1, NUM_FIELD, 5, RM_16_32_64__IMM_8},
	{ISAR, {0xc1}, 1, NUM_FIELD, 7, RM_16_32_64__IMM_8},
	// some
	{IXCHG, {0x86}, 1, REG_FIELD, 0, R_8__RM_8},
	{IXCHG, {0x87}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{IXCHG, {0x90}, 1, PLUS_REGF, 0, R_16_32_64__RAX},
	{ILEA, {0x8d}, 1, REG_FIELD, 0, R_16_32_64__M},
	{IENTER, {0xc8}, 1, NOT_FIELD, 0, IMM_16__IMM_8},
	{IINPUT, {0xe4}, 1, NOT_FIELD, 0, AL__IMM_8},
	{IINPUT, {0xe5}, 1, NOT_FIELD, 0, EAX__IMM_8},
	{IOUTPUT, {0xe6}, 1, NOT_FIELD, 0, IMM_8__AL},
	{IOUTPUT, {0xe7}, 1, NOT_FIELD, 0, IMM_8__EAX},
	{IINPUT, {0xec}, 1, NOT_FIELD, 0, AL__DX},
	{IINPUT, {0xed}, 1, NOT_FIELD, 0, EAX__DX},
	{IOUTPUT, {0xee}, 1, NOT_FIELD, 0, DX__AL},
	{IOUTPUT, {0xef}, 1, NOT_FIELD, 0, DX__EAX},
	// xmm
	{IMOVUPS, {0x0f, 0x10}, 2, REG_FIELD, 0, X__XM_128},
	{IMOVSS, {0x0f, 0x10}, 2, REG_FIELD, 0, X__XM_32},
	{IMOVUPD, {0x0f, 0x10}, 2, REG_FIELD, 0, X__XM_128},
	{IMOVSD, {0x0f, 0x10}, 2, REG_FIELD, 0, X__XM_64},
	{IMOVUPS, {0x0f, 0x11}, 2, REG_FIELD, 0, XM_128__X},
	{IMOVSS, {0x0f, 0x11}, 2, REG_FIELD, 0, XM_32__X},
	{IMOVUPD, {0x0f, 0x11}, 2, REG_FIELD, 0, XM_128__X},
	{IMOVSD, {0x0f, 0x11}, 2, REG_FIELD, 0, XM_64__X},
	{IMOVHLPS, {0x0f, 0x12}, 2, REG_FIELD, 0, X__X},
	{IMOVLPS, {0x0f, 0x12}, 2, REG_FIELD, 0, X__M_64},
	{IMOVLPD, {0x0f, 0x12}, 2, REG_FIELD, 0, X__M_64},
	{IMOVDDUP, {0x0f, 0x12}, 2, REG_FIELD, 0, X__XM_64},
	{IMOVSLDUP, {0x0f, 0x12}, 2, REG_FIELD, 0, X__XM_64},
	{IMOVLPS, {0x0f, 0x13}, 2, REG_FIELD, 0, M_64__X},
	{IMOVLPS, {0x0f, 0x13}, 2, REG_FIELD, 0, M_64__X},
	{IUNPCKLPS, {0x0f, 0x14}, 2, REG_FIELD, 0, X__XM_64},
	{IUNPCKLPD, {0x0f, 0x14}, 2, REG_FIELD, 0, X__XM_128},
	{IUNPCKHPS, {0x0f, 0x15}, 2, REG_FIELD, 0, X__XM_64},
	{IUNPCKHPD, {0x0f, 0x15}, 2, REG_FIELD, 0, X__XM_128},
	{IMOVLHPS, {0x0f, 0x16}, 2, REG_FIELD, 0, X__X},
	{IMOVHPS, {0x0f, 0x16}, 2, REG_FIELD, 0, X__M_64},
	{IMOVHPD, {0x0f, 0x16}, 2, REG_FIELD, 0, X__M_64},
	{IMOVSHDUP, {0x0f, 0x16}, 2, REG_FIELD, 0, X__XM_64},
	{IMOVHPS, {0x0f, 0x17}, 2, REG_FIELD, 0, M_64__X},
	{IMOVHPD, {0x0f, 0x17}, 2, REG_FIELD, 0, M_64__X},
	{IMOVAPS, {0x0f, 0x28}, 2, REG_FIELD, 0, X__XM_128},
	{IMOVAPD, {0x0f, 0x28}, 2, REG_FIELD, 0, X__XM_128},
	{IMOVAPS, {0x0f, 0x29}, 2, REG_FIELD, 0, XM_128__X},
	{IMOVAPD, {0x0f, 0x29}, 2, REG_FIELD, 0, XM_128__X},
	{ICVTPI2PS, {0x0f, 0x2a}, 2, REG_FIELD, 0, X__MMM_64},
	{ICVTSI2SS, {0x0f, 0x2a}, 2, REG_FIELD, 0, X__RM_32_64},
	{ICVTPI2PD, {0x0f, 0x2a}, 2, REG_FIELD, 0, X__MMM_64},
	{ICVTSI2SD, {0x0f, 0x2a}, 2, REG_FIELD, 0, X__RM_32_64},
	{IMOVNTPS, {0x0f, 0x2b}, 2, REG_FIELD, 0, M_128__X},
	{IMOVNTPD, {0x0f, 0x2b}, 2, REG_FIELD, 0, M_128__X},
	{ICVTTPS2PI, {0x0f, 0x2c}, 2, REG_FIELD, 0, MM__XM_64},
	{ICVTTSS2SI, {0x0f, 0x2c}, 2, REG_FIELD, 0, R_32_64__XM_32},
	{ICVTTPD2PI, {0x0f, 0x2c}, 2, REG_FIELD, 0, MM__XM_128},
	{ICVTTSD2SI, {0x0f, 0x2c}, 2, REG_FIELD, 0, R_32_64__XM_64},
	{ICVTPS2PI, {0x0f, 0x2d}, 2, REG_FIELD, 0, MM__XM_64},
	{ICVTSS2SI, {0x0f, 0x2d}, 2, REG_FIELD, 0, R_32_64__XM_32},
	{ICVTPD2PI, {0x0f, 0x2d}, 2, REG_FIELD, 0, MM__XM_128},
	{ICVTSD2SI, {0x0f, 0x2d}, 2, REG_FIELD, 0, R_32_64__XM_64},
	{IUCOMISS, {0x0f, 0x2e}, 2, REG_FIELD, 0, X__XM_32},
	{IUCOMISD, {0x0f, 0x2e}, 2, REG_FIELD, 0, X__XM_64},
	{ICOMISS, {0x0f, 0x2f}, 2, REG_FIELD, 0, X__XM_32},
	{ICOMISD, {0x0f, 0x2f}, 2, REG_FIELD, 0, X__XM_64},
	{IPALIGNR_MM, {0x0f, 0x3a, 0x0f}, 3, REG_FIELD, 0, MM__MMM_64},
	{IPALIGNR, {0x0f, 0x3a, 0x0f}, 3, REG_FIELD, 0, X__XM_128},
	{IDPPS, {0x0f, 0x3a, 0x40}, 3, REG_FIELD, 0, X__XM_128},
	{IDPPD, {0x0f, 0x3a, 0x41}, 3, REG_FIELD, 0, X__XM_128},
	{ICMOVO, {0x0f, 0x40}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVNO, {0x0f, 0x41}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVB, {0x0f, 0x42}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVNB, {0x0f, 0x43}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVE, {0x0f, 0x44}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVNE, {0x0f, 0x45}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVBE, {0x0f, 0x46}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVA, {0x0f, 0x47}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVS, {0x0f, 0x48}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVNS, {0x0f, 0x49}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVP, {0x0f, 0x4a}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVNP, {0x0f, 0x4b}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVL, {0x0f, 0x4c}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVNL, {0x0f, 0x4d}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVLE, {0x0f, 0x4e}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{ICMOVG, {0x0f, 0x4f}, 2, REG_FIELD, 0, R_16_32_64__RM_16_32_64},
	{IMOVMSKPS, {0x0f, 0x50}, 2, REG_FIELD, 0, R_32_64__X},
	{IMOVMSKPD, {0x0f, 0x50}, 2, REG_FIELD, 0, R_32_64__X},
	{ISQRTPS, {0x0f, 0x51}, 2, REG_FIELD, 0, X__XM_128},
	{ISQRTSS, {0x0f, 0x51}, 2, REG_FIELD, 0, X__XM_32},
	{ISQRTPD, {0x0f, 0x51}, 2, REG_FIELD, 0, X__XM_128},
	{ISQRTSD, {0x0f, 0x51}, 2, REG_FIELD, 0, X__XM_64},
	{IRSQRTPS, {0x0f, 0x52}, 2, REG_FIELD, 0, X__XM_128},
	{IRSQRTSS, {0x0f, 0x52}, 2, REG_FIELD, 0, X__XM_32},
	{IRCPPS, {0x0f, 0x53}, 2, REG_FIELD, 0, X__XM_128},
	{IRCPSS, {0x0f, 0x53}, 2, REG_FIELD, 0, X__XM_32},
	{IANDPS, {0x0f, 0x54}, 2, REG_FIELD, 0, X__XM_128},
	{IANDPD, {0x0f, 0x54}, 2, REG_FIELD, 0, X__XM_128},
	{IANDNPS, {0x0f, 0x55}, 2, REG_FIELD, 0, X__XM_128},
	{IANDNPD, {0x0f, 0x55}, 2, REG_FIELD, 0, X__XM_128},
	{IORPS, {0x0f, 0x56}, 2, REG_FIELD, 0, X__XM_128},
	{IORPD, {0x0f, 0x56}, 2, REG_FIELD, 0, X__XM_128},
	{IXORPS, {0x0f, 0x57}, 2, REG_FIELD, 0, X__XM_128},
	{IXORPD, {0x0f, 0x57}, 2, REG_FIELD, 0, X__XM_128},
	{IADDPS, {0x0f, 0x58}, 2, REG_FIELD, 0, X__XM_128},
	{IADDSS, {0x0f, 0x58}, 2, REG_FIELD, 0, X__XM_32},
	{IADDPD, {0x0f, 0x58}, 2, REG_FIELD, 0, X__XM_128},
	{IADDSD, {0x0f, 0x58}, 2, REG_FIELD, 0, X__XM_64},
	{IMULPS, {0x0f, 0x59}, 2, REG_FIELD, 0, X__XM_128},
	{IMULSS, {0x0f, 0x59}, 2, REG_FIELD, 0, X__XM_32},
	{IMULPD, {0x0f, 0x59}, 2, REG_FIELD, 0, X__XM_128},
	{IMULSD, {0x0f, 0x59}, 2, REG_FIELD, 0, X__XM_64},
	{ICVTPS2PD, {0x0f, 0x5a}, 2, REG_FIELD, 0, X__XM_128},
	{ICVTPD2PS, {0x0f, 0x5a}, 2, REG_FIELD, 0, X__XM_128},
	{ICVTSS2SD, {0x0f, 0x5a}, 2, REG_FIELD, 0, X__XM_32},
	{ICVTSD2SS, {0x0f, 0x5a}, 2, REG_FIELD, 0, X__XM_64},
	{ICVTDQ2PS, {0x0f, 0x5b}, 2, REG_FIELD, 0, X__XM_128},
	{ICVTPS2DQ, {0x0f, 0x5b}, 2, REG_FIELD, 0, X__XM_128},
	{ICVTTPS2DQ, {0x0f, 0x5b}, 2, REG_FIELD, 0, X__XM_128},
	{ISUBPS, {0x0f, 0x5c}, 2, REG_FIELD, 0, X__XM_128},
	{ISUBSS, {0x0f, 0x5c}, 2, REG_FIELD, 0, X__XM_32},
	{ISUBPD, {0x0f, 0x5c}, 2, REG_FIELD, 0, X__XM_128},
	{ISUBSD, {0x0f, 0x5c}, 2, REG_FIELD, 0, X__XM_64},
	{IMINPS, {0x0f, 0x5d}, 2, REG_FIELD, 0, X__XM_128},
	{IMINSS, {0x0f, 0x5d}, 2, REG_FIELD, 0, X__XM_32},
	{IMINPD, {0x0f, 0x5d}, 2, REG_FIELD, 0, X__XM_128},
	{IMINSD, {0x0f, 0x5d}, 2, REG_FIELD, 0, X__XM_64},
	{IDIVPS, {0x0f, 0x5e}, 2, REG_FIELD, 0, X__XM_128},
	{IDIVSS, {0x0f, 0x5e}, 2, REG_FIELD, 0, X__XM_32},
	{IDIVPD, {0x0f, 0x5e}, 2, REG_FIELD, 0, X__XM_128},
	{IDIVSD, {0x0f, 0x5e}, 2, REG_FIELD, 0, X__XM_64},
	{IMAXPS, {0x0f, 0x5f}, 2, REG_FIELD, 0, X__XM_128},
	{IMAXSS, {0x0f, 0x5f}, 2, REG_FIELD, 0, X__XM_32},
	{IMAXPD, {0x0f, 0x5f}, 2, REG_FIELD, 0, X__XM_128},
	{IMAXSD, {0x0f, 0x5f}, 2, REG_FIELD, 0, X__XM_64},
	{IPUNPCKLBW, {0x0f, 0x60}, 2, REG_FIELD, 0, X__XM_128},
	{IPUNPCKLWD, {0x0f, 0x61}, 2, REG_FIELD, 0, X__XM_128},
	{IPUNPCKLDQ, {0x0f, 0x62}, 2, REG_FIELD, 0, X__XM_128},
	{IPACKSSWB, {0x0f, 0x63}, 2, REG_FIELD, 0, X__XM_128},
	{IPCMPGTB, {0x0f, 0x64}, 2, REG_FIELD, 0, X__XM_128},
	{IPCMPGTW, {0x0f, 0x65}, 2, REG_FIELD, 0, X__XM_128},
	{IPCMPGTD, {0x0f, 0x66}, 2, REG_FIELD, 0, X__XM_128},
	{IPACKUSWB, {0x0f, 0x67}, 2, REG_FIELD, 0, X__XM_128},
	{IPUNPCKHBW, {0x0f, 0x68}, 2, REG_FIELD, 0, X__XM_128},
	{IPUNPCKHWD, {0x0f, 0x69}, 2, REG_FIELD, 0, X__XM_128},
	{IPUNPCKHDQ, {0x0f, 0x6a}, 2, REG_FIELD, 0, X__XM_128},
	{IPACKSSDW, {0x0f, 0x6b}, 2, REG_FIELD, 0, X__XM_128},
	{IPUNPCKLQDQ, {0x0f, 0x6c}, 2, REG_FIELD, 0, X__XM_128},
	{IPUNPCKHQDQ, {0x0f, 0x6d}, 2, REG_FIELD, 0, X__XM_128},
	{IMOV_XMM, {0x0f, 0x6e}, 2, REG_FIELD, 0, X__RM_32_64},
	{IMOVDQA, {0x0f, 0x6f}, 2, REG_FIELD, 0, X__XM_128},
	{IMOVDQU, {0x0f, 0x6f}, 2, REG_FIELD, 0, X__XM_128},
	{IPSRLW, {0x0f, 0x71}, 2, NUM_FIELD, 2, X__IMM_8},
	{IPSRAW, {0x0f, 0x71}, 2, NUM_FIELD, 4, X__IMM_8},
	{IPSLLW, {0x0f, 0x71}, 2, NUM_FIELD, 6, X__IMM_8},
	{IPSRLD, {0x0f, 0x72}, 2, NUM_FIELD, 2, X__IMM_8},
	{IPSRAD, {0x0f, 0x72}, 2, NUM_FIELD, 4, X__IMM_8},
	{IPSLLD, {0x0f, 0x72}, 2, NUM_FIELD, 6, X__IMM_8},
	{IPSRLQ, {0x0f, 0x73}, 2, NUM_FIELD, 2, X__IMM_8},
	{IPSRLDQ, {0x0f, 0x73}, 2, NUM_FIELD, 3, X__IMM_8},
	{IPSLLQ, {0x0f, 0x73}, 2, NUM_FIELD, 6, X__IMM_8},
	{IPSLLDQ, {0x0f, 0x73}, 2, NUM_FIELD, 7, X__IMM_8},
	{IPCMPEQB, {0x0f, 0x74}, 2, REG_FIELD, 0, X__XM_128},
	{IPCMPEQW, {0x0f, 0x75}, 2, REG_FIELD, 0, X__XM_128},
	{IPCMPEQD, {0x0f, 0x76}, 2, REG_FIELD, 0, X__XM_128},
	{IHADDPD, {0x0f, 0x7c}, 2, REG_FIELD, 0, X__XM_128},
	{IHADDPS, {0x0f, 0x7c}, 2, REG_FIELD, 0, X__XM_128},
	{IHSUBPD, {0x0f, 0x7d}, 2, REG_FIELD, 0, X__XM_128},
	{IHSUBPS, {0x0f, 0x7d}, 2, REG_FIELD, 0, X__XM_128},
	{IMOV_XMM, {0x0f, 0x7e}, 2, REG_FIELD, 0, RM_32_64__X},
	{IMOV_XMM, {0x0f, 0x7e}, 2, REG_FIELD, 0, X__XM_64},
	{IMOVDQA, {0x0f, 0x7f}, 2, REG_FIELD, 0, XM_128__X},
	{IMOVDQU, {0x0f, 0x7f}, 2, REG_FIELD, 0, XM_128__X},

	{IMOV, {0x0f, 0xb6}, 2, REG_FIELD, 0, R_16_32_64__RM_8},
	{IMOV, {0x0f, 0xb7}, 2, REG_FIELD, 0, R_16_32_64__RM_16},

	{IMOVNTI, {0x0f, 0xc3}, 2, REG_FIELD, 0, M_32_64__R_32_64},

	{IADDSUBPD, {0x0f, 0xd0}, 2, REG_FIELD, 0, X__XM_128},
	{IADDSUBPS, {0x0f, 0xd0}, 2, REG_FIELD, 0, X__XM_128},
	{IPSRLW, {0x0f, 0xd1}, 2, REG_FIELD, 0, X__XM_128},
	{IPSRLD, {0x0f, 0xd2}, 2, REG_FIELD, 0, X__XM_128},
	{IPSRLQ, {0x0f, 0xd3}, 2, REG_FIELD, 0, X__XM_128},
	{IPADDQ, {0x0f, 0xd4}, 2, REG_FIELD, 0, X__XM_128},
	{IPMULLW, {0x0f, 0xd5}, 2, REG_FIELD, 0, X__XM_128},
	{IMOV_XMM, {0x0f, 0xd6}, 2, REG_FIELD, 0, XM_64__X},
	// {IMOVQ2DQ, {0x0f, 0xd6}, 2, REG_FIELD, 0, X__MM},
	// {IMOVDQ2Q, {0x0f, 0xd6}, 2, REG_FIELD, 0, MM__X},
	{IPMOVMSKB, {0x0f, 0xd7}, 2, REG_FIELD, 0, R_32_64__X},
	{IPSUBUSB, {0x0f, 0xd8}, 2, REG_FIELD, 0, X__XM_128},
	{IPSUBUSW, {0x0f, 0xd9}, 2, REG_FIELD, 0, X__XM_128},
	{IPMINUB, {0x0f, 0xda}, 2, REG_FIELD, 0, X__XM_128},
	{IPAND, {0x0f, 0xdb}, 2, REG_FIELD, 0, X__XM_128},
	{IPADDUSB, {0x0f, 0xdc}, 2, REG_FIELD, 0, X__XM_128},
	{IPADDUSW, {0x0f, 0xdd}, 2, REG_FIELD, 0, X__XM_128},
	{IPMAXUB, {0x0f, 0xde}, 2, REG_FIELD, 0, X__XM_128},
	{IPANDN, {0x0f, 0xdf}, 2, REG_FIELD, 0, X__XM_128},
	{IPAVGB, {0x0f, 0xe0}, 2, REG_FIELD, 0, X__XM_128},
	{IPSRAW, {0x0f, 0xe1}, 2, REG_FIELD, 0, X__XM_128},
	{IPSRAD, {0x0f, 0xe2}, 2, REG_FIELD, 0, X__XM_128},
	{IPAVGW, {0x0f, 0xe3}, 2, REG_FIELD, 0, X__XM_128},
	{IPMULHUW, {0x0f, 0xe4}, 2, REG_FIELD, 0, X__XM_128},
	{IPMULHW, {0x0f, 0xe5}, 2, REG_FIELD, 0, X__XM_128},
	{ICVTPD2DQ, {0x0f, 0xe6}, 2, REG_FIELD, 0, X__XM_128},
	{ICVTTPD2DQ, {0x0f, 0xe6}, 2, REG_FIELD, 0, X__XM_128},
	{ICVTDQ2PD, {0x0f, 0xe6}, 2, REG_FIELD, 0, X__XM_128},
	{IMOVNTDQ, {0x0f, 0xe7}, 2, REG_FIELD, 0, M_128__X},
	{IPSUBSB, {0x0f, 0xe8}, 2, REG_FIELD, 0, X__XM_128},
	{IPSUBSW, {0x0f, 0xe9}, 2, REG_FIELD, 0, X__XM_128},
	{IPMINSW, {0x0f, 0xea}, 2, REG_FIELD, 0, X__XM_128},
	{IPOR, {0x0f, 0xeb}, 2, REG_FIELD, 0, X__XM_128},
	{IPADDSB, {0x0f, 0xec}, 2, REG_FIELD, 0, X__XM_128},
	{IPADDSW, {0x0f, 0xed}, 2, REG_FIELD, 0, X__XM_128},
	{IPMAXSW, {0x0f, 0xee}, 2, REG_FIELD, 0, X__XM_128},
	{IPXOR, {0x0f, 0xef}, 2, REG_FIELD, 0, X__XM_128},
	{ILDDQU, {0x0f, 0xf0}, 2, REG_FIELD, 0, X__XM_128},
	{IPSLLW, {0x0f, 0xf1}, 2, REG_FIELD, 0, X__XM_128},
	{IPSLLD, {0x0f, 0xf2}, 2, REG_FIELD, 0, X__XM_128},
	{IPSLLQ, {0x0f, 0xf3}, 2, REG_FIELD, 0, X__XM_128},
	{IPMULUDQ, {0x0f, 0xf4}, 2, REG_FIELD, 0, X__XM_128},
	{IPMADDWD, {0x0f, 0xf5}, 2, REG_FIELD, 0, X__XM_128},
	{IPSADBW, {0x0f, 0xf6}, 2, REG_FIELD, 0, X__XM_128},
	{IMASKMOVDQU, {0x0f, 0xf7}, 2, REG_FIELD, 0, X__X},
	{IPSUBB, {0x0f, 0xf8}, 2, REG_FIELD, 0, X__XM_128},
	{IPSUBW, {0x0f, 0xf9}, 2, REG_FIELD, 0, X__XM_128},
	{IPSUBD, {0x0f, 0xfa}, 2, REG_FIELD, 0, X__XM_128},
	{IPSUBQ, {0x0f, 0xfb}, 2, REG_FIELD, 0, X__XM_128},
	{IPADDB, {0x0f, 0xfc}, 2, REG_FIELD, 0, X__XM_128},
	{IPADDW, {0x0f, 0xfd}, 2, REG_FIELD, 0, X__XM_128},
	{IPADDD, {0x0f, 0xfe}, 2, REG_FIELD, 0, X__XM_128},
};
const struct Cmnd cmnds3[] = {
	{IIMUL, {0x69}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64__IMM_16_32},
	{IIMUL, {0x6b}, 1, REG_FIELD, 0, R_16_32_64__RM_16_32_64__IMM_8},

	{IROUNDPS, {0x0f, 0x3a, 0x08}, 3, REG_FIELD, 0, X__XM_128__IMM_8},
	{IROUNDPD, {0x0f, 0x3a, 0x09}, 3, REG_FIELD, 0, X__XM_128__IMM_8},
	{IROUNDSS, {0x0f, 0x3a, 0x0a}, 3, REG_FIELD, 0, X__XM_32__IMM_8},
	{IROUNDSD, {0x0f, 0x3a, 0x0b}, 3, REG_FIELD, 0, X__XM_64__IMM_8},
	{IBLENDPS, {0x0f, 0x3a, 0x0c}, 3, REG_FIELD, 0, X__XM_128__IMM_8},
	{IBLENDPD, {0x0f, 0x3a, 0x0d}, 3, REG_FIELD, 0, X__XM_128__IMM_8},
	{IPBLENDW, {0x0f, 0x3a, 0x0e}, 3, REG_FIELD, 0, X__XM_128__IMM_8},
	{IPEXTRB, {0x0f, 0x3a, 0x14}, 3, REG_FIELD, 0, R_32_64_M_8__X__IMM_8},
	{IPEXTRW, {0x0f, 0x3a, 0x15}, 3, REG_FIELD, 0, R_32_64_M_16__X__IMM_8},
	{IPEXTRDQ, {0x0f, 0x3a, 0x16}, 3, REG_FIELD, 0, RM_32_64__X__IMM_8},
	{IEXTRACTPS, {0x0f, 0x3a, 0x17}, 3, REG_FIELD, 0, RM_32__X__IMM_8},
	{IPINSRB, {0x0f, 0x3a, 0x20}, 3, REG_FIELD, 0, X__R_32_64_M_8__IMM_8},
	{IINSERTPS, {0x0f, 0x3a, 0x21}, 3, REG_FIELD, 0, X__XM_32__IMM_8},
	{IPINSRDQ, {0x0f, 0x3a, 0x22}, 3, REG_FIELD, 0, X__RM_32_64__IMM_8},
	{IMPSADBW, {0x0f, 0x3a, 0x42}, 3, REG_FIELD, 0, X__XM_128__IMM_8},
	{IPCMPESTRM, {0x0f, 0x3a, 0x60}, 3, REG_FIELD, 0, X__XM_128__IMM_8},
	{IPCMPESTRI, {0x0f, 0x3a, 0x61}, 3, REG_FIELD, 0, X__XM_128__IMM_8},
	{IPCMPISTRM, {0x0f, 0x3a, 0x62}, 3, REG_FIELD, 0, X__XM_128__IMM_8},
	{IPCMPISTRI, {0x0f, 0x3a, 0x63}, 3, REG_FIELD, 0, X__XM_128__IMM_8},
	{IPSHUFLW, {0x0f, 0x70}, 2, REG_FIELD, 0, X__XM_128__IMM_8},
	{IPSHUFHW, {0x0f, 0x70}, 2, REG_FIELD, 0, X__XM_128__IMM_8},
	{IPSHUFD, {0x0f, 0x70}, 2, REG_FIELD, 0, X__XM_128__IMM_8},

	{ICMPPS, {0x0f, 0xc2}, 2, REG_FIELD, 0, X__XM_128__IMM_8},
	{ICMPSS, {0x0f, 0xc2}, 2, REG_FIELD, 0, X__XM_32__IMM_8},
	{ICMPPD, {0x0f, 0xc2}, 2, REG_FIELD, 0, X__XM_128__IMM_8},
	{ICMPSD_XMM, {0x0f, 0xc2}, 2, REG_FIELD, 0, X__XM_64__IMM_8},
	{IPINSRW, {0x0f, 0xc4}, 2, REG_FIELD, 0, X__R_32_64_M_16__IMM_8},
	{ISHUFPS, {0x0f, 0xc6}, 2, REG_FIELD, 0, X__XM_128__IMM_8},
	{ISHUFPD, {0x0f, 0xc6}, 2, REG_FIELD, 0, X__XM_128__IMM_8},
};

const long cmnds0_len = lenofarr(cmnds0);
const long cmnds1_len = lenofarr(cmnds1);
const long cmnds2_len = lenofarr(cmnds2);
const long cmnds3_len = lenofarr(cmnds3);

const char *const WRONG_INST_OPS =
	"Для данной инструкции и выражений не было найдено подходящего кода, "
	"возможно выражения были неверные.";

const struct Cmnd *get_cmnd(struct Ipcd *i, const struct Cmnd cmnds[],
							const long cmnds_len, enum OpsCode code) {
	const struct Cmnd *c = 0, *ct;
	for (long j = 0; j < cmnds_len; j++) {
		ct = cmnds + j;
		if (ct->inst == i->in->code && ct->opsc == code) {
			c = ct;
			break;
		}
	}
	if (c == 0) {
		if (i->debug)
			printf("### инструкция %d, типы операндов %d\n", i->in->code, code);
		ee(i->in->f, i->in->p, WRONG_INST_OPS);
	}
	return c;
}

void get_zero_ops_code(struct Ipcd *i) {
	// TODO: could propably optimize OPC_INVALID but not sure for varops
	const struct Cmnd *c = get_cmnd(i, cmnds0, cmnds0_len, OPC_INVALID);
	blat(i->cmd, (uc *)c->cmd, c->len);
}

void get_align(struct Ipcd *i, int size, int align, int value) {
	// there is a thing bout nops at 0F 18 - 0F 1F
	// but im not sure, maybe there is a better way to align
	// TODO: see if there is better nop for align
	if (value == -1)
		value = ((const struct Cmnd *)cmnds0)->cmd[0]; // nop
	int len = (align - (size % align)) % align;

	for (int j = 0; j < len; j++)
		blist_add(i->cmd, value);
}

const char *const WARN_IMM_SIZE_WILL_BE_CHANGED =
	"Размер числа был вбайт, но данный тип инструкций не "
	"поддерживает числа таких размеров, поэтому скорее всего размер числа "
	"будет урезан.";
const char *const WARN_CHANGE_MEM_SIZE =
	"Размер адресанта не был верен, но при "
	"этом был явно указан, его размер будет изменен.";
const char *const WARN_CHANGE_IMM_SIZE =
	"Размер числа был явно указан но будет изменен.";
const char *const ERR_WRONG_OPS_FOR_THIS_INST =
	"Неверные выражения для данного типа инструкции."
	"(МОЖЕТ ПРОСТО НЕ ДОДЕЛАНО ПОКА)";
const char *const ERR_WRONG_BYTE_REG =
	"Данная инструкция не поддерживает регистры размером байт.";

void change_mem_size(struct Inst *in, struct Oper *o, uc sz) {
	if (o->forsed_sz && o->sz != sz)
		pw(in->f, in->p, WARN_CHANGE_MEM_SIZE);
	o->sz = sz;
}
void change_imm_size(struct Inst *in, struct Oper *o, uc sz) {
	if (o->forsed_sz && o->sz != sz)
		pw(in->f, in->p, WARN_CHANGE_IMM_SIZE);
	o->sz = sz;
}
int try_change_imm_to_byte(struct Inst *in, struct Oper *o) {
	if (is_imm_can_be_a_byte(o)) {
		if (o->forsed_sz && o->sz != BYTE)
			pw(in->f, in->p, WARN_CHANGE_IMM_SIZE);
		o->sz = BYTE;
		return 1;
	}
	return 0;
}

#define change_size_lr(in, l, r) (change_imm_size((in), (r), (l)->sz))

void change_m_sz(struct Inst *in, struct Oper *r, struct Oper *rm) {
	if (is_mem(rm) && rm->sz != r->sz)
		change_mem_size(in, rm, r->sz);
	else if (rm->sz != r->sz)
		ee(in->f, in->p, REGS_SIZES_NOT_MATCH);
}
void warn_change_to_eq_size_lr(struct Inst *i, struct Oper *l, struct Oper *r) {
	if (l->sz != r->sz)
		change_size_lr(i, l, r);
}
int warn_change_size_lr(struct Inst *in, struct Oper *l, struct Oper *r) {
	if (l->sz < r->sz) {
		change_size_lr(in, l, r);
		return 1;
	}
	return 0;
}

struct PList *get_all_ops_codes(enum ICode code) {
	const struct Cmnd *cmnd;
	struct PList *all_ops_codes = new_plist(32);
	long ops_code;
	uint32_t i;

#define take_ops_on_arr(num)                                                   \
	for (i = 0; i < cmnds##num##_len; i++) {                                   \
		cmnd = cmnds##num + i;                                                 \
		if (cmnd->inst == code) {                                              \
			ops_code = cmnd->opsc;                                             \
			plist_add(all_ops_codes, (void *)ops_code);                        \
		}                                                                      \
	}
	take_ops_on_arr(1);
	take_ops_on_arr(2);
	take_ops_on_arr(3);

	return all_ops_codes;
}

const char *const OPS_CODE_INVALID_ =
	"Не удалось определить типы выражений для инструкции, возможно она или "
	"операнды были неверны.";
const char *const AND_POSSIBLE_ARE =
	"Вот возможные типы операндов для данной инструкции:\n";

void ee_with_inst_code(struct Inst *in) {
	struct PList *all_ops_codes = get_all_ops_codes(in->code);
	const char *opsc_text;
	uint32_t i;

	fprintf(stderr, "%s%s:%d:%d %sОШИБКА: %s\n", COLOR_WHITE, in->f->path,
			in->p->line, in->p->col, COLOR_RED, OPS_CODE_INVALID_);
	print_source_line(in->f->code, in->p->line, COLOR_LIGHT_RED);
	fprintf(stderr, "%s%s", COLOR_GREEN, AND_POSSIBLE_ARE);

	for (i = 0; i < all_ops_codes->size; i++) {
		opsc_text = get_ops_text((long)plist_get(all_ops_codes, i));
		printf("  * %s\n", opsc_text);
	}

	printf("%s", COLOR_RESET);
	exit(1);
}
