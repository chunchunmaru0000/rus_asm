#include "lists.h"
#include <stdlib.h>

struct PList *new_plist(long cap_pace) {
	struct PList *l = malloc(sizeof(struct PList));
	l->cap_pace = cap_pace;
	l->cap = cap_pace;
	l->size = 0;
	l->st = malloc(cap_pace * sizeof(void *));
	return l;
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
