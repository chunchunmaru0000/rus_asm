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
// instruction words
const char *STR_NOP = "ыыы";
const char *STR_IJMP = "идти";
const char *STR_IMOV = "быть";
const char *STR_IADD = "плюс";
const char *STR_ISUB = "минс";
const char *STR_IMUL = "зумн";
const long LTWO_OPS_STRS = 4;
const char *TWO_OPS_STRS[] = {"быть", "плюс", "минс", "зумн"};
const char *STR_CALL = "зов";
const char *STR_SCAL = "сзов";
// 8 and 16
// д8 for r8w and б8 for r8b
// 32
const int E_REGS_LEN = 26;
const struct Reg E_REGS[] = {
	{R_EAX, "еах"},	  {R_EBX, "ебх"},	   {R_ECX, "есх"},	 {R_EDX, "едх"},
	{R_ESI, "еси"},	  {R_EDI, "еди"},	   {R_EBP, "ебп"},	 {R_ESP, "есп"},
	{R_R8D, "е8"},	  {R_R9D, "е9"},	   {R_R10D, "е10"},	 {R_R11D, "е11"},
	{R_R12D, "е12"},  {R_R13D, "е13"},	   {R_R14D, "е14"},	 {R_R15D, "е15"},
	{R_R8D, "р8ч"},	  {R_R9D, "р9ч"},	   {R_R10D, "р10ч"}, {R_R11D, "р11ч"},
	{R_R12D, "р12ч"}, {R_R13D, "р13ч"},	   {R_R14D, "р14ч"}, {R_R15D, "р15ч"},
	{R_EIP, "еип"},	  {R_EFLAGS, "ефлаги"}};
const int R_REGS_LEN = 18;
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

int search_reg(char *v, const int regs_len, const struct Reg regs[],
			   struct Oper *o, uc sz) {
	for (int i = 0; i < regs_len; i++)
		if (sc(v, regs[i].v)) {
			o->rcode = regs[i].c;
			o->sz = sz;
			return 1;
		}
	return 0;
}

const int STRS_SIZES_LEN = 4;
const char *STRS_SIZES[] = {"байт", "дбайт", "чбайт", "вбайт"};

int search_size(char *v, struct Oper **o, struct Pser *p) {
	for (int i = 0; i < STRS_SIZES_LEN; i++)
		if (sc(v, STRS_SIZES[i])) {
			*o = expression(p);
			(*o)->sz = 1 << i;
			return 1;
		}
	return 0;
}

int is_size_word(char *v) {
	for (int i = 0; i < STRS_SIZES_LEN; i++)
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

struct Oper *expression(struct Pser *p) {
	struct Oper *o = malloc(sizeof(struct Oper));
	enum OCode code, *cp = &code;

	struct Token *ot, *t0 = next_get(p, -1), *t1, *t2;
	struct Token **t0p = &t0;
	char *v;
	uint64_t buf;
	uc *szp = &o->sz;

	switch (t0->code) {
	case INT:
		set_tc(t0p, cp, t0, OINT, szp, DWORD);
		ot = t0;
		break;
	case REAL:
		set_tc(t0p, cp, t0, OFPN, szp, DWORD);
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
		o->rcode = R_NONE;
		if (search_reg(v, E_REGS_LEN, E_REGS, o, DWORD))
			;
		else if (search_reg(v, R_REGS_LEN, R_REGS, o, QWORD))
			;
		else if (search_size(v, &o, p))
			return o; // its special
		else if (search_defn(v, &o, p))
			return o; // its too
		if (o->rcode != R_NONE) {
			ot = t0;
			code = OREG;
			break;
		}
		ot = t0;
		code = OREL;
		o->sz = DWORD;
		break;
	default:
		eep(t0, ERR_WRONG_TOKEN);
	};

	o->code = code;
	o->t = ot;
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
	while (cur->code == SLASHN)
		cur = next_get(p, 0);
	char *cv = cur->view;
	n = gettp(p, 1);
	enum ICode code;

	// fill *os in funcs
	if (cur->code == EF)
		code = IEOI;
	else if (cur->code == ID) {
		if (cont_str(cv, TWO_OPS_STRS, LTWO_OPS_STRS))
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
