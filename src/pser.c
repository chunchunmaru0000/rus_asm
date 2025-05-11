#include "pser.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *fn;
void eep(struct Token *t, char *msg) { // error exit
	fprintf(stderr, "%s:%ld:%ld %s [%s]:[%d]\n", fn, t->line, t->col, msg,
			t->view, t->code);
	exit(1);
}

int get_reg_field(enum RegCode rm) {
	int f = -1;
	if (is_f_reg8(rm))
		f = rm - R_AH;
	else if (is_f_reg16(rm))
		f = rm - R_AX;
	else if (is_f_reg32(rm))
		f = rm - R_EAX;
	else if (is_f_reg64(rm))
		f = rm - R_RAX;
	if (f > 0b111)
		f -= 0b1000;
	return f;
}

void print_oper(struct Oper *o) {
	printf("[view:%s][oc:%d][rm:%d]\t[base:%d]"
		   "[scale:%d][index:%d][disp:%d][mod:%d][rex:%d]\n",
		   o->t->view, o->code, get_reg_field(o->rm), get_reg_field(o->base),
		   1 << o->scale, get_reg_field(o->index), o->disp, o->mod, o->rex);
}

struct Pser *new_pser(char *filename, uc debug) {
	fn = filename;
	struct Pser *p = malloc(sizeof(struct Pser));
	struct Tzer *t = new_tzer(filename);
	struct PList *ts = tze(t, 10);
	free(t);
	p->pos = 0;
	p->ts = ts;
	p->debug = debug;
	p->ds = new_plist(3);
	return p;
}

struct Inst *new_inst(enum ICode code, struct PList *os, struct Token *t) {
	struct Inst *i = malloc(sizeof(struct Inst));
	i->code = code;
	i->os = os;
	i->col = t->col;
	i->line = t->line;
	i->file = fn;
	return i;
}

struct Token *gettp(struct Pser *p, long off) {
	long i = p->pos + off;
	return p->ts->size > i ? p->ts->st[i] : p->ts->st[p->ts->size - 1];
}

struct Token *next_get(struct Pser *p, long off) {
	p->pos++;
	return gettp(p, off);
}

// parser directives
const char *STR_DEFINE = "вот";
// directives
const char *STR_ENTRY = "вход";
const char *STR_SEG = "участок";
const char *STR_SEG_R = "чит";
const char *STR_SEG_W = "изм";
const char *STR_SEG_X = "исп";
const char *STR_EOF = "_КОНЕЦ_ФАЙЛА_";
const char *STR_LET = "пусть";
const char *STR_BYTE = "байт";
const char *STR_WORD = "дбайт";
const char *STR_DWORD = "чбайт";
const char *STR_QWORD = "вбайт";
const char *STR_ADDR = "адр";
// instruction words
const char *STR_NOP = "ыыы";
const char *STR_IJMP = "идти";
const char *STR_IMOV = "быть";
const char *STR_IADD = "плюс";
const char *STR_ISUB = "минс";
const char *STR_IMUL = "зумн";
const char *TWO_OPS_STRS[] = {"быть", "плюс", "минс", "зумн"};
const char *STR_CALL = "зов";
const char *STR_SCAL = "сзов";
// seg
const struct Reg SEG_REGS[] = {{R_CS, "кс"},	 {R_DS, "дс"}, {R_ES, "ес"},
							   {R_SS, "сс"},	 {R_FS, "фс"}, {R_GS, "гс"},
							   {R_LDTR, "лдтр"}, {R_TR, "тр"}};
// 8
const struct Reg B_REGS[] = {
	{R_AX, "ах"},	  {R_CX, "сх"},		{R_DX, "дх"},	  {R_BX, "бх"},
	{R_SP, "сп"},	  {R_BP, "бп"},		{R_SI, "си"},	  {R_DI, "ди"},
	{R_R8B, "б8"},	  {R_R9B, "б9"},	{R_R10B, "б10"},  {R_R11B, "б11"},
	{R_R12B, "б12"},  {R_R13B, "дб3"},	{R_R14B, "б14"},  {R_R15B, "б15"},
	{R_R8B, "р8б"},	  {R_R9B, "р9б"},	{R_R10B, "р10б"}, {R_R11B, "р11б"},
	{R_R12B, "р12б"}, {R_R13B, "р13б"}, {R_R14B, "р14б"}, {R_R15B, "р15б"}};
