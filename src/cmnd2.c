#include "cmnd.h"

void get_two_ops_code(struct Ipcd *i) {
	i->l = plist_get(i->in->os, 0);
	i->r = plist_get(i->in->os, 1);

	enum OpsCode code = get_two_opscode(i);
	i->c = get_cmnd(i, cmnds2, cmnds2_len, code);
	get_two_ops_prefs(i, code);
	fill_two_ops_cmd_and_data(i);
}

void fill_two_ops_cmd_and_data(struct Ipcd *i) {
	struct Oper *l = i->l, *r = i->r;
	const struct Cmnd *c = i->c;
	uc modrm = 0;

	blat(i->cmd, (uc *)c->cmd, c->len);
	// o Register/ Opcode Field
	if (c->o == NOT_FIELD) {
		//   0. NOT_FIELD just op code
		if (i->in->code == IENTER)
			add_imm_data(i, l);
		if (is_imm(r) || is_moffs(r))
			add_imm_data(i, r);
		else if (is_imm(l) || is_moffs(l))
			add_imm_data(i, l);
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
			ee(i->in->f, i->in->p, "только числа пока");
	} else if (c->o == REG_FIELD) {
		//   2. REG_FIELD r indicates that the ModR/M byte contains a register
		//   - operand and an r/m operand. 00 ADD
		//   - - primary used with
		//   - - - r__rm or rm__r, lea uses r__m
		//   - - - also sreg__rm or rm__sreg
		//   - - - also xmm, CRn, DRn and maybe more shit
		if (is_rm__r(c->opsc) || is_xrm__xr(c->opsc)) {
			modrm += l->mod << 6;				// mod
			modrm += get_reg_field(r->rm) << 3; // r
			modrm += get_reg_field(l->rm);		// rm
			blist_add(i->cmd, modrm);
			if (is_mem(l))
				add_mem(i, l);
		} else if (is_r__rm(c->opsc) || is_xr__xrm(c->opsc)) {
			modrm += r->mod << 6;				// mod
			modrm += get_reg_field(l->rm) << 3; // r
			modrm += get_reg_field(r->rm);		// rm
			blist_add(i->cmd, modrm);
			if (is_mem(r))
				add_mem(i, r);
		} else
			ee(i->in->f, i->in->p, "только р__рм и рм__р пока");
	} else if (c->o == PLUS_REGF) {
		//   3. PLUS_REGF When just op code + reg code
		//   - BSWAP, PUSH r64/16, POP r64/16, XCHG r16/32/64 rAX
		//   - B0+r MOV r8 imm8, B8+r MOV r16/32/64 imm16/32/64
		*(i->cmd->st + i->cmd->size - 1) += get_reg_field(l->rm);
		if (i->in->code == IMOV)
			add_imm_data(i, r);
	} else
		ee(i->in->f, i->in->p, "че не так то");
}

void get_xm_xm_code(enum OpsCode *code, struct Inst *in, struct Oper *l,
					struct Oper *r, enum OpsCode x_xm, enum OpsCode xm_x,
					uc sz) {
	if (is_xmm(l) && is_xm(r)) {
		if (is_mem(r))
			change_mem_size(in, r, sz);
		*code = x_xm;
	} else if (is_mem(l) && is_xmm(r)) {
		change_mem_size(in, l, sz);
		*code = xm_x;
	}
}
void get_r3264_xm(enum OpsCode *code, struct Inst *in, struct Oper *l,
				  struct Oper *r, enum OpsCode oc, uc sz) {
	if (is_reg(l) && (is_32(l) || is_64(l)) && is_xm(r)) {
		if (is_mem(r))
			change_mem_size(in, r, sz);
		*code = oc;
	}
}
void get_mm_xm(enum OpsCode *code, struct Inst *in, struct Oper *l,
			   struct Oper *r, enum OpsCode oc, uc sz) {
	if (is_mm(l) && is_xm(r)) {
		if (is_mem(r))
			change_mem_size(in, r, sz);
		*code = oc;
	}
}
void get_mm_mmm(enum OpsCode *code, struct Inst *in, struct Oper *l,
				struct Oper *r, enum OpsCode oc, uc sz) {
	if (is_mm(l) && is_mmm(r)) {
		if (is_mem(r))
			change_mem_size(in, r, sz);
		*code = oc;
	}
}

