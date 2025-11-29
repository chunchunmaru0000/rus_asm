#include "pser.h"
#include <stdio.h>

char *ERR_WRONG_TOKEN = "Неверное выражение.";
char *ERR_WRONG_MINUS = "Минус можно использовать только перед числами.";
char *INVALID_STR_LEN =
	"Длина строки в выражении не может быть равна 0 или быть больше 2.";
char *TOO_MUCH_OS =
	"Слишком много или мало выражений в адресанте, максимум может быть "
	"4, минимум 1, в следующих порядках:\n"
	"\t[<регистр> <множитель: 1,2,4,8> регистр <смещение>]\n"
	"\t[множитель: 1,2,4,8 регистр регистр смещение]\n"
	"\t[смещение]\n";
char *POSSIBLE_WRONG_ORDER =
	"Возможно выражения в адресанте были в неверном порядке, "
	"всевозможные порядки:\n"
	"\t[<регистр> <множитель: 1,2,4,8> регистр <смещение>]\n"
	"\t[множитель: 1,2,4,8 регистр регистр смещение]\n"
	"\t[смещение]\n";
char *WRONG_ADDRES_OP = "Неверное выражение внутри адресанта, ими могут быть "
						"только регистры, метки или целые числа.";
char *WRONG_DISP = "Неверное смещение, ожидалось число 32 бит или метка.";
char *WRONG_SCALE =
	"Неверный множитель, ожидалось число(1, 2, 4, 8) или его отсутствие.";
char *WRONG_INDEX = "Неверный индекс, ожидался регистр.";
char *WRONG_RM = "Неверное выражение, ожидался регистр.";
char *WRONG_BASE = "Неверная основа, ожидался регистр.";
char *WRONG_MOFFS = "Неверное выражение, ожидалось число в качестве адреса.";
char *WRONG_ADDR_REG_SZ =
	"Неверный размер регистра в выражении операнда, адресными регистрами могут "
	"быть только 32-х или 64-х битные регистры.";
char *WRONG_MOFFS_MEM_SZ = "Неверный размер адреса, адресация в х64 может быть "
						   "только или 64 бит или 32 бит.";
// МИО - Множитель Индекс Основа
char *FORBIDDEN_RSP_INDEX =
	"Регистр рсп или есп запрещены в качестве индекса в МИО байте.";
char *DIFFERENT_SIZE_REGS = "Регистры адресанта не могут быть разных размеров, "
							"только или все 64 бит или 32 бит.";
char *CANT_CHANGE_REG_SZ =
	"Нельзя менять размер регистра, это ни к чему не приведет так и так.";

const char *STR_ADDR = "адр";
const char *STRS_SIZES[] = {"байт",	 "дбайт", "чбайт", "вбайт",
							"эбайт", "юбайт", "ябайт"};
const char *STR_RESERVED = "запас";

#define set_tc(tp, cp, t, c, sp, s)                                            \
	do {                                                                       \
		*(tp) = (t);                                                           \
		*(cp) = (c);                                                           \
		*(sp) = (s);                                                           \
	} while (0)
// void set_tc(struct Token **tp, enum OCode *cp, struct Token *t, enum
// OCode c) { *tp = t; *cp = c; }

int search_size(struct Pser *p, struct Oper **o, char *v) {
	for (uint32_t i = 0; i < lenofarr(STRS_SIZES); i++)
		if (sc(v, STRS_SIZES[i])) {
			free(*o); // free o that was malloced in expression before

			*o = bin_expr(p);
			if (is_reg(*o) || is_xmm(*o) || is_mm(*o))
				ee_token(p->f, (*o)->t, CANT_CHANGE_REG_SZ);
			(*o)->sz = 1 << i;
			(*o)->forsed_sz = 1;
			return 1;
		}
	return 0;
}

int is_size_word(char *v) {
	if (sc(v, STR_RESERVED))
		return RESERVED; // -1
	for (uint32_t i = 0; i < lenofarr(STRS_SIZES); i++)
		if (sc(v, STRS_SIZES[i]))
			return 1 << i;
	return 0;
}

// og - original
struct Oper *copy_oper(struct Oper *og) {
	struct Oper *o = malloc(sizeof(struct Oper));
	memcpy(o, og, sizeof(struct Oper));
	return o;
}

int search_defn(struct Pser *p, struct Oper **o, char *v) {
	struct Defn *d;
	for (uint32_t i = 0; i < p->ds->size; i++) {
		d = plist_get(p->ds, i);
		if (sc(v, d->view)) {
			// free(*o); // free o that was malloced in expression before

			// i dont really know why but its needed cuz if
			// вот чето 123
			// быть (рсп чето) е8
			// быть (рсп чето) е9
			// it will reassign чето to е8, cuz i dunno but just
			// cope the Oper solves it i really didnt found
			// where ot does assigns smtng to Oper
			//*o = copy_oper(d->value);
			memcpy(*o, d->value, sizeof(struct Oper));
			// *o = d->value;
			return 1;
		}
	}
	return 0;
}