// 16
const struct Reg W_REGS[] = {
	{R_AX, "ах"},	  {R_CX, "сх"},		 {R_DX, "дх"},	   {R_BX, "бх"},
	{R_SP, "сп"},	  {R_BP, "бп"},		 {R_SI, "си"},	   {R_DI, "ди"},
	{R_R8W, "д8"},	  {R_R9W, "д9"},	 {R_R10W, "д10"},  {R_R11W, "д11"},
	{R_R12W, "д12"},  {R_R13W, "д13"},	 {R_R14W, "д14"},  {R_R15W, "д15"},
	{R_R8W, "р8д"},	  {R_R9W, "р9д"},	 {R_R10W, "р10д"}, {R_R11W, "р11д"},
	{R_R12W, "р12д"}, {R_R13W, "р13д"},	 {R_R14W, "р14д"}, {R_R15W, "р15д"},
	{R_IP, "ип"},	  {R_FLAGS, "флаги"}};
// 32
const struct Reg E_REGS[] = {
	{R_EAX, "еах"},	  {R_EBX, "ебх"},	   {R_ECX, "есх"},	 {R_EDX, "едх"},
	{R_ESI, "еси"},	  {R_EDI, "еди"},	   {R_EBP, "ебп"},	 {R_ESP, "есп"},
	{R_R8D, "е8"},	  {R_R9D, "е9"},	   {R_R10D, "е10"},	 {R_R11D, "е11"},
	{R_R12D, "е12"},  {R_R13D, "е13"},	   {R_R14D, "е14"},	 {R_R15D, "е15"},
	{R_R8D, "р8ч"},	  {R_R9D, "р9ч"},	   {R_R10D, "р10ч"}, {R_R11D, "р11ч"},
	{R_R12D, "р12ч"}, {R_R13D, "р13ч"},	   {R_R14D, "р14ч"}, {R_R15D, "р15ч"},
	{R_EIP, "еип"},	  {R_EFLAGS, "ефлаги"}};
// 64
const struct Reg R_REGS[] = {
	{R_RAX, "рах"}, {R_RBX, "рбх"},		 {R_RCX, "рсх"}, {R_RDX, "рдх"},
	{R_RSI, "рси"}, {R_RDI, "рди"},		 {R_RBP, "рбп"}, {R_RSP, "рсп"},
	{R_R8, "р8"},	{R_R9, "р9"},		 {R_R10, "р10"}, {R_R11, "р11"},
	{R_R12, "р12"}, {R_R13, "р13"},		 {R_R14, "р14"}, {R_R15, "р15"},
	{R_RIP, "рип"}, {R_RFLAGS, "рфлаги"}};

uc sc(char *view, const char *str) {
	// printf("[%s]==[%s]\n", view, str);
	return strcmp(view, str) == 0;
}
uc cont_str(char *view, const char **strs, long strs_len) {
	for (int i = 0; i < strs_len; i++)
		if (sc(view, strs[i]))
			return 1;
	return 0;
}

enum ICode entry_i(struct Pser *p, struct PList *os) {
	plist_add(os, next_get(p, 0));
	next_get(p, 0);
	return IENTRY;
}
enum ICode seg_i(struct Pser *p, struct PList *os) {
	struct Token *t = next_get(p, 0);
	int *flags = malloc(sizeof(int));
	*flags = 0;
	if (t->code == INT)
		*flags = t->number;
	else
		while (t->code != SLASHN && t->code != EF) {
			if (sc(t->view, STR_SEG_X))
				*flags |= 0b1;
			else if (sc(t->view, STR_SEG_W))
				*flags |= 0b10;
			else if (sc(t->view, STR_SEG_R))
				*flags |= 0b100;
			t = next_get(p, 0);
		}
	next_get(p, 0); // skip \n
	plist_add(os, flags);
	return ISEGMENT;
}

struct Oper *expression(struct Pser *);

enum ICode two_ops_i(struct Pser *p, struct PList *os) {
	char *v = ((struct Token *)gettp(p, 0))->view;
	enum ICode code;

	if (sc(v, STR_IMOV))
		code = IMOV;
	else if (sc(v, STR_IADD))
		code = IADD;
	else if (sc(v, STR_ISUB))
		code = ISUB;
	else if (sc(v, STR_IMUL))
		code = IIMUL;
	// else
	next_get(p, 0); // skip instruction
	plist_add(os, expression(p));
	plist_add(os, expression(p));

