#include "lsts.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct PList *new_plist(uint32_t cap_pace) {
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

void plist_re(struct PList *l) {
	if (l->cap != l->cap_pace) {
		l->cap = l->cap_pace;
		l->st = realloc(l->st, l->cap_pace * sizeof(void *));
	}
	l->size = 0;
}

void plist_clear_items_free(struct PList *l) {
	for (uint32_t i = 0; i < l->size; i++)
		free(plist_get(l, i));
	plist_clear(l);
}

void plist_free_items_free(struct PList *l) {
	for (uint32_t i = 0; i < l->size; i++)
		free(plist_get(l, i));
	plist_free(l);
}

// void plist_cut_items_free(struct PList *l, long cut_to) {
// 	long i, j;
// 	void *value;
//
// 	for (i = 0; i < l->size; i++) {
// 		value = plist_get(l, i);
//
// 		if (usage->ic >= cut_to) {
// 			for (j = i; j < l->size; j++)
// 				free(plist_get(l, j));
// 			l->size = i;
// 			break;
// 		}
// 	}
// }

uint32_t plist_add(struct PList *l, void *p) {
	if (l->size >= l->cap) {
		l->cap += l->cap_pace;
		l->st = realloc(l->st, l->cap * sizeof(void *));
	}
	l->st[l->size] = p;
	l->size++;
	return l->size;
}

void *plist_get(struct PList *l, uint32_t i) { return l->st[i]; }

void *plist_set(struct PList *l, uint32_t i, void *p) {
	void *old = l->st[i];
	l->st[i] = p;
	return old;
}

struct BList *new_blist(uint32_t cap_pace) {
	struct BList *l = malloc(sizeof(struct BList));
	l->cap_pace = cap_pace;
	l->cap = cap_pace;
	l->size = 0;
	l->st = malloc(cap_pace * sizeof(uc));
	return l;
}

struct BList *blist_from_str(char *str, uint32_t str_len) {
	struct BList *l = malloc(sizeof(struct BList));
	// 	l->cap_pace = str_len + 1;
	// 	l->cap = str_len + 1;
	// 	l->st = malloc(l->cap_pace * sizeof(uc));
	// 	l->size = 0;
	//
	// 	blat(l, (uc *)str, str_len + 1);
	// 	l->size--;

	l->cap_pace = 8;
	l->size = str_len;
	l->cap = str_len;
	l->st = (uc *)str;
	return l;
}

void convert_blist_to_blist_from_str(struct BList *l) {
	blist_add(l, 0);
	// l->cap_pace = 0;
	l->size--;
}

uint32_t blist_add(struct BList *l, uc p) {
	if (l->size >= l->cap) {
		l->cap += l->cap_pace;
		l->st = realloc(l->st, l->cap * sizeof(uc));
	}
	l->st[l->size] = p;
	l->size++;
	return l->size;
}

uint32_t blist_cut(struct BList *l) {
	l->cap = l->size;
	if (!l->size)
		return 0;
	l->st = realloc(l->st, l->size);
	return l->size;
}

uc blist_get(struct BList *l, uint32_t i) { return l->st[i]; }

uc blist_set(struct BList *l, uint32_t i, uc p) {
	uc old = l->st[i];
	l->st[i] = p;
	return old;
}

// Byte List Add Times
void blat(struct BList *l, uc *s, uint32_t t) {
	if (t <= 0)
		return;
	if (l->cap < l->size + t) {
		l->cap = l->size + t + l->cap_pace;
		l->st = realloc(l->st, l->cap * sizeof(uc));
	}
	memcpy(l->st + l->size, s, t);
	l->size += t;
}

void blist_re(struct BList *l) {
	if (l->cap != l->cap_pace) {
		l->cap = l->cap_pace;
		l->st = realloc(l->st, l->cap_pace * sizeof(uc));
	}
	l->size = 0;
}

void blist_clear_free(struct BList *l) {
	free(l->st);
	free(l);
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

	uint32_t new_size = l->size + n * sz, old_size = l->size;
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

struct BList *copy_str(struct BList *src) {
	struct BList *dest = new_blist(32);
	blat_blist(dest, src);
	convert_blist_to_blist_from_str(dest);
	return dest;
}

struct BList *copy_blist(struct BList *l) {
	struct BList *copy = new_blist(l->cap_pace);
	blat_blist(copy, l);
	return copy;
}
