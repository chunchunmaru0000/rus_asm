#include <stdlib.h>

struct List {
	long e_size; // one element size
	void *start;
	long cap;
};

struct List *new_list(long e_size, long cap) {
	struct List *l = malloc(sizeof(struct List));
	l->start = malloc(cap * e_size);
	l->e_size = e_size;
	l->cap = cap;
	return l;
}

long list_inc(struct List *l, long es) {
	long new_cap = l->cap + es;
	l->start = realloc(l->start, new_cap * l->e_size);
	l->cap = new_cap;
	return new_cap;
}