	return code;
}

#define set_tc(tp, cp, t, c, sp, s)                                            \
	do {                                                                       \
		*(tp) = (t);                                                           \
		*(cp) = (c);                                                           \
		*(sp) = (s);                                                           \
	} while (0)
// void set_tc(struct Token **tp, enum OCode *cp, struct Token *t, enum
// OCode c) { *tp = t; *cp = c; }

const char *STRS_SIZES[] = {"байт", "дбайт", "чбайт", "вбайт"};

int search_size(char *v, struct Oper **o, struct Pser *p) {
	for (uint32_t i = 0; i < lenofarr(STRS_SIZES); i++)
		if (sc(v, STRS_SIZES[i])) {
			*o = expression(p);
			(*o)->sz = 1 << i;
			return 1;
		}
	return 0;
}

int is_size_word(char *v) {
	for (uint32_t i = 0; i < lenofarr(STRS_SIZES); i++)
		if (sc(v, STRS_SIZES[i]))
			return 1 << i;
	return 0;
}

int search_defn(char *v, struct Oper **o, struct Pser *p) {
	struct Defn *d;
	for (int i = 0; i < p->ds->size; i++) {
		d = plist_get(p->ds, i);
		if (sc(v, d->view)) {
			*o = d->value;
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

char *ERR_WRONG_TOKEN = "Неверное выражение";
char *ERR_WRONG_MINUS = "Минус можно использовать только перед числами";
char *INVALID_STR_LEN =
	"Длина строки в выражении не может быть равна 0 или быть больше 2";
char *TOO_MUCH_OS =
	"Слишком много или мало выражений в адресанте, максимум может быть "
	"4, минимум 1, в следующих порядках:\n"
	"\t[<регистр> <множитель: 1,2,4,8> регистр <смещение>]\n"
	"\t[смещение]\n";
char *POSSIBLE_WRONG_ORDER =
	"Возможно выражения в адресанте были в неверном порядке, "
	"всевозможные порядки:\n"
	"\t[<регистр> <множитель: 1,2,4,8> регистр <смещение>]\n"
	"\t[смещение]\n";
char *WRONG_ADDRES_OP = "Неверное выражение внутри адресанта, ими могут быть "
						"только регистры, метки или целые числа";
char *WRONG_DISP = "Неверное смещение, ожидалось число 32 бит или метка";
char *WRONG_SCALE =
	"Неверный множитель, ожидалось число(0, 2, 4, 8) или его отсутствие";
char *WRONG_INDEX = "Неверный индекс, ожидался регистр";
char *WRONG_RM = "Неверное выражение, ожидался регистр";
char *WRONG_BASE = "Неверная основа, ожидался регистр";
char *WRONG_ADDR_REG_SZ =
	"Неверный размер регистра в выражении операнда, адресными регистрами могут "
	"быть только 32-х или 64-х битные регистры";
// МИО - Множитель Индекс Основа
char *FORBIDDEN_RSP_INDEX =
	"Регистр рсп или есп запрещены в качестве индекса в МИО байте";
char *DISSERENT_SIZE_REGS = "Регистры адресанта не могут быть разных размеров, "
							"только или все 64 бит или 32 бит";

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
			// if (is_r_new(o)) TODO: check if its true
			//	o->rex |= REX_R;
			o->sz = sz;
			o->mod = MOD_REG;
			return 1;
		}
	return 0;
}

void set_disp_to_op(struct Oper *o, struct Oper *d) {
	if (d->code == OINT) {
		int disp = d->t->number;
		if (disp < -128 || disp > 127)
			o->mod = MOD_MEM_D32;
		else
			o->mod = MOD_MEM_D8;
		o->disp = disp;
	} else if (d->code == OREL) {
		o->disp_is_rel_flag = 1;
		o->mod = MOD_MEM_D32;
		o->rel_view = d->t->view;
	} else
		eep(d->t, WRONG_DISP);
	free(d);
}
void set_scale_to_op(struct Oper *o, struct Oper *s) {
	if (!(s->code == OINT))
		eep(s->t, WRONG_SCALE);
	int scale = s->t->number;
	if (scale == 1)
		o->scale = SCALE_1;
	else if (scale == 2)
		o->scale = SCALE_2;
	else if (scale == 4)
		o->scale = SCALE_4;
	else if (scale == 8)
		o->scale = SCALE_8;
	else
		eep(s->t, WRONG_SCALE);
	free(s);
}
void set_index_to_op(struct Oper *o, struct Oper *i) {
	if (!(i->code == OREG))
		eep(i->t, WRONG_INDEX);
	if (is_rsp_addr(i))
		eep(i->t, FORBIDDEN_RSP_INDEX);
	if (is_r_new(i))
		o->rex |= REX_X;
	o->index = get_mem_reg(i->rm);
	free(i);
}
void set_base_to_op(struct Oper *o, struct Oper *b) {
	if (b->code != OREG)
		eep(b->t, WRONG_BASE);
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

	struct PList *sib = new_plist(4);
	enum OCode code, *cp = &code;

	struct Token *ot, *t0 = next_get(p, -1); //, *t1, *t2;
	char *v;
	uint64_t buf;

	switch (t0->code) {
	case INT:
		set_tc(&t0, cp, t0, OINT, &o->sz, DWORD);
		ot = t0;
		break;
	case REAL:
		set_tc(&t0, cp, t0, OFPN, &o->sz, DWORD);
		ot = t0;
		break;
	case STR:
		if (t0->string_len == 1) {
			t0->number = (uint64_t)t0->string[0];
			o->sz = BYTE;
		} else if (t0->string_len == 2) {
			buf = 0;
			memcpy((uc *)&buf + 1, t0->string + 1, 1);
			memcpy(&buf, t0->string, 1);
			t0->number = buf;
			// TODO: do it as a human, not as a черт
			//((uint32_t)t0->string[1] << 8) + (uint32_t)t0->string[0];
			o->sz = WORD;
		} else
			eep(t0, INVALID_STR_LEN);
		code = OINT;
		ot = t0;
		break;
	case MINUS:
		t0 = next_get(p, -1);
		if (t0->code == INT) {
			t0->number *= -1;
			code = OINT;
		} else if (t0->code == REAL) {
			t0->fpn *= -1;
			code = OFPN;
		} else
			eep(t0, ERR_WRONG_MINUS);
		o->sz = DWORD;
		ot = t0;
		break;
	case ID:
		v = t0->view;
		ot = t0;
		if (search_reg(v, lenofarr(E_REGS), E_REGS, o, DWORD))
			;
		else if (search_reg(v, lenofarr(R_REGS), R_REGS, o, QWORD))
			;
		else if (search_reg(v, lenofarr(W_REGS), W_REGS, o, WORD))
			;
		else if (search_reg(v, lenofarr(B_REGS), B_REGS, o, BYTE))
			;
		else if (search_size(v, &o, p)) {
			plist_free(sib);
			return o; // its special
		} else if (search_defn(v, &o, p)) {
			plist_free(sib);
			return o; // its too
		} else if (sc(v, STR_ADDR)) {
			code = OMOFFS;
			// TODO: moffs
			// return get_moffs(p, o);
			eep(t0, "НЕ СДЕЛАНО MOFFS");
		}
		if (o->rm != R_NONE) {
			code = OREG;
			break;
		}
		code = OREL;
		o->sz = DWORD;
		break;
	case PAR_L:
		do {
			otmp = expression(p);
			if (otmp->code != OINT && otmp->code != OREL && otmp->code != OREG)
				eep(otmp->t, WRONG_ADDRES_OP);
			if (is_r8(otmp) || is_r16(otmp))
				eep(otmp->t, WRONG_ADDR_REG_SZ);
			if (otmp->code == OREG) {
				if (o->mem_sz) {
					if ((is_f_reg64(otmp->rm) && o->mem_sz == DWORD) ||
						(is_f_reg32(otmp->rm) && o->mem_sz == QWORD))
						eep(otmp->t, DISSERENT_SIZE_REGS);
				} else {
					if (is_f_reg32(otmp->rm))
						o->mem_sz = DWORD;
					else
						o->mem_sz = QWORD;
				}
			}
			plist_add(sib, otmp);
			t0 = gettp(p, 0);
		} while (t0->code != PAR_R);
		next_get(p, 0); // skip )

		// [r s r d] = 4 os max
		if (sib->size > 4 || sib->size == 0)
			eep(t0, TOO_MUCH_OS);
		ot = t0;
		code = OMEM;
		o->mod = MOD_MEM;

		// TODO: ALL MEM REGS ARE RAX-RDI, OTHER SHOULD BE CONVERTED VIA FLAGS
		// TODO: REX

		otmp = plist_get(sib, 0);
		if (sib->size == 1) {
			if (otmp->code == OREL || otmp->code == OINT) {
				// disp
				set_disp_to_op(o, otmp); // changes mod
				// REMEMBER: mod = 00, rm = 101 == [RIP+disp32]
				o->mod = MOD_MEM;
				o->rm = R_RBP;
			} else if (otmp->code == OREG) {
				// reg(rm)
				set_rm_to_op(o, otmp);
			} else
				eep(t0, POSSIBLE_WRONG_ORDER);

		} else if (sib->size == 2) {
			if (otmp->code == OINT) {
				// sc reg(index)
				set_scale_to_op(o, otmp);
				set_index_to_op(o, plist_get(sib, 1));
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
					set_base_to_op(o, otmp);
					set_index_to_op(o, otmp2);
				} else if (otmp2->code == OREL || otmp2->code == OINT) {
					// reg(rm) disp         | not sib
					set_rm_to_op(o, otmp);
					set_disp_to_op(o, otmp2); // changes mod to non 00
				} else
					eep(t0, POSSIBLE_WRONG_ORDER);
			} else
				eep(t0, POSSIBLE_WRONG_ORDER);

		} else if (sib->size == 3) {
			if (otmp->code == OREG) {
				set_base_to_op(o, otmp); // sets sib

				otmp2 = plist_get(sib, 1);
				if (otmp2->code == OREG) {
					// reg(base) reg(index) disp | sib
					set_index_to_op(o, otmp2);
					otmp2 = plist_get(sib, 2);
					set_disp_to_op(o, otmp2); // changes mod to non 00
				} else if (otmp2->code == OINT) {
					// reg(base) sc reg(index)   | sib
					set_scale_to_op(o, otmp2);
					otmp2 = plist_get(sib, 2);
					set_index_to_op(o, otmp2);
				} else
					eep(t0, POSSIBLE_WRONG_ORDER);
			} else if (otmp->code == OINT) {
				o->rm = R_RSP; // sib
				// sc reg(index) disp            | sib
				set_scale_to_op(o, otmp);
				otmp = plist_get(sib, 1);
				set_index_to_op(o, otmp);
				// mod = 00, rm = 100, base = 101 no base register and disp32
				o->base = R_RBP;
				otmp = plist_get(sib, 2);
				// REMEMBER: here too need to write disp32
				set_disp_to_op(o, otmp); // changes mod
				o->mod = MOD_MEM;
			} else
				eep(t0, POSSIBLE_WRONG_ORDER);

		} else if (otmp->code == OREG) { // size is 4
			// reg(base) sc reg(index) disp | sib
			set_base_to_op(o, otmp); // sets sib
			set_scale_to_op(o, plist_get(sib, 1));
			set_index_to_op(o, plist_get(sib, 2));
			set_disp_to_op(o, plist_get(sib, 3)); // changes mod
		} else
			eep(t0, POSSIBLE_WRONG_ORDER);
		break;
	default:
		eep(t0, ERR_WRONG_TOKEN);
	};

	o->code = code;
	o->t = ot;
	plist_free(sib);
	// print_oper(o);
	return o;
}

