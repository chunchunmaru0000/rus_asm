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

struct Pser *new_pser(char *filename) {
	fn = filename;
	struct Pser *p = malloc(sizeof(struct Pser));
	struct Tzer *t = new_tzer(filename);
	struct PList *ts = tze(t, 10);
	p->pos = 0;
	p->ts = ts;
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
const char *STR_PLUS = "плюс";
const char *STR_MINS = "минс";
const char *STR_IMUL = "зумн";
const char *TWO_OPS_STRS[] = {"быть", "плюс", "минс", "зумн"};
const long LTWO_OPS_STRS = 4;
const char *STR_CALL = "зов";
const char *STR_SCAL = "сзов";
// registers words
const char *STR_EAX = "еах";
const char *STR_RAX = "рах";
const char *STR_RBX = "рбх";
const char *STR_RCX = "рсх";
const char *STR_EDX = "едх";
const char *STR_RDX = "рдх";
const char *STR_EDI = "еди";
const char *STR_RDI = "рди";
const char *STR_ESI = "еси";
const char *STR_RSI = "рси";
const char *STR_RSP = "рсп";
const char *STR_RBP = "рбп";

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
		while (t->code != SLASHN) {
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

char *ERR_MOV_EAX = "НЕИЗВЕСТНЫЙ ОПЕРАНД ДЛЯ быть еах ...";
char *ERR_MOV_EDX = "НЕИЗВЕСТНЫЙ ОПЕРАНД ДЛЯ быть едх ...";
char *ERR_MOV_EDI = "НЕИЗВЕСТНЫЙ ОПЕРАНД ДЛЯ быть еди ...";
char *ERR_MOV_ESI = "НЕИЗВЕСТНЫЙ ОПЕРАНД ДЛЯ быть еси ...";
char *ERR_MOV = "НЕИЗВЕСТНЫЙ ОПЕРАНД ДЛЯ быть ... ...";
enum ICode two_ops_i(struct Pser *p, struct PList *os) {
	char *v = ((struct Token *)gettp(p, 0))->view;
	enum ICode code;
	struct Token *fst, *snd;
	fst = next_get(p, 0);
	snd = next_get(p, 0);
	plist_add(os, fst);
	plist_add(os, snd);
	next_get(p, 0); // skip snd op

	if (sc(v, STR_IMOV)) {
		if (sc(fst->view, STR_EAX)) {
			switch (snd->code) {
			case INT:
				code = IMOV_EAX_INT;
				break;
			default:
				eep(snd, ERR_MOV_EAX);
			}
		} else if (sc(fst->view, STR_EDX)) {
			switch (snd->code) {
			case INT:
				code = IMOV_EDX_INT;
				break;
			default:
				eep(snd, ERR_MOV_EDX);
			}
		} else if (sc(fst->view, STR_EDI)) {
			switch (snd->code) {
			case INT:
				code = IMOV_EDI_INT;
				break;
			default:
				eep(snd, ERR_MOV_EDI);
			}
		} else if (sc(fst->view, STR_ESI)) {
			switch (snd->code) {
			case INT:
				code = IMOV_ESI_INT;
				break;
			case ID:
				code = IMOV_ESI_LABEL;
				break;
			default:
				eep(snd, ERR_MOV_ESI);
			}
		} else
			eep(fst, ERR_MOV);
	} else if (sc(v, STR_PLUS))
		code = IADD;
	else if (sc(v, STR_MINS))
		code = ISUB;
	else if (sc(v, STR_IMUL))
		code = IIMUL;
	// else

	return code;
}
enum ICode syscall_i(struct Pser *p) {
	next_get(p, 0);
	return ISYSCALL;
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
			printf("let %s be %ld bytes\n", var->view, data->size);
			break;
		}
	}

	plist_add(os, var);
	plist_add(os, data);
	return ILET;
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
			code = syscall_i(p);
		else if (n->code == COLO)
			code = label_i(p, os);
		else if (sc(cv, STR_LET))
			code = let_i(p, os);
		else if (sc(cv, STR_IJMP))
			code = jmp_i(p, os);
		else if (sc(cv, STR_SEG))
			code = seg_i(p, os);
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
