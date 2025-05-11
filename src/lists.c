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

uc blist_get(struct BList *l, long i) { return l->st[i]; }

uc blist_set(struct BList *l, long i, uc p) {
	uc old = l->st[i];
	l->st[i] = p;
	return old;
}

// Byte List Add Times
void blat(struct BList *l, uc *s, long t) {
	//	for (long i = 0; i < t; i++)
	//		blist_add(l, s[i]);
	if (t <= 0)
		return;
	if (l->cap < l->size + t) {
		l->cap += t;
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