enum ICode label_i(struct Pser *p, struct PList *os) {
	plist_add(os, gettp(p, 0));
	next_get(p, 0); // skip label
	next_get(p, 0); // skip :
	return ILABEL;
}

enum ICode jmp_i(struct Pser *p, struct PList *os) {
	next_get(p, 0); // skip jmp
	plist_add(os, expression(p));

	return IJMP;
}

char *INVALID_SIZE_NOT_FOUND =
	"Ожидался размер операнда: <байт> <дбайт> <чбайт> <вбайт>";
char *INVALID_SIZE_OF_FPN = "Неверный размер для числа с плавающей точкой, "
							"ожидался размер <чбайт> или <вбайт>";
char *AWAITED_SLASHN = "Ожидался перевод строки";
char *INVALID_DEFN_USAGE = "Неверное использование определения";
enum ICode let_i(struct Pser *p, struct PList *os) {
	struct BList *data = new_blist(8);
	struct Token *c = next_get(p, 0), *name; // skip let word
	uint64_t buf;
	enum ICode code = ILET;
	struct Defn *d;

	while (c->code == SLASH || c->code == SLASHN)
		c = next_get(p, 0);

	int size = is_size_word(c->view), old_sz;
	if (size)
		code = IDATA;
	else {
		name = c;
		do
			c = next_get(p, 0);
		while (c->code == SLASH || c->code == SLASHN);
		size = is_size_word(c->view);
		if (!size)
			eep(c, INVALID_SIZE_NOT_FOUND);
	}