struct Defn *is_defn(struct Pser *p, char *v) {
	struct Defn *d;
	for (int i = 0; i < p->ds->size; i++) {
		d = plist_get(p->ds, i);
		if (sc(v, d->view))
			return d;
	}
	return 0;
}

enum RegCode get_mem_reg(enum RegCode r) {
	enum RegCode n = R_NONE; // new
	if (r >= R_RAX && r <= R_RDI)
		n = r;
	else if (r >= R_R8 && r <= R_R15)
		n = r - R_R8 + R_RAX;
	else if (r >= R_EAX && r <= R_EDI)
		n = r - R_EAX + R_RAX;
	else if (r >= R_R8D && r <= R_R15D)
		n = r - R_R8D + R_RAX;
	if (n < R_RAX || n > R_RDI) {
		printf("\t\t\treg cvt err\n"); // 16 bytes str
		exit(1);
	}
	return n;
}

int search_reg(char *v, const int regs_len, const struct Reg regs[],
			   struct Oper *o, uc sz) {
	for (int i = 0; i < regs_len; i++)
		if (sc(v, regs[i].v)) {
			o->rm = regs[i].c;
			o->sz = sz;
			o->code = OREG;
			o->mod = MOD_REG;
			return 1;
		}
	return 0;
}

int search_some_reg(char *v, const int regs_len, const struct Reg regs[],
					struct Oper *o, enum OCode code, int sz) {
	for (int i = 0; i < regs_len; i++)
		if (sc(v, regs[i].v)) {
			o->rm = regs[i].c;
			o->code = code;
			o->sz = sz;
			return 1;
		}
	return 0;
}

void set_disp_to_op(struct Pser *p, struct Oper *o, struct Oper *d) {
	if (d->code == OINT) {
		int disp = d->t->num;
		o->mod = is_in_byte(disp) ? MOD_MEM_D8 : MOD_MEM_D32;
		o->disp = disp;
	} else if (d->code == OREL) {
		o->disp_is_rel_flag = OREL;
		o->mod = MOD_MEM_D32;
		o->rel_view = d->t->view;
	} else if (d->code == OBIN) {
		o->disp_is_rel_flag = OBIN;
		o->mod = MOD_MEM_D32;
		o->t->num = (long)d;
	} else
		ee_token(p->f, d->t, WRONG_DISP);

	if (d->code != OBIN)
		free(d);
}
void set_scale_to_op(struct Pser *p, struct Oper *o, struct Oper *s) {
	if (!(s->code == OINT))
		ee_token(p->f, s->t, WRONG_SCALE);
	int scale = s->t->num;
	if (scale == 1)
		o->scale = SCALE_1;
	else if (scale == 2)
		o->scale = SCALE_2;
	else if (scale == 4)
		o->scale = SCALE_4;
	else if (scale == 8)
		o->scale = SCALE_8;
	else
		ee_token(p->f, s->t, WRONG_SCALE);
	free(s);
}
void set_index_to_op(struct Pser *p, struct Oper *o, struct Oper *i) {
	if (!(i->code == OREG))
		ee_token(p->f, i->t, WRONG_INDEX);
	if (is_rsp_addr(i))
		ee_token(p->f, i->t, FORBIDDEN_RSP_INDEX);
	if (is_r_new(i))
		o->rex |= REX_X;
	o->index = get_mem_reg(i->rm);
	free(i);
}
void set_base_to_op(struct Pser *p, struct Oper *o, struct Oper *b) {
	if (b->code != OREG)
		ee_token(p->f, b->t, WRONG_BASE);
	o->rm = R_RSP; // sib
	if (is_rbp_addr(b) || is_r13_addr(b))
		o->mod = MOD_MEM_D8;
	if (is_r_new(b))
		o->rex |= REX_B;
	o->base = get_mem_reg(b->rm);
	free(b);
}
void set_rm_to_op(struct Oper *o, struct Oper *rm) {
	if (is_rsp_addr(rm) || is_r12_addr(rm)) {
		// REMEMBER: only R_RSI as SIB flag, esp is not allowed
		// mod = 00, rm = 100, base = 100, index = 100
		o->rm = R_RSP;	  // sib
		o->index = R_RSP; // unused
		o->base = R_RSP;
	} else
		o->rm = get_mem_reg(rm->rm);

	if (is_rbp_addr(rm) || is_r13_addr(rm))
		o->mod = MOD_MEM_D8;
	if (is_r_new(rm))
		o->rex |= REX_B;
	free(rm);
}

