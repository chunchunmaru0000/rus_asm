#include "pser.h"
#include <stdio.h>

constr BIP_OPS_ONLY_FOR_INT_NUMS = "Бинарные операции(+, -, *, /) используются "
								   "только для целых чисел и меток.";

struct Oper *bin(struct Pser *p, struct Oper *l, struct Oper *r,
				 struct Token *op) {
	struct Oper *o;

	if ((l->code != OINT && l->code != OREL && l->code != OBIN) ||
		(r->code != OINT && r->code != OREL && r->code != OBIN))
		ee(p->f, op->p, BIP_OPS_ONLY_FOR_INT_NUMS);

	if (l->code == OINT && r->code == OINT) {
		if (op->code == PLUS)
			l->t->num += r->t->num;
		else if (op->code == MINUS)
			l->t->num -= r->t->num;
		else if (op->code == MUL)
			l->t->num *= r->t->num;
		else if (op->code == DIV)
			l->t->num /= r->t->num;
		else
			ee(p->f, op->p, "Эээээ эээЭэЭэ ЭэЭэээ эээ.");

		free(r);
		return l;
	}

	o = malloc(sizeof(struct Oper));
	o->code = OBIN;
	o->t = op;
	// 	REMEMBER: op->num = (long)l, op->real = (double)(long)r;
	op->num = (long)l;
	op->real = (double)(long)r;
	o->sz = l->sz > r->sz ? l->sz : r->sz;

	o->disp_is_rel_flag = 0, o->disp = 0, o->mem_sz = 0, o->mod = MOD_REG,
	o->scale = SCALE_1, o->index = R_NONE, o->base = R_NONE, o->rm = R_NONE,
	o->rex = 0, o->forsed_sz = 0, o->rel_flags = RF_NONE;

	return o;
}

struct Oper *mul_div_expr(struct Pser *p) {
	struct Oper *e = expression(p);
	struct Token *c;
	loop {
		c = cur_token(p);
		if (c->code == MUL || c->code == DIV) {
			next_get(p, 0);
			e = bin(p, e, expression(p), c);
		} else
			break;
	}
	return e;
}
struct Oper *add_sub_expr(struct Pser *p) {
	struct Oper *e = mul_div_expr(p);
	struct Token *c;
	loop {
		c = cur_token(p);
		if (c->code == PLUS || c->code == MINUS) {
			next_get(p, 0);
			e = bin(p, e, mul_div_expr(p), c);
		} else
			break;
	}
	return e;
}

struct Oper *find_any_label_in_bin_tree(struct Oper *o) {
	struct Oper *l, *r, *result = 0;
	l = (struct Oper *)o->t->num;
	r = (struct Oper *)(long)o->t->real;

	if (l->code == OREL)
		result = l;
	else if (r->code == OREL)
		result = r;
	else if (l->code == OBIN && (l = find_any_label_in_bin_tree(l)))
		result = l;
	else if (r->code == OBIN && (r = find_any_label_in_bin_tree(r)))
		result = r;

	return result;
}