	loop {
		c = next_get(p, 0);
		if (c->code == SLASH) {
			c = next_get(p, 0); // skip slash get \n
			if (c->code != SLASHN)
				eep(c, AWAITED_SLASHN);
			continue;
		}
		if (c->code == SEP) {
			next_get(p, 0); // skip separator
			break;
		}
		if (c->code == ID) {
			old_sz = size;
			size = is_size_word(c->view);
			if (size)
				continue;
			size = old_sz;
			d = is_defn(p, c->view);
			if (!d)
				break; // break if ID is not size or defn

			if (d->value->code == OINT)
				blat(data, (uc *)&d->value->t->number, size);
			else if (d->value->code == OFPN) {
				c = d->value->t;
				goto let_i_real;
			} else
				eep(c, INVALID_DEFN_USAGE);
		} else if (c->code == INT)
			blat(data, (uc *)&c->number, size);
		else if (c->code == STR)
			blat(data, (uc *)c->string, c->string_len);
		else if (c->code == REAL) {
		let_i_real:
			if (size == 8) {
				memcpy(&buf, &c->fpn, 8);
				blat(data, (uc *)&buf, 8);
			} else if (size == 4) {
				float value = c->fpn;
				memcpy(&buf, &value, 4);
				blat(data, (uc *)&buf, 4);
			} else
				eep(c, INVALID_SIZE_OF_FPN);
		} else {
			if (p->debug) {
				if (code == ILET)
					printf("пусть %s будет %ld байт\n", name->view, data->size);
				else
					printf("пусть %ld байт\n", data->size);
			}
			break;
		}
	}