void get_moffs(struct Pser *p, struct Oper **o) {
	*o = bin_expr(p);
	if ((*o)->code != OINT)
		ee_token(p->f, (*o)->t, WRONG_MOFFS);
	if ((*o)->sz != DWORD && (*o)->sz != QWORD)
		ee_token(p->f, (*o)->t, WRONG_MOFFS_MEM_SZ);

	(*o)->code = OMOFFS;
	(*o)->mem_sz = (*o)->sz;
	(*o)->sz = DWORD;
}

#define is_int_or_rel_or_bin(o)                                                \
	((o)->code == OINT || (o)->code == OREL || (o)->code == OBIN)
struct Oper *expression(struct Pser *p) {
	struct Oper *o = malloc(sizeof(struct Oper)), *otmp, *otmp2;
	o->disp_is_rel_flag = 0;
	o->disp = 0;
	o->mem_sz = 0;
	o->mod = MOD_REG;
	o->scale = SCALE_1;
	o->index = R_NONE;
	o->base = R_NONE;
	o->rm = R_NONE;
	o->rex = 0;
	o->forsed_sz = 0;
	o->sz = DWORD;
	o->rel_flags = RF_NONE;

	struct PList *sib = new_plist(4);
	enum OCode code, *cp = &code;

	struct Token *ot, *t0 = next_get(p, -1);
	char *v;

