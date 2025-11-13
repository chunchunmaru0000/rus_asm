#include "pser.h"
#include <stdio.h>

constr BIP_OPS_ONLY_FOR_INT_NUMS = "Бинарные операции(+, -, *, /) используются "
								   "только для целых чисел и меток.";

struct Oper *bin(struct Pser *p, struct Oper *l, struct Oper *r,
				 struct Token *op) {

	if ((l->code != OINT && l->code != OREL) ||
		(r->code != OINT && r->code != OREL))
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
	exit(123);
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
