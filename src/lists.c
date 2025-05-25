#include "lists.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PList *new_plist(long cap_pace) {
	struct PList *l = malloc(sizeof(struct PList));
	l->cap_pace = cap_pace;
	l->cap = cap_pace;
	l->size = 0;
	l->st = malloc(cap_pace * sizeof(void *));
	return l;
}

void plist_free(struct PList *l) {
	free(l->st);
	free(l);
}

void plist_clear(struct PList *l) {
	if (l->cap != l->cap_pace) {
		l->cap = l->cap_pace;
		l->st = realloc(l->st, l->cap_pace * sizeof(void *));
	}
	l->size = 0;
}

void plist_clear_items_free(struct PList *l) {
	for (long i = 0; i < l->size; i++)
		free(plist_get(l, i));
	plist_clear(l);
}

long plist_add(struct PList *l, void *p) {
	if (l->size >= l->cap) {
		l->cap += l->cap_pace;
		l->st = realloc(l->st, l->cap * sizeof(void *));
	}
	l->st[l->size] = p;
	l->size++;
	return l->size;
}

void *plist_get(struct PList *l, long i) { return l->st[i]; }

void *plist_set(struct PList *l, long i, void *p) {
	void *old = l->st[i];
	l->st[i] = p;
	return old;
}

struct BList *new_blist(long cap_pace) {
	struct BList *l = malloc(sizeof(struct BList));
	l->cap_pace = cap_pace;
	l->cap = cap_pace;
	l->size = 0;
	l->st = malloc(cap_pace * sizeof(uc));
	return l;
}

long blist_add(struct BList *l, uc p) {
	if (l->size >= l->cap) {
		l->cap += l->cap_pace;
		l->st = realloc(l->st, l->cap * sizeof(uc));
	}
	l->st[l->size] = p;
	l->size++;
	return l->size;
}

long blist_cut(struct BList *l) {
	l->cap = l->size;
	if (!l->size){
		free(l->st);
		return 0;
	}
	l->st = realloc(l->st, l->size);
	return l->size;
}

uc blist_get(struct BList *l, long i) { return l->st[i]; }

uc blist_set(struct BList *l, long i, uc p) {
	uc old = l->st[i];
	l->st[i] = p;
	return old;
}

// Byte List Add Times
void blat(struct BList *l, uc *s, long t) {
	if (t <= 0)
		return;
	if (l->cap < l->size + t) {
		l->cap = l->size + t + l->cap_pace;
		l->st = realloc(l->st, l->cap * sizeof(uc));
	}
	memcpy(l->st + l->size, s, t);
	l->size += t;
}

void blist_clear(struct BList *l) {
	if (l->cap != l->cap_pace) {
		l->cap = l->cap_pace;
		l->st = realloc(l->st, l->cap_pace * sizeof(uc));
	}
	l->size = 0;
}

void blist_print(struct BList *l) {
	for (uint32_t i = 0; i < l->size; i++) {
		printf("%02x", l->st[i]);
		putchar(' ');
	}
	putchar('\n');
}

void blist_add_set(struct BList *l, uc sz, long *value, size_t n) {
	if (n < 1)
		return;

	long new_size = l->size + n * sz, old_size = l->size;
	if (l->cap < new_size) {
		l->cap = new_size + l->cap_pace;
		l->st = realloc(l->st, l->cap * sizeof(uc));
	}
	l->size = new_size;

	if (sz == 1)
		memset(l->st + old_size, *value, n * sz);
	else
		for (; old_size < new_size; old_size += sz)
			memcpy(l->st + old_size, value, sz);
}
