#include "pser.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

void ee_token(struct Fpfc *f, struct Token *t, char *msg) { // error exit
	fprintf(stderr, "%s%s:%d:%d:%s ОШИБКА: %s [%s]:[%d]%s\n", COLOR_WHITE,
			f->path, t->p->line, t->p->col, COLOR_RED, msg, t->view, t->code,
			COLOR_RESET);
	print_source_line(f->code, t->p->line, COLOR_LIGHT_RED);
	exit(1);
}

// print warning
void pw(struct Fpfc *f, struct Pos *p, const char *const msg) {
	if (!NEED_WARN)
		return;
	fprintf(stderr, "%s%s:%d:%d%s ПРЕДУПРЕЖДЕНИЕ: %s%s\n", COLOR_WHITE, f->path,
			p->line, p->col, COLOR_LIGHT_PURPLE, msg, COLOR_RESET);
	print_source_line(f->code, p->line, COLOR_LIGHT_PURPLE);
}

int get_reg_field(enum RegCode rm) {
	int f = -1;
	if (is_f_reg8(rm))
		f = rm - R_AL;
	else if (is_f_reg16(rm))
		f = rm - R_AX;
	else if (is_f_reg32(rm))
		f = rm - R_EAX;
	else if (is_f_reg64(rm))
		f = rm - R_RAX;
	else if (is_f_xmm(rm))
		f = rm - R_XMM0;
	else if (is_f_mm(rm))
		f = rm - R_MM0;
	else if (is_f_seg(rm))
		f = rm - R_ES;
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
	struct Pser *p = malloc(sizeof(struct Pser));
	struct Tzer *t = new_tzer(filename);
	p->f = t->f;

	struct PList *ts = tze(t, 10);
	free(t);
	p->pos = 0;
	p->ts = ts;
	p->debug = debug;
	p->ds = new_plist(3);
	return p;
}

