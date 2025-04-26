#include "gner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Byte List Add Times
void blat(struct BList *l, const uc *s, long t) {
	for (long i = 0; i < t; i++)
		blist_add(l, s[i]);
}

struct Gner *new_gner(struct PList *is) {
	struct Gner *g = malloc(sizeof(struct Gner));
	g->is = is;
	g->pos = 0;
	g->prol = new_blist(100);
	g->text = new_blist(100);
	return g;
}

void gen_Linux_ELF_86_64_prolog(struct Gner *, struct Header *);
void gen_Linux_ELF_86_64_text(struct Gner *, struct Header *);

void gen(struct Gner *g, enum Target tr) {
	struct Header *h = new_header();

	switch (tr) {
	case Linux_ELF_86_64:
		gen_Linux_ELF_86_64_text(g, h);
		gen_Linux_ELF_86_64_prolog(g, h);
		break;
	case Linux_OBJ_86_64:
		break;
	case Windows_EXE_86_64:
		break;
	}
}

void eeg(const char *msg, struct Inst *i) {
	fprintf(stderr, "%s:%ld:%ld %s\n", i->file, i->line, i->col, msg);
	exit(1);
}

// header MLE - Machine code Linux Elf
const uc MLE_MAGIC[] = {0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00,
						0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uc MLE_E_TYPE[] = {0x02, 0x00};
const uc MLE_E_MACHINE[] = {0x3e, 0x00};
const uc MLE_E_VERSION[] = {0x01, 0x00, 0x00, 0x00};
// machine codes
const uc MLESYSCALL[] = {0x0f, 0x05};

void gen_Linux_ELF_86_64_prolog(struct Gner *g, struct Header *h) {
	struct BList *l = g->prol;
	blat(l, MLE_MAGIC, 9);
	blat(l, MLE_E_TYPE, 2);
	blat(l, MLE_E_MACHINE, 2);
	blat(l, MLE_E_VERSION, 4);
	blat(l, h->entry, 8);
	blat(l, h->phoff, 8);
	blat(l, h->shoff, 8);
	blat(l, h->flags, 4);
	blat(l, h->ehsize, 2);
	blat(l, h->phsize, 2);
	blat(l, h->phnum, 2);
	blat(l, h->shsize, 2);
	blat(l, h->shnum, 2);
	blat(l, h->shstrndx, 2);
}

void *alloc_len(long len, long *buf_len) {
	*buf_len = len;
	return malloc(len);
}

void gen_Linux_ELF_86_64_text(struct Gner *g, struct Header *h) {
	long i = 0, buf_len;
	long *blp = &buf_len; // buf len ptr
	struct Inst *in;
	uc *ibuff;
	for (; i < g->is->size; i++) {
		in = plist_get(g->is, i);

		switch (in->code) {
		case ISYSCALL:
			ibuff = alloc_len(2, blp);
			memcpy(ibuff, MLESYSCALL, 2);
			break;
		}
		blat(g->text, ibuff, buf_len);
	}
}
