#include "pser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *fn;
void eep(struct Token *t, char *msg) { // error exit
	fprintf(stderr, "%s:%ld:%ld %s %d:%s\n", fn, t->line, t->col, msg, t->code,
			t->view);
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

// directives
const char *STR_ENTRY = "вход";
const char *STR_NOP = "ыыы";
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
const char *STR_IJMP = "идти";
const char *STR_IMOV = "быть";
const char *STR_IADD = "плюс";
const char *STR_ISUB = "минс";
const char *STR_IMUL = "зумн";
const char *TWO_OPS_STRS[] = {"быть", "плюс", "минс", "зумн"};
const long LTWO_OPS_STRS = 4;
const char *STR_CALL = "зов";
const char *STR_SCAL = "сзов";
// 32
const int E_REGS_LEN = 18;
const struct Reg E_REGS[] = {
	{R_EIP, "еип"},	 {R_EFLAGS, "ефлаги"}, {R_EAX, "еах"},	{R_EBX, "ебх"},
	{R_ECX, "есх"},	 {R_EDX, "едх"},	   {R_ESI, "еси"},	{R_EDI, "еди"},
	{R_EBP, "ебп"},	 {R_ESP, "есп"},	   {R_R8D, "е8"},	{R_R9D, "е9"},
	{R_R10D, "е10"}, {R_R11D, "е11"},	   {R_R12D, "е12"}, {R_R13D, "е13"},
	{R_R14D, "е14"}, {R_R15D, "е15"}
};
// д8 for r8w and б8 for r8b
const char *STR_EIP = "еип";
const char *STR_EFLAGS = "ефлаги";
const char *STR_EAX = "еах";
const char *STR_EBX = "ебх";
const char *STR_ECX = "еси";
const char *STR_EDX = "едх";
const char *STR_ESI = "еси";
const char *STR_EDI = "еди";
const char *STR_EBP = "ебп";
const char *STR_ESP = "есп";
const char *STR_R8D = "е8";
const char *STR_R9D = "е9";
const char *STR_R10D = "е10";
const char *STR_R11D = "е11";
const char *STR_R12D = "е12";
const char *STR_R13D = "е13";
const char *STR_R14D = "е14";
const char *STR_R15D = "е15";
// 64
const char *STR_RIP = "рип";
const char *STR_RFLAGS = "рфлаги";
const char *STR_RAX = "рах";
const char *STR_RBX = "рбх";
const char *STR_RCX = "рсх";
const char *STR_RDX = "рдх";
const char *STR_RSI = "рси";
const char *STR_RDI = "рди";
const char *STR_RBP = "рбп";
const char *STR_RSP = "рсп";
const char *STR_R8 = "р8";
const char *STR_R9 = "р9";
const char *STR_R10 = "р10";
const char *STR_R11 = "р11";
const char *STR_R12 = "р12";
const char *STR_R13 = "р13";
const char *STR_R14 = "р14";
const char *STR_R15 = "р15";

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

#define set_tc(tp, cp, t, c)                                                   \
	do {                                                                       \
		*(tp) = (t);                                                           \
		*(cp) = (c);                                                           \
	} while (0)
// void set_tc(struct Token **tp, enum OCode *cp, struct Token *t, enum OCode c)
// { *tp = t; *cp = c; }

char *ERR_WRONG_TOKEN = "НЕВЕРНОЕ ВЫРАЖЕНИЕ";
char *ERR_WRONG_ID = "НЕИЗВЕСТНОЕ СЛОВО НА ДАННЫЙ МОМЕНТ";
char *ERR_WRONG_MINUS = "МИНУС МОЖНО ИСПОЛЬЗОВАТЬ ТОЛЬКО ПЕРЕД ЧИСЛАМИ";

struct Oper *expression(struct Pser *p) {
	struct Oper *o = malloc(sizeof(struct Oper));
	enum OCode code, *cp = &code;

	struct Token *ot, *t0 = next_get(p, -1), *t1, *t2;
	struct Token **t0p = &t0;
	char *v;
	int i;

	switch (t0->code) {
	case INT:
		set_tc(t0p, cp, t0, OINT);
		break;
	case REAL:
		set_tc(t0p, cp, t0, OFPN);
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
		ot = t0;
		break;
	case ID:
		v = t0->view;
		o->rcode = R_NONE;
		for (i = 0; i < E_REGS_LEN; i++)
			if (sc(v, E_REGS[i].v)) {
				o->rcode = E_REGS[i].c;
				o->sz = 4;
				break;
			}
		if (o->rcode != R_NONE) {
			ot = t0;
			code = OREG;
			break;
		} // else
		code = OREL;
		o->sz = 4;
		ot = t0;
		// eep(t0, ERR_WRONG_ID);
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
	struct Token *label = next_get(p, 0);
	next_get(p, 0); // skip label

	if (!(label->code == ID))
		eep(label, "ОЖИДАЛАСЬ МЕТКА");
	plist_add(os, label);
	return IJMP;
}

enum ICode let_i(struct Pser *p, struct PList *os) {
	struct BList *data = new_blist(8);
	struct Token *var = next_get(p, 0), *cur = next_get(p, 0);

	int size;
	if (sc(cur->view, STR_BYTE))
		size = 1;
	else if (sc(cur->view, STR_WORD))
		size = 2;
	else if (sc(cur->view, STR_DWORD))
		size = 4;
	else if (sc(cur->view, STR_QWORD))
		size = 8;
	else
		eep(cur, "ОЖИДАЛСЯ РАЗМЕР ОПЕРАНДА: <байт> <дбайт> <чбайт> <вбайт>");

	while (1) {
		cur = next_get(p, 0);
		if (cur->code == SLASH) {
			cur = next_get(p, 0); // skip slash get \n
			continue;
		}
		if (cur->code == INT)
			blat(data, (uc *)&cur->number, size);
		else if (cur->code == STR)
			blat(data, (uc *)cur->string, cur->string_len);
		else {
			if (p->debug)
				printf("let %s be %ld bytes\n", var->view, data->size);
			break;
		}
	}

	plist_add(os, var);
	plist_add(os, data);
	return ILET;
}

enum ICode no_ops_inst(struct Pser *p, enum ICode code) {
	next_get(p, 0);
	return code;
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
		plist_add(is, i);
		i = get_inst(p);
	}
	plist_add(is, i);

	return is;
}