struct Inst *new_inst(struct Pser *p, enum ICode code, struct PList *os,
					  struct Token *t) {
	struct Inst *i = malloc(sizeof(struct Inst));
	struct Pos *pos = malloc(sizeof(struct Pos));
	i->f = p->f;
	pos->col = t->p->col;
	pos->line = t->p->line;
	i->p = pos;

	i->code = code;
	i->os = os;
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
const char *STR_INCLUDE = "влечь";
// directives
const char *STR_ENTRY = "вход";
const char *STR_SEG = "участок";
const char *STR_SEG_R = "чит";
const char *STR_SEG_W = "изм";
const char *STR_SEG_X = "исп";
const char *STR_EOF = "_КОНЕЦ_ФАЙЛА_";
const char *STR_LET = "пусть";
const char *STR_ALIGN = "равнять";

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

enum ICode n_ops_i(struct Pser *p, int n, struct PList *os, enum ICode code) {
	next_get(p, 0); // skip instruction
	for (int i = 0; i < n; i++)
		plist_add(os, expression(p));
	return code;
}

enum ICode label_i(struct Pser *p, struct PList *os) {
	plist_add(os, gettp(p, 0));
	next_get(p, 0); // skip label
	next_get(p, 0); // skip :
	return ILABEL;
}

char *INVALID_SIZE_NOT_FOUND =
	"Ожидался размер операнда: <байт> <дбайт> <чбайт> <вбайт>.";
char *INVALID_SIZE_OF_FPN = "Неверный размер для числа с плавающей точкой, "
							"ожидался размер <чбайт> или <вбайт>.";
char *AWAITED_SLASHN = "Ожидался перевод строки.";
char *INVALID_DEFN_USAGE = "Неверное использование определения.";
char *DEFN_NOT_FOUND = "Определение не было найдено.";
char *INVALID_RESERVED_USAGE_BEGIN =
	"Нельзя начинать объявление переменной со слова \"запас\".";
char *INVALID_RESERVED_USAGE =
	"Нельзя так использовать слово \"запас\"."
	"\nПравильный синтаксис для использования слова \"запас\":"
	"\n\tзапас значение раз.";
char *EXPEXTED_INT_FOR_RESERVED_TIMES =
	"Правильный синтаксис для использования слова \"запас\":"
	"\n\tзапас значение раз.";
char *INVALID_INT_FOR_RESERVED_TIMES =
	"Число означающее количество раз значений не может быть меньше 1.";
char *LABEL_ADDR_CANT_BE_LESS_THAN_DWORD =
	"Размер метки не может быть меньше 4-х байт.";

// zero
const long ZERO = 0;

enum ICode let_i(struct Pser *p, struct PList *os) {
	struct BList *data = new_blist(8);
	struct PList *not_plovs = new_plist(1);
	struct Token *c = next_get(p, 0), *name; // skip let word
	enum ICode code = ILET;
	long value;
	struct Defn *d;
	struct Oper *o;

	while (c->code == SLASH || c->code == SLASHN)
		c = next_get(p, 0);

	int size = is_size_word(c->view), old_sz;
	if (size > 0) // -1 is RESERVED
		code = IDATA;
	else {
		if (size < 0)
			ee_token(p->f, c, INVALID_RESERVED_USAGE_BEGIN);

		name = c;
		do
			c = next_get(p, 0);
		while (c->code == SLASH || c->code == SLASHN);
		size = is_size_word(c->view);
		if (!size)
			ee_token(p->f, c, INVALID_SIZE_NOT_FOUND);
		if (size == RESERVED)
			ee_token(p->f, c, INVALID_RESERVED_USAGE_BEGIN);
	}

	loop {
		c = next_get(p, 0);
		if (c->code == SLASH) {
			c = next_get(p, 0); // skip slash get \n
			if (c->code != SLASHN)
				ee_token(p->f, c, AWAITED_SLASHN);
			continue;
		}
		if (c->code == SEP) {
			next_get(p, 0); // skip separator
			break;
		}
		if (c->code == ID) {
			old_sz = size;
			size = is_size_word(c->view);
			if (size) {
				if (size == RESERVED) {
					size = old_sz;
					next_get(p, 0); // skip запас word

					o = expression(p);
					if (o->code != OINT)
						ee_token(p->f, c, INVALID_RESERVED_USAGE);
					value = o->t->number;

					o = expression(p);
					if (o->code != OINT)
						ee_token(p->f, c, EXPEXTED_INT_FOR_RESERVED_TIMES);

					blist_add_set(data, size, &value, o->t->number);
					p->pos--; // here expression eats next token so
				}
				continue;
			}

			size = old_sz;
			d = is_defn(p, c->view);

			if (!d) {
				if (size < DWORD)
					ee_token(p->f, c, LABEL_ADDR_CANT_BE_LESS_THAN_DWORD);

				// add not plov cuz its may be a label
				d = new_not_plov(c->view, data->size, ADDR);
				((struct Usage *)(d->value))->place = data->size;
				((struct Usage *)(d->value))->cmd_end = size;
				plist_add(not_plovs, d);

				// zero the future place of an address by size of size
				blat(data, (uc *)&ZERO, size);

				continue;
			}

			o = d->value;
			if (o->code == OINT)
				blat(data, (uc *)&o->t->number, size);
			else if (o->code == OFPN) {
				c->fpn = o->t->fpn;
				goto let_i_real;
			} else
				ee_token(p->f, c, INVALID_DEFN_USAGE);
		} else if (c->code == INT)
			blat(data, (uc *)&c->number, size);
		else if (c->code == STR)
			blat(data, (uc *)c->str->st, c->str->size);
		else if (c->code == REAL) {
		let_i_real:
			if (size == QWORD)
				blat(data, (uc *)&c->fpn, QWORD);
			else if (size == DWORD) {
				float value = c->fpn;
				blat(data, (uc *)&value, DWORD);
			} else
				ee_token(p->f, c, INVALID_SIZE_OF_FPN);
		} else {
			if (p->debug) {
				if (code == ILET)
					printf("пусть %s будет %d байт\n", name->view, data->size);
				else
					printf("пусть %d байт\n", data->size);
			}
			break;
		}
	}

	if (code == ILET)
		plist_add(os, name);

	plist_add(os, data);
	plist_add(os, not_plovs);

	return code;
}

// TODO: is it possible to do better str search
enum ICode define_pd(struct Pser *p) {
	struct Defn *d;
	char *view = next_get(p, 0)->view;
	next_get(p, 0); // skip name token
	struct Oper *value = expression(p);

	for (long i = 0; i < p->ds->size; i++) {
		d = plist_get(p->ds, i);

		if (sc(view, d->view)) {
			// expression can be freed here cuz it doesnt holds pointers that
			// it mallocs, it only borrows them
			free(d->value);
			d->value = value;
			return INONE;
		}
	}

	d = malloc(sizeof(struct Defn));
	d->view = view;
	d->value = value;
	plist_add(p->ds, d);
	return INONE;
}

const char *const EXPECTED_POSITIVE_INT_FOR_ALIGN =
	"Для функции \"равнять\" в качестве единственного аргумента ожидалось "
	"положительное целое число.";

enum ICode align_pd(struct Pser *p, struct PList *os) {
	next_get(p, 0); // skip равнять

	struct Oper *o = expression(p);
	if (o->code != OINT || o->t->number < 0)
		ee(p->f, o->t->p, EXPECTED_POSITIVE_INT_FOR_ALIGN);
	plist_add(os, o);
	if (gettp(p, 0)->code == SLASHN)
		return IALIGN;

	o = expression(p);
	if (o->code != OINT || o->t->number < 0)
		ee(p->f, o->t->p, EXPECTED_POSITIVE_INT_FOR_ALIGN);
	plist_add(os, o);
	return IALIGN;
}

const char *const EXPECTED_STR_AS_INCLUDE_PATH =
	"В выраженнии \"влечь\" в качестве параметра ожидалась строка "
	"с путем к файлу.";

enum ICode include_pd(struct Pser *p, struct PList *os) {
	// TODO: make relative folder addressation
	struct Token *path = next_get(p, 0);
	if (path->code != STR)
		ee(p->f, path->p, EXPECTED_STR_AS_INCLUDE_PATH);

	next_get(p, 0);
	plist_add(os, path);
	return IINCLUDE;
}

int ops_i(struct Pser *p, struct PList *os, char *view, enum ICode *c) {
	size_t i;
	for (i = 0; i < arr_l(VAR_OPS_WORDS); i++)
		if (sc(view, VAR_OPS_WORDS[i].view)) {
			struct Token *cur = next_get(p, 0);
			while (cur->code != SLASHN && cur->code != SEP && cur->code != EF) {
				plist_add(os, expression(p));
				cur = gettp(p, 0);
			}
			// this is still can be a wrong instruction because its just
			// wont have the right opsCode in cmnd so dont care here about it
			*c = VAR_OPS_WORDS[i].inst;
			return 1;
		}
	for (i = 0; i < arr_l(TWO_OPS_WORDS); i++)
		if (sc(view, TWO_OPS_WORDS[i].view)) {
			*c = n_ops_i(p, 2, os, TWO_OPS_WORDS[i].inst);
			return 1;
		}
	for (i = 0; i < arr_l(ZERO_OPS_WORDS); i++)
		if (sc(view, ZERO_OPS_WORDS[i].view)) {
			next_get(p, 0);
			*c = ZERO_OPS_WORDS[i].inst;
			return 1;
		}
	for (i = 0; i < arr_l(ONE_OPS_WORDS); i++)
		if (sc(view, ONE_OPS_WORDS[i].view)) {
			*c = n_ops_i(p, 1, os, ONE_OPS_WORDS[i].inst);
			return 1;
		}
	for (i = 0; i < arr_l(TRI_OPS_WORDS); i++)
		if (sc(view, TRI_OPS_WORDS[i].view)) {
			*c = n_ops_i(p, 3, os, TRI_OPS_WORDS[i].inst);
			return 1;
		}
	return 0;
}

struct Inst *get_inst(struct Pser *p) {
	struct PList *os = new_plist(4);
	struct Token *cur = gettp(p, 0), *n;
	while (cur->code == SLASHN || cur->code == SEP)
		cur = next_get(p, 0);
	char *cv = cur->view;
	enum ICode code;
	n = gettp(p, 1);

	// fill *os in funcs
	if (cur->code == EF)
		code = IEOI;
	else if (cur->code == ID) {
		if (ops_i(p, os, cv, &code))
			;
		else if (n->code == COLO)
			code = label_i(p, os);
		else if (sc(cv, STR_LET))
			code = let_i(p, os);
		else if (sc(cv, STR_DEFINE))
			code = define_pd(p);
		else if (sc(cv, STR_ALIGN))
			code = align_pd(p, os);
		else if (sc(cv, STR_INCLUDE))
			code = include_pd(p, os);
		else if (sc(cv, STR_SEG))
			code = seg_i(p, os);
		else if (sc(cv, STR_ENTRY))
			code = entry_i(p, os);
		else
			ee_token(p->f, cur, "НЕИЗВЕСТНАЯ КОМАНДА");
	} else if (cur->code == INC)
		code = n_ops_i(p, 1, os, IINC);
	else if (cur->code == DEC)
		code = n_ops_i(p, 1, os, IDEC);
	else
		ee_token(p->f, cur, "НЕИЗВЕСТНАЯ КОМАНДА");

	return new_inst(p, code, os, cur);
}

void include_in_is(struct Pser *p, struct PList *is, struct Inst *i) {
	struct Token *path = plist_get(i->os, 0);
	blist_add(path->str, 0); // string 0 terminator

	struct Pser *tmp_p = new_pser((char *)path->str->st, p->debug);
	plist_free(tmp_p->ds);
	tmp_p->ds = p->ds;
	struct PList *inc = pse(tmp_p);

	for (uint32_t j = 0; j < inc->size; j++)
		plist_add(is, plist_get(inc, j));

	free(tmp_p);
	free(inc);
}

struct PList *pse(struct Pser *p) {
	struct PList *is = new_plist(p->ts->cap_pace); // why not

	struct Inst *i = get_inst(p);
	while (i->code != IEOI) {
		if (i->code == IINCLUDE)
			include_in_is(p, is, i);
		else if (i->code != INONE)
			plist_add(is, i);
		i = get_inst(p);
	}
	plist_add(is, i);

	return is;
}