// TODO: for example
// зумн ебх ебх 2 ; работает
// зумн ебх 2     ; ошибка, но опкод то один
enum OpsCode get_two_opscode(struct Ipcd *i) {
	struct Oper *l = i->l, *r = i->r;
	struct Inst *in = i->in;

	enum OpsCode code = OPC_INVALID;
	switch (in->code) {
	case IIMUL:
		if (is_reg(l) && is_rm(r) && !is_8(l)) {
			warn_change_to_eq_size_lr(in, l, r);
			code = R_16_32_64__RM_16_32_64;
		}
		break;
	case IXCHG:
		if (is_reg(l)) {
			if (is_rA(r)) {
				if (l->sz != r->sz)
					ee(in->f, in->p, REG_MEM_IMM_SIZES_NOT_MATCH);
				code = R_16_32_64__RAX;
			} else if (is_rm(r)) {
				change_m_sz(in, l, r);
				code = is_8(l) ? R_8__RM_8 : R_16_32_64__RM_16_32_64;
			}
		}
		break;
	case ILEA:
		if (is_reg(l) && !is_8(l) && is_mem(r)) {
			change_m_sz(in, l, r);
			code = R_16_32_64__M;
		}
		break;
	case IENTER:
		if (is_imm(l) && is_imm(r)) {
			change_imm_size(in, l, WORD);
			change_imm_size(in, r, BYTE);
			code = IMM_16__IMM_8;
		}
		break;
	case IROL:
	case IROR:
	case IRCL:
	case IRCR:
	case ISHL:
	case ISHR:
	case ISAR:
		if (is_rm(l) && is_imm(r)) {
			change_imm_size(in, r, BYTE);
			code = is_8(l) ? RM_8__IMM_8 : RM_16_32_64__IMM_8;
		}
		break;
	case IOUTPUT:
		if ((is_imm(l) || is_dx(l)) && (is_al(r) || is_eA(r))) {
			if (is_imm(l)) {
				change_imm_size(in, l, BYTE);
				code = is_8(r) ? IMM_8__AL : IMM_8__EAX;
			} else
				code = is_8(r) ? DX__AL : DX__EAX;
		}
		break;
	case IINPUT:
		if ((is_al(l) || is_eA(l)) && (is_imm(r) || is_dx(r))) {
			if (is_imm(r)) {
				change_imm_size(in, r, BYTE);
				code = is_8(l) ? AL__IMM_8 : EAX__IMM_8;
			} else
				code = is_8(l) ? AL__DX : EAX__DX;
		}
		break;
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
			if (r->sz == QWORD)
				pw(in->f, in->p, WARN_IMM_SIZE_WILL_BE_CHANGED);

			if (warn_change_size_lr(in, l, r))
				;
			else if (l->sz != r->sz && !(l->sz == QWORD && r->sz == DWORD))
				change_size_lr(in, l, r);

			if (is_imm_can_be_a_byte(r) && !(in->code == ITEST && !is_8(l))) {
				if (r->forsed_sz)
					pw(in->f, in->p, WARN_CHANGE_IMM_SIZE);
				r->sz = BYTE;
			}
			if (l->sz != r->sz && !(is_64(l) && is_32(r)) &&
				!(!is_8(l) && is_8(r)))
				ee(in->f, in->p, REG_MEM_IMM_SIZES_NOT_MATCH);
			if (is_al(l))
				code = AL__IMM_8;
			else if (is_rA(l) && !is_8(r))
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
		if (is_reg(l) && is_64(l) && is_rm(r) && is_32(r))
			code = R_64__RM_32;
		else if (is_rm(l) && is_reg(r)) {
			change_m_sz(in, r, l);
			code = is_8(l) ? RM_8__R_8 : RM_16_32_64__R_16_32_64;
		} else if (is_reg(l) && is_rm(r)) {
			change_m_sz(in, l, r);
			code = is_8(l) ? R_8__RM_8 : R_16_32_64__RM_16_32_64;
		} else if (is_seg(r)) {
			if (is_mem(l)) {
				change_m_sz(in, r, l);
				code = M_16__SREG;
			} else if (is_reg(l) && !is_8(l))
				code = R_16_32_64__SREG;
		} else if (is_seg(l)) {
			change_m_sz(in, l, r);
			code = SREG__RM_16;
		} else if (is_al(l) && is_moffs(r)) {
			warn_change_to_eq_size_lr(in, l, r);
			code = AL__MOFFS_8;
		} else if (is_rA(l) && is_moffs(r)) {
			warn_change_to_eq_size_lr(in, l, r);
			code = RAX__MOFFS_16_32_64;
		} else if (is_moffs(l) && is_al(r)) {
			warn_change_to_eq_size_lr(in, r, l);
			code = MOFFS_8__AL;
		} else if (is_moffs(l) && is_rA(r)) {
			warn_change_to_eq_size_lr(in, r, l);
			code = MOFFS_16_32_64__RAX;
		} else if (is_imm(r)) {
			warn_change_size_lr(in, l, r);

			if (l->sz != r->sz && !(is_64(l) && is_32(r)))
				ee(in->f, in->p, REG_MEM_IMM_SIZES_NOT_MATCH);

			if (is_reg(l)) {
				if (is_8(l))
					code = R_8__IMM_8;
				else if (!is_8(l)) {
					if (l->sz == r->sz)
						code = R_16_32_64__IMM_16_32_64;
					else
						code = RM_16_32_64__IMM_16_32;
				}
			} else if (is_mem(l)) {
				if (is_64(l) && is_64(r))
					ee(in->f, in->p, MEM_IMM_SIZE_QWORD);

				if (is_8(l))
					code = RM_8__IMM_8;
				else if (!is_8(l))
					code = RM_16_32_64__IMM_16_32;
			}
		}
		break;
	case IMOV_XMM:
		if (is_xmm(l) && is_rm(r) && (is_32(r) || is_64(r)))
			code = X__RM_32_64;
		else if (is_rm(l) && (is_32(l) || is_64(l)) && is_xmm(r))
			code = RM_32_64__X;
		else if (is_xmm(l) && is_xm(r)) {
			if (is_mem(r))
				change_mem_size(in, r, DWORD);
			code = X__XM_64;
		} else if (is_xm(l) && is_xmm(r)) {
			if (is_mem(l))
				change_mem_size(in, l, DWORD);
			code = XM_64__X;
		}
		break;
	case ICMOVO:
	case ICMOVNO:
	case ICMOVB:
	case ICMOVNB:
	case ICMOVE:
	case ICMOVNE:
	case ICMOVBE:
	case ICMOVA:
	case ICMOVS:
	case ICMOVNS:
	case ICMOVP:
	case ICMOVNP:
	case ICMOVL:
	case ICMOVNL:
	case ICMOVLE:
	case ICMOVG:
		if (!(is_reg(l) && is_rm(r)) && !is_8(l))
			break;
		if (is_mem(r)) {
			change_mem_size(in, r, l->sz);
			code = R_16_32_64__RM_16_32_64;
		} else if (l->sz == r->sz)
			code = R_16_32_64__RM_16_32_64;
		break;
	case IMOVUPS:
	case IMOVUPD:
	case IUNPCKLPD:
	case IUNPCKHPD:
	case IMOVAPS:
	case IMOVAPD:
	case IPALIGNR:
	case IDPPS:
	case IDPPD:
	case ISQRTPS:
	case ISQRTPD:
	case IRSQRTPS:
	case IRCPPS:
	case IANDPS:
	case IANDPD:
	case IANDNPS:
	case IANDNPD:
	case IORPS:
	case IORPD:
	case IXORPS:
	case IXORPD:
	case IADDPS:
	case IADDPD:
	case IMULPS:
	case IMULPD:
	case ICVTPS2PD:
	case ICVTPD2PS:
	case ICVTDQ2PS:
	case ICVTPS2DQ:
	case ICVTTPS2DQ:
	case ISUBPS:
	case ISUBPD:
	case IMINPS:
	case IMINPD:
	case IDIVPS:
	case IDIVPD:
	case IMAXPS:
	case IMAXPD:
	case IPUNPCKLBW:
	case IPUNPCKLWD:
	case IPUNPCKLDQ:
	case IPACKSSWB:
	case IPCMPGTB:
	case IPCMPGTW:
	case IPCMPGTD:
	case IPACKUSWB:
	case IPUNPCKHBW:
	case IPUNPCKHWD:
	case IPUNPCKHDQ:
	case IPACKSSDW:
	case IPUNPCKLQDQ:
	case IPUNPCKHQDQ:
	case IPCMPEQB:
	case IPCMPEQW:
	case IPCMPEQD:
	case IHADDPD:
	case IHADDPS:
	case IHSUBPD:
	case IHSUBPS:
	case IMOVDQA:
	case IMOVDQU:
	case IADDSUBPD:
	case IADDSUBPS:
	case IPADDQ:
	case IPMULLW:
	case IPSUBUSB:
	case IPSUBUSW:
	case IPMINUB:
	case IPAND:
	case IPADDUSB:
	case IPADDUSW:
	case IPMAXUB:
	case IPANDN:
	case IPAVGB:
	case IPAVGW:
	case IPMULHUW:
	case IPMULHW:
	case ICVTPD2DQ:
	case ICVTTPD2DQ:
	case ICVTDQ2PD:
	case IPSUBSB:
	case IPSUBSW:
	case IPMINSW:
	case IPOR:
	case IPADDSB:
	case IPADDSW:
	case IPMAXSW:
	case IPXOR:
	case ILDDQU:
	case IPMULUDQ:
	case IPMADDWD:
	case IPSADBW:
	case IPSUBB:
	case IPSUBW:
	case IPSUBD:
	case IPSUBQ:
	case IPADDB:
	case IPADDW:
	case IPADDD:
		get_xm_xm_code(&code, in, l, r, X__XM_128, XM_128__X, XWORD);
		break;
	case IMOVSS:
	case IUCOMISS:
	case ICOMISS:
	case ISQRTSS:
	case IRSQRTSS:
	case IRCPSS:
	case IADDSS:
	case IMULSS:
	case ICVTSS2SD:
	case ISUBSS:
	case IMINSS:
	case IDIVSS:
	case IMAXSS:
		get_xm_xm_code(&code, in, l, r, X__XM_32, XM_32__X, DWORD);
		break;
	case IMOVSD_XMM:
	case IMOVDDUP:
	case IMOVSLDUP:
	case IUNPCKLPS:
	case IUNPCKHPS:
	case IMOVSHDUP:
	case IUCOMISD:
	case ICOMISD:
	case ISQRTSD:
	case IADDSD:
	case IMULSD:
	case ICVTSD2SS:
	case ISUBSD:
	case IMINSD:
	case IDIVSD:
	case IMAXSD:
		get_xm_xm_code(&code, in, l, r, X__XM_64, XM_64__X, QWORD);
		break;
	case IMOVHLPS:
	case IMOVLHPS:
	case IMASKMOVDQU:
		// here OPC_INVALID grants that if is_mem(l) then invalid ops code
		if (!is_mem(r))
			get_xm_xm_code(&code, in, l, r, X__X, OPC_INVALID, 0);
		break;
	case IMOVLPS:
	case IMOVLPD:
	case IMOVHPS:
	case IMOVHPD:
		get_xm_xm_code(&code, in, l, r, X__M_64, M_64__X, QWORD);
		break;
	case IMOVNTPS:
	case IMOVNTPD:
	case IMOVNTDQ:
		get_xm_xm_code(&code, in, l, r, OPC_INVALID, M_128__X, XWORD);
		break;
	case ICVTSI2SS:
	case ICVTSI2SD:
		if (is_xmm(l) && is_rm(r) && (is_32(r) || is_64(r)))
			code = X__RM_32_64;
		break;
	case IMOVMSKPS:
	case IMOVMSKPD:
	case IPMOVMSKB:
		if (is_reg(l) && (is_32(l) || is_64(l)) && is_xmm(r))
			code = R_32_64__X;
		break;
	case IPSRLW:
	case IPSRAW:
	case IPSLLW:
	case IPSRLD:
	case IPSRAD:
	case IPSLLD:
	case IPSRLQ:
	case IPSLLQ:
		if (!is_xmm(l))
			break;
		if (is_imm(r)) {
			change_imm_size(in, r, BYTE);
			code = X__IMM_8;
		} else if (is_xm(r)) {
			if (is_mem(r))
				change_mem_size(in, r, XWORD);
			code = X__XM_128;
		}
		break;
	case IPSRLDQ:
	case IPSLLDQ:
		if (is_xmm(l) && is_imm(r)) {
			change_imm_size(in, r, BYTE);
			code = X__IMM_8;
		}
		break;
	case ICVTPI2PS:
	case ICVTPI2PD:
		if (is_xmm(l) && is_mmm(r) && is_64(r))
			code = X__MMM_64;
		break;
	case ICVTTPS2PI:
	case ICVTPS2PI:
		get_mm_xm(&code, in, l, r, MM__XM_64, QWORD);
		break;
	case ICVTTSS2SI:
	case ICVTSS2SI:
		get_r3264_xm(&code, in, l, r, R_32_64__XM_32, DWORD);
		break;
	case ICVTTPD2PI:
	case ICVTPD2PI:
		get_mm_xm(&code, in, l, r, MM__XM_128, XWORD);
		break;
	case ICVTTSD2SI:
	case ICVTSD2SI:
		get_r3264_xm(&code, in, l, r, R_32_64__XM_64, QWORD);
		break;
	case IPALIGNR_MM:
		get_mm_mmm(&code, in, l, r, MM__MMM_64, QWORD);
		break;
	case IMOVNTI:
		if (is_mem(l) && is_reg(r) && r->sz >= DWORD) {
			warn_change_to_eq_size_lr(in, l, r);
			code = M_32_64__R_32_64;
		}
		break;
	default:
		ee(in->f, in->p, ERR_WRONG_OPS_FOR_THIS_INST);
	}
	if (code == OPC_INVALID)
		ee(in->f, in->p, OPS_CODE_INVALID);
	return code;
}