	if (code == ILET)
		plist_add(os, name);
	plist_add(os, data);
	return code;
}

enum ICode no_ops_inst(struct Pser *p, enum ICode code) {
	next_get(p, 0);
	return code;
}

enum ICode define_pd(struct Pser *p) {
	struct Defn *d = malloc(sizeof(struct Defn));
	d->view = next_get(p, 0)->view;
	next_get(p, 0); // skip name token
	d->value = expression(p);
	plist_add(p->ds, d);
	return INONE;
}

struct Inst *get_inst(struct Pser *p) {
	struct PList *os = new_plist(4);
	struct Token *cur = gettp(p, 0), *n;
	while (cur->code == SLASHN || cur->code == SEP)
		cur = next_get(p, 0);
	char *cv = cur->view;
	n = gettp(p, 1);
	enum ICode code;

	// fill *os in funcs
	if (cur->code == EF)
		code = IEOI;
	else if (cur->code == ID) {
		if (cont_str(cv, TWO_OPS_STRS, lenofarr(TWO_OPS_STRS)))
			code = two_ops_i(p, os);
		else if (sc(cv, STR_SCAL))
			code = no_ops_inst(p, ISYSCALL);
		else if (n->code == COLO)
			code = label_i(p, os);
		else if (sc(cv, STR_LET))
			code = let_i(p, os);
		else if (sc(cv, STR_IJMP))
			code = jmp_i(p, os);
		else if (sc(cv, STR_DEFINE))
			code = define_pd(p);
		else if (sc(cv, STR_SEG))
			code = seg_i(p, os);
		else if (sc(cv, STR_NOP))
			code = no_ops_inst(p, INOP);
		else if (sc(cv, STR_ENTRY))
			code = entry_i(p, os);
		else
			eep(n, "НЕИЗВЕСТНАЯ КОМАНДА");
	} else
		eep(cur, "НЕИЗВЕСТНАЯ КОМАНДА");

	return new_inst(code, os, cur);
}

struct PList *pse(struct Pser *p) {
	struct PList *is = new_plist(p->ts->cap_pace); // why not

	struct Inst *i = get_inst(p);
	while (i->code != IEOI) {
		if (i->code != INONE)
			plist_add(is, i);
		i = get_inst(p);
	}
	plist_add(is, i);

	return is;
}
