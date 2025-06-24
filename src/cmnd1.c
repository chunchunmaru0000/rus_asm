#include "cmnd.h"

#define get_first_o(in) (plist_get((in)->os, 0))

const char *const WARN_CHANGE_IMM_32_SIZE =
	"Размер выражения не был равен чбайт, но при "
	"этом был явно указан, его размер будет изменен под чбайт.";
const char *const ERR_REG_NOT_16_OR_64 =
	"Для данного типа инструкций поддерживаются только регистры размеров "
	"дбайт "
	"и вбайт.";

void get_one_ops_code(struct Ipcd *i) {
	enum OpsCode code = get_one_opscode(i->in);
	i->c = get_cmnd(i, cmnds1, cmnds1_len, code);
	get_one_ops_prefs(i, code);
	fill_one_ops_cmd_and_data(i);
}

enum OpsCode get_one_opscode(struct Inst *in) {
	enum OpsCode code = OPC_INVALID;
	struct Oper *o = get_first_o(in);
	switch (in->code) {
	case ILOOPNZ:
	case ILOOPZ:
	case ILOOP:
	case IJECXZ:
	case IJRCXZ:
	case IINT:
		if (is_imm(o)) {
			change_imm_size(in, o, BYTE);
			if (in->code == IINT)
				code = __IMM_8;
			else
				code = __REL_8;
		}
		break;
	case IRET:
	case IRETF:
		if (is_imm(o)) {
			change_imm_size(in, o, WORD);
			code = __IMM_16;
		}
		break;
	case IROL:
	case IROR:
	case IRCL:
	case IRCR:
	case ISHL:
	case ISHR:
	case ISAR:
	case IROL1:
	case IROR1:
	case IRCL1:
	case IRCR1:
	case ISHL1:
	case ISHR1:
	case ISAR1:
	case INOT:
	case INEG:
	case IINC:
	case IDEC:
	case IMUL:
	case IIMUL:
	case IDIV:
	case IIDIV:
		if (is_rm(o))
			code = is_8(o) ? __RM_8 : __RM_16_32_64;
		break;
	case IJO:
	case IJNO:
	case IJB:
	case IJNB:
	case IJE:
	case IJNE:
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
	case ICALL:
		if (is_imm(o)) {
			if (in->code == ICALL) {
				if (o->sz != DWORD && o->forsed_sz)
					pw(in->f, in->p, WARN_CHANGE_IMM_32_SIZE);
				o->sz = DWORD;
				code = __REL_32;
			} else if (is_8(o))
				code = __REL_8;
			else if (is_32(o))
				code = __REL_32;
			else {
				pw(in->f, in->p, WARN_CHANGE_IMM_32_SIZE);
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
				ee(in->f, in->p, ERR_REG_NOT_16_OR_64);
		} else if (is_mem(o)) {
			if (is_16(o) || is_64(o))
				code = __RM_16_64;
		} else if (is_imm(o)) {
			if (is_32(o))
				code = __IMM_32;
			else if (is_8(o))
				code = __IMM_8;
			else {
				pw(in->f, in->p, WARN_CHANGE_IMM_32_SIZE);
				o->sz = DWORD;
				code = __IMM_32;
			}
		}
		break;
	default:
		ee(in->f, in->p, ERR_WRONG_OPS_FOR_THIS_INST);
	}
	if (code == OPC_INVALID)
		ee(in->f, in->p, OPS_CODE_INVALID);
	return code;
}

void get_one_ops_prefs(struct Ipcd *i, enum OpsCode ops) {
	struct Oper *o = get_first_o(i->in);
	// 67 Address-size OVERRIRE prefix, when adress 32-bit like [eax]
	if (is_addr32(o))
		blist_add(i->cmd, 0x67);
	// 66 16-bit Operand-size OVERRIRE prefix
	if (!is_seg(o) && is_16(o) &&
		!(i->in->code == IRET || i->in->code == IRETF))
		blist_add(i->cmd, 0x66);
	// REX prefixes
	uc rex = 0b01000000;
	if (ops == __RM_16_32_64 && is_64(o))
		rex |= REX_W;
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
		// - this "ModR/M" byte also have mod and if its just reg and imm
		// then
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
			ee(i->in->f, i->in->p, "а э ээ ээээ ээ да");
	} else if (c->o == REG_FIELD) {
		// 2. REG_FIELD r indicates that the ModR/M byte contains a register
		// - operand and an r/m operand
		ee(i->in->f, i->in->p,
		   "REG_FIELD наверно не существует для операций с одним "
		   "выражением");
	} else if (c->o == PLUS_REGF) {
		// 3. PLUS_REGF When just op code + reg code: PUSH r64/16, POP
		// r64/16
		*(i->cmd->st + i->cmd->size - 1) += get_reg_field(o->rm);
	} else
		ee(i->in->f, i->in->p, "у меня муха щас над столом летает");
}
