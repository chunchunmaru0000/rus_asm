#include "cmnd.h"

void get_tri_ops_code(struct Ipcd *i) {
	enum OpsCode code = get_tri_opscode(i->in);
	i->c = get_cmnd(i, cmnds3, cmnds3_len, code);
	get_tri_ops_prefs(i, code);
	fill_tri_ops_cmd_and_data(i);
}

const char *const TRI_OPS_DIDNT_EXISTS =
	"НЕ СУЩЕСТВУЮЩИЙ ТИП КОМАНД ДЛЯ ДАННЫХ ВЫРАЖЕНИЙ(трёх).";

void fill_tri_ops_cmd_and_data(struct Ipcd *i) {
	const struct Cmnd *c = i->c;
	struct Oper *l = plist_get(i->in->os, 0);
	struct Oper *r = plist_get(i->in->os, 1);
	struct Oper *o = plist_get(i->in->os, 2);
	uc modrm = 0;

	blat(i->cmd, (uc *)c->cmd, c->len);
	if (c->o == REG_FIELD) {
		//  2. REG_FIELD r indicates that the ModR/M byte contains a
		//  register
		//  - operand and an r/m operand
		if (is_r__rm(c->opsc)) {
			modrm += r->mod << 6;				// mod
			modrm += get_reg_field(l->rm) << 3; // r
			modrm += get_reg_field(r->rm);		// rm
			blist_add(i->cmd, modrm);
			if (is_mem(r))
				add_mem(i, r);
			// R_16_32_64__RM_16_32_64__IMM_16_32
			// R_16_32_64__RM_16_32_64__IMM_8
			add_imm_data(i, o);
		} else
			ee(i->in->f, i->in->p, "REG_FIELD ЭЭЭ");
	} else
		ee(i->in->f, i->in->p, TRI_OPS_DIDNT_EXISTS);
}

enum OpsCode get_tri_opscode(struct Inst *in) {
	enum OpsCode code = OPC_INVALID;
	struct Oper *l = plist_get(in->os, 0);
	struct Oper *r = plist_get(in->os, 1);
	struct Oper *o = plist_get(in->os, 2);

	switch (in->code) {
	case IIMUL:
		if (is_reg(l) && is_rm(r) && is_imm(o)) {
			if (is_8(l))
				ee(in->f, in->p, ERR_WRONG_BYTE_REG);

			change_m_sz(in, l, r);

			if (is_64(o)) {
				if (o->forsed_sz)
					pw(in->f, in->p, WARN_CHANGE_IMM_SIZE);
				o->sz = is_64(l) ? DWORD : l->sz;
			}
			if (is_imm_can_be_a_byte(o)) {
				if (o->forsed_sz)
					pw(in->f, in->p, WARN_CHANGE_IMM_SIZE);
				o->sz = BYTE;
			}
			code = is_8(o) ? R_16_32_64__RM_16_32_64__IMM_8
						   : R_16_32_64__RM_16_32_64__IMM_16_32;
		}
		break;
	default:
		ee(in->f, in->p, ERR_WRONG_OPS_FOR_THIS_INST);
	}
	return code;
}

void get_tri_ops_prefs(struct Ipcd *i, enum OpsCode ops) {
	// i beleive there is no three regs op code in x64
	get_two_ops_prefs(i, ops);
}
