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
	return i;
}

struct Token *get(struct Pser *p, long off) {
	long i = p->pos + off;
	return p->ts->size > i ? p->ts->st[i] : p->ts->st[p->ts->size - 1];
}

struct Token *next_get(struct Pser *p, long off) {
	p->pos++;
	return get(p, off);
}

// directives
const char *STR_ENTRY = "вход";
const char *STR_SEG = "сегмент";
const char *STR_SEG_TEXT = "\".текст\"";
const char *STR_SEG_DATA = "\".данные\"";
const char *STR_EOF = "__КФ__";
// instruction words
const char *STR_LET = "быть";
const char *STR_PLUS = "плюс";
const char *STR_MINS = "минс";
const char *STR_IMUL = "зумн";
const char *TWO_OPS_STRS[] = {"быть", "плюс", "минс", "зумн"};
const long LTWO_OPS_STRS = 4;
const char *STR_CALL = "зов";
const char *STR_SCAL = "сзов";
// registers words
const char *STR_RAX = "рах";
const char *STR_RBX = "рбх";
const char *STR_RCX = "рсх";
const char *STR_RDX = "рдх";
const char *STR_RDI = "рди";
const char *STR_RSI = "рси";
const char *STR_RSP = "рсп";
const char *STR_RBP = "рбп";

uc sc(char *view, const char *str) {
	printf("[%s]==[%s]\n", view, str);
	return strcmp(view, str) == 0;
}
uc cont_str(char *view, const char **strs, long strs_len) {
	for (int i = 0; i < strs_len; i++)
		if (sc(view, strs[i]))
			return 1;
	return 0;
}

enum ICode entry_i(struct Pser *p) {
	next_get(p, 0);
	next_get(p, 0);
	return IENTRY;
}
enum ICode seg_i(struct Pser *p) {
	next_get(p, 0);
	next_get(p, 0);
	return ISEGMENT;
}
enum ICode two_ops_i(struct Pser *p) {
	next_get(p, 0);
	next_get(p, 0);
	next_get(p, 0);
	return IADD;
}

enum ICode skip(struct Pser *p, long s) {
	while (s--)
		next_get(p, 0);
	return ILABEL;
}

struct Inst *get_inst(struct Pser *p) {
	struct PList *os = new_plist(4);
	struct Token *cur = get(p, 0);
	while (cur->code == SLASHN)
		cur = next_get(p, 0);
	char *cv = cur->view;
	enum ICode code;

	if (cur->code == EF)
		code = IEOI;
	else if (cur->code == ID) {
		if (sc(cv, STR_ENTRY))
			code = entry_i(p);
		else if (sc(cv, STR_SEG))
			code = seg_i(p);
		else if (cont_str(cv, TWO_OPS_STRS, LTWO_OPS_STRS))
			code = two_ops_i(p);
		else
			code = skip(p, 2); // eep(cur, "НЕИЗВЕСТНАЯ КОМАНДА");
	} else if (cur->code == STR)
		code = skip(p, 5);
	else
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
