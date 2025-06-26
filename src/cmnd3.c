#include "cmnd.h"

void get_tri_ops_code(struct Ipcd *i) {
	i->l = plist_get(i->in->os, 0);
	i->r = plist_get(i->in->os, 1);
	i->o = plist_get(i->in->os, 2);

	enum OpsCode code = get_tri_opscode(i);
	i->c = get_cmnd(i, cmnds3, cmnds3_len, code);
	get_tri_ops_prefs(i, code);
	fill_tri_ops_cmd_and_data(i);
}

const char *const TRI_OPS_DIDNT_EXISTS =
	"НЕ СУЩЕСТВУЮЩИЙ ТИП КОМАНД ДЛЯ ДАННЫХ ВЫРАЖЕНИЙ(трёх).";

void fill_tri_ops_cmd_and_data(struct Ipcd *i) {
	const struct Cmnd *c = i->c;
	struct Oper *l = i->l, *r = i->r, *o = i->o, *rm, *reg;
	uc modrm = 0;

	blat(i->cmd, (uc *)c->cmd, c->len);
	if (c->o == REG_FIELD) {
		//  2. REG_FIELD r indicates that the ModR/M byte contains a
		//  register
		//  - operand and an r/m operand
		if (is_r__rm(c->opsc) || is_xr__xrm(c->opsc)) {
			reg = l;
			rm = r;
		} else if (is_rm__r(c->opsc) || is_xrm__xr(c->opsc)) {
			reg = r;
			rm = l;
		} else
			ee(i->in->f, i->in->p, "REG_FIELD ЭЭЭ");

		modrm += rm->mod << 6;				  // mod
		modrm += get_reg_field(reg->rm) << 3; // r
		modrm += get_reg_field(rm->rm);		  // rm
		blist_add(i->cmd, modrm);
		if (is_mem(rm))
			add_mem(i, rm);

		if (is_imm(o)) // i beleive that o is always imm
			add_imm_data(i, o);
		else
			ee(i->in->f, i->in->p, "не imm эээ");
	} else
		ee(i->in->f, i->in->p, TRI_OPS_DIDNT_EXISTS);
}

void c_x_xm_i(enum OpsCode *code, struct Ipcd *i, uc mem_sz,
			  enum OpsCode set_code) {
	if (!(is_xmm(i->l) && is_xm(i->r) && is_imm(i->o)))
		return;
	if (is_mem(i->r))
		change_mem_size(i->in, i->r, mem_sz);
	change_imm_size(i->in, i->o, BYTE);
	*code = set_code;
}

enum OpsCode get_tri_opscode(struct Ipcd *i) {
	enum OpsCode code = OPC_INVALID;
	struct Oper *l = i->l, *r = i->r, *o = i->o;
	struct Inst *in = i->in;

	switch (in->code) {
	case IIMUL:
		if (!(is_reg(l) && is_rm(r) && is_imm(o)))
			break;

		if (is_8(l))
			ee(in->f, in->p, ERR_WRONG_BYTE_REG);
		change_m_sz(in, l, r);

		if (try_change_imm_to_byte(in, o))
			code = R_16_32_64__RM_16_32_64__IMM_8;
		else {
			change_imm_size(in, o, is_64(l) ? DWORD : l->sz);
			code = R_16_32_64__RM_16_32_64__IMM_16_32;
		}
		break;
	case IROUNDPS:
	case IROUNDPD:
	case IBLENDPS:
	case IBLENDPD:
	case IPBLENDW:
	case IPCMPESTRM:
	case IPCMPESTRI:
	case IPCMPISTRM:
	case IPCMPISTRI:
	case IMPSADBW:
	case IPSHUFLW:
	case IPSHUFHW:
	case IPSHUFD:
	case ISHUFPD:
	case ICMPPS:
	case ICMPPD:
	case ISHUFPS:
		c_x_xm_i(&code, i, XWORD, X__XM_128__IMM_8);
		break;
	case IROUNDSS:
	case ICMPSS:
		c_x_xm_i(&code, i, DWORD, X__XM_32__IMM_8);
		break;
	case IROUNDSD:
	case ICMPSD_XMM:
		c_x_xm_i(&code, i, QWORD, X__XM_64__IMM_8);
		break;
	case IPEXTRB:
		if (!(is_xmm(r) && is_imm(o)))
			break;
		change_imm_size(in, o, BYTE);
		if (is_reg(l) && (is_32(l) || is_64(l)))
			code = R_32_64_M_8__X__IMM_8;
		else if (is_mem(l)) {
			change_mem_size(in, l, BYTE);
			code = R_32_64_M_8__X__IMM_8;
		}
		break;
	case IPEXTRW:
		if (!(is_xmm(r) && is_imm(o)))
			break;
		change_imm_size(in, o, BYTE);
		if (is_reg(l) && (is_32(l) || is_64(l)))
			code = R_32_64_M_16__X__IMM_8;
		else if (is_mem(l)) {
			change_mem_size(in, l, WORD);
			code = R_32_64_M_16__X__IMM_8;
		}
		break;
	case IPEXTRDQ:
		if (!(is_rm(l) && is_xmm(r) && is_imm(o)))
			break;
		change_imm_size(in, o, BYTE);
		if (l->sz < DWORD)
			ee(in->f, in->p, EXPEXTED_DWORD_OR_QWORD);
		code = RM_32_64__X__IMM_8;
		break;
	case IEXTRACTPS:
		if (is_rm(l) && is_xmm(r) && is_imm(o)) {
			if (is_mem(l))
				change_mem_size(in, r, DWORD);
			else if (is_reg(l) && !is_32(l))
				break;
			change_imm_size(in, o, BYTE);
			code = RM_32__X__IMM_8;
		}
		break;
	case IPINSRB:
		if (!(is_xmm(l) && is_imm(o)))
			break;
		change_imm_size(in, o, BYTE);
		if (is_reg(r) && (is_32(r) || is_64(r)))
			code = X__R_32_64_M_8__IMM_8;
		else if (is_mem(r)) {
			change_mem_size(in, r, BYTE);
			code = X__R_32_64_M_8__IMM_8;
		}
		break;
	case IPINSRW:
		if (!(is_xmm(l) && is_imm(o)))
			break;
		change_imm_size(in, o, BYTE);
		if (is_reg(r) && (is_32(r) || is_64(r)))
			code = X__R_32_64_M_16__IMM_8;
		else if (is_mem(r)) {
			change_mem_size(in, r, WORD);
			code = X__R_32_64_M_16__IMM_8;
		}
		break;
	case IINSERTPS:
		if (is_xmm(l) && is_xm(r) && is_imm(o)) {
			if (is_mem(r))
				change_mem_size(in, r, DWORD);
			change_imm_size(in, o, BYTE);
			code = X__XM_32__IMM_8;
		}
		break;
	case IPINSRDQ:
		if (!(is_xmm(l) && is_rm(r) && is_imm(o)))
			break;
		change_imm_size(in, o, BYTE);
		if (r->sz < DWORD)
			ee(in->f, in->p, EXPEXTED_DWORD_OR_QWORD);
		code = X__RM_32_64__IMM_8;
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