	switch (t0->code) {
	case INT:
		set_tc(&t0, cp, t0, OINT, &o->sz, DWORD);
		ot = t0;
		break;
	case REAL:
		set_tc(&t0, cp, t0, OFPN, &o->sz, QWORD);
		ot = t0;
		break;
	case STR:
		if (t0->str->size == 1) {
			t0->num = (uint64_t)t0->str->st[0];
			o->sz = BYTE;
		} else if (t0->str->size == 2) {
			t0->num = *(uint16_t *)(t0->str->st);
			o->sz = WORD;
		} else
			ee_token(p->f, t0, INVALID_STR_LEN);
		code = OINT;
		ot = t0;
		break;
	case MINUS:
		t0 = next_get(p, -1);
		if (t0->code == INT) {
			t0->num *= -1;
			code = OINT;
		} else if (t0->code == REAL) {
			t0->real *= -1;
			code = OFPN;
		} else
			ee_token(p->f, t0, ERR_WRONG_MINUS);
		o->sz = DWORD;
		ot = t0;
		break;
	case ID:
		v = t0->view;
		ot = t0;
		if (sc(t0->view, STR__HERE)) {
			code = OREL;
			o->rel_flags = RF_HERE;
			ot = t0;
			o->sz = DWORD;
			break;
		} else if (sc(t0->view, STR__TUT)) {
			code = OREL;
			o->rel_flags = RF_TUT;
			ot = t0;
			o->sz = DWORD;
			break;
		} else if (search_reg(v, arr_l(E_REGS), E_REGS, o, DWORD))
			;
		else if (search_reg(v, arr_l(R_REGS), R_REGS, o, QWORD))
			;
		else if (search_reg(v, arr_l(W_REGS), W_REGS, o, WORD))
			;
		else if (search_reg(v, arr_l(B_REGS), B_REGS, o, BYTE))
			;
		else if (search_some_reg(v, arr_l(XMM_REGS), XMM_REGS, o, OXMM, XWORD))
			;
		else if (search_some_reg(v, arr_l(SEG_REGS), SEG_REGS, o, OSREG, WORD))
			;
		else if (search_some_reg(v, arr_l(MM_REGS), MM_REGS, o, OMM, QWORD))
			;
		else if (search_size(p, &o, v) || search_defn(p, &o, v)) {
			plist_free(sib);
			return o; // its special
		} else if (sc(v, STR_ADDR)) {
			get_moffs(p, &o);
			plist_free(sib);
			return o; // not so special indeed
		}
		if (o->rm != R_NONE) {
			code = o->code; // OREG or OSREG for now and OSIMD later
			break;
		}
		code = OREL;
		o->sz = DWORD;
		break;
	case PAR_L:
		do {
			otmp = bin_expr(p);
			if (otmp->code != OINT && otmp->code != OREL &&
				otmp->code != OREG && otmp->code != OBIN)
				ee_token(p->f, otmp->t, WRONG_ADDRES_OP);
			if (is_r8(otmp) || is_r16(otmp))
				ee_token(p->f, otmp->t, WRONG_ADDR_REG_SZ);
			if (otmp->code == OREG) {
				if (o->mem_sz) {
					if ((is_f_reg64(otmp->rm) && o->mem_sz == DWORD) ||
						(is_f_reg32(otmp->rm) && o->mem_sz == QWORD))
						ee_token(p->f, otmp->t, DIFFERENT_SIZE_REGS);
				} else {
					if (is_f_reg32(otmp->rm))
						o->mem_sz = DWORD;
					else
						o->mem_sz = QWORD;
				}
			}
			plist_add(sib, otmp);
			t0 = cur_token(p);
		} while (t0->code != PAR_R);
		next_get(p, 0); // skip )

		// [r s r d] = 4 os max
		if (sib->size > 4 || sib->size == 0)
			ee_token(p->f, t0, TOO_MUCH_OS);
		o->t = ot = t0;
		code = OMEM;
		o->mod = MOD_MEM;

		// REMEMBER: ALL MEM REGS ARE RAX-RDI
		// OTHER SHOULD BE CONVERTED VIA FLAGS

		otmp = plist_get(sib, 0);
		if (sib->size == 1) {
			if (is_int_or_rel_or_bin(otmp)) {
				// disp
				set_disp_to_op(p, o, otmp); // changes mod
				// REMEMBER: mod = 00, rm = 101 == [RIP+disp32]
				o->mod = MOD_MEM;
				o->rm = R_RBP;
			} else if (otmp->code == OREG) {
				// reg(rm)
				set_rm_to_op(o, otmp);
			} else
				ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);

		} else if (sib->size == 2) {
			if (otmp->code == OINT) {
				// sc reg(index)
				set_scale_to_op(p, o, otmp);
				set_index_to_op(p, o, plist_get(sib, 1));
				// mod = 00, rm = 100, base = 101 ==
				o->rm = R_RSP; // sib
				// no base register and
				o->base = R_RBP;
				// REMEMBER: do disp 32 although mod is 00
				// a 32-bit displacement
			} else if (otmp->code == OREG) {
				otmp2 = plist_get(sib, 1);
				if (otmp2->code == OREG) {
					// reg(base) reg(index) | sib
					set_base_to_op(p, o, otmp);
					set_index_to_op(p, o, otmp2);
				} else if (is_int_or_rel_or_bin(otmp2)) {
					// reg(rm) disp         | not sib
					set_rm_to_op(o, otmp);
					set_disp_to_op(p, o, otmp2); // changes mod to non 00
				} else
					ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);
			} else
				ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);

		} else if (sib->size == 3) {
			if (otmp->code == OREG) {
				set_base_to_op(p, o, otmp); // sets sib

				otmp2 = plist_get(sib, 1);
				if (otmp2->code == OREG) {
					// reg(base) reg(index) disp | sib
					set_index_to_op(p, o, otmp2);
					otmp2 = plist_get(sib, 2);
					set_disp_to_op(p, o, otmp2); // changes mod to non 00
				} else if (otmp2->code == OINT) {
					// reg(base) sc reg(index)   | sib
					set_scale_to_op(p, o, otmp2);
					otmp2 = plist_get(sib, 2);
					set_index_to_op(p, o, otmp2);
				} else
					ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);
			} else if (otmp->code == OINT) {
				o->rm = R_RSP; // sib
				// sc reg(index) disp            | sib
				set_scale_to_op(p, o, otmp);
				otmp = plist_get(sib, 1);
				set_index_to_op(p, o, otmp);
				// mod = 00, rm = 100, base = 101 no base register and disp32
				o->base = R_RBP;
				otmp = plist_get(sib, 2);
				// REMEMBER: here too need to write disp32
				set_disp_to_op(p, o, otmp); // changes mod
				o->mod = MOD_MEM;
			} else
				ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);

		} else if (otmp->code == OREG) { // size is 4
			// reg(base) sc reg(index) disp | sib
			set_base_to_op(p, o, otmp); // sets sib
			set_scale_to_op(p, o, plist_get(sib, 1));
			set_index_to_op(p, o, plist_get(sib, 2));
			set_disp_to_op(p, o, plist_get(sib, 3)); // changes mod
		} else if (otmp->code == OINT) {
			// sc reg(index) reg(base) disp | sib
			set_scale_to_op(p, o, otmp);
			set_index_to_op(p, o, plist_get(sib, 1));
			set_base_to_op(p, o, plist_get(sib, 2)); // sets sib
			set_disp_to_op(p, o, plist_get(sib, 3)); // changes mod
		} else
			ee_token(p->f, t0, POSSIBLE_WRONG_ORDER);
		break;
	default:
		ee_token(p->f, t0, ERR_WRONG_TOKEN);
	};

	o->code = code;
	o->t = ot;
	plist_free(sib);
	// print_oper(o);
	return o;
}