#define get_mov_xmm_pref(o)                                                    \
	((o) == X__RM_32_64 || (o) == RM_32_64__X ? 0x66 : 0xf3)

void get_two_ops_prefs(struct Ipcd *i, enum OpsCode code) {
	struct Oper *l = i->l, *r = i->r;

	// 67 Address-size OVERRIRE prefix, when adress 32-bit like [eax]
	if (is_addr32(l) || is_addr32(r))
		blist_add(i->cmd, 0x67);

	// REX prefixes
	uc rex = 0b01000000;

	// xmm prefixes
	enum ICode in_code = i->in->code;
	if (in_code > I_XMM_INSTRUCTIONS_BEGIN) {
		uc xmm_pref = 0x00;
		if (in_code == IMOV_XMM)
			xmm_pref = get_mov_xmm_pref(i->c->opsc);
		else if (is_66(in_code))
			xmm_pref = 0x66;
		else if (is_F2(in_code))
			xmm_pref = 0xf2;
		else if (is_F3(in_code))
			xmm_pref = 0xf3;

		if (xmm_pref)
			blist_add(i->cmd, xmm_pref);

		if ((is_reg(l) && is_64(l)) || (is_reg(r) && is_64(r)) ||
			(is_x__rm(code) && is_64(r)) || (is_rm__x(code) && is_64(l)))
			rex |= REX_W;
		if (is_xm_l(code)) {
			if (is_mem(l))
				rex |= l->rex; // get mem REX's
			else if (is_xr_new(l))
				rex |= REX_B; // Extension of ModR/M r/m
			if (is_xr_new(r))
				rex |= REX_R; // Extension of ModR/M reg
		} else if (is_xm_r(code)) {
			if (is_mem(r))
				rex |= r->rex; // get mem REX's
			else if (is_xr_new(r))
				rex |= REX_B; // Extension of ModR/M r/m
			if (is_xr_new(l))
				rex |= REX_R; // Extension of ModR/M reg
		} else if (is_imm_r(code)) {
			if (is_xm_l(code)) {
				if (is_mem(l))
					rex |= l->rex; // get mem REX's
				else if (is_xr_new(l))
					rex |= REX_B;	   // Extension of ModR/M r/m
			} else if (is_xr_new(l)) { // SUPPOSED TO BE REG
				if (i->c->o == PLUS_REGF)
					rex |= REX_B; // Extension of ModR/M r/m
				else
					rex |= REX_R; // Extension of ModR/M reg
			}
		}
	} else {
		// 66 16-bit Operand-size OVERRIRE prefix
		// mov M_16__SREG dont need cuz its always 16-bit
		if ((code == EAX__IMM_8 || code == EAX__DX) && is_16(l))
			blist_add(i->cmd, 0x66);
		else if ((code == IMM_8__EAX || code == DX__EAX) && is_16(r))
			blist_add(i->cmd, 0x66);
		else if (!is_seg(l) && is_16(l) && !(is_mem(l) && is_seg(r)) &&
				 !(code == IMM_16__IMM_8 || code == DX__EAX))
			blist_add(i->cmd, 0x66);

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
	}
	if (rex != 0b01000000)
		blist_add(i->cmd, rex);
}
