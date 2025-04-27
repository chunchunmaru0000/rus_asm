#include "gner.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void eeg(const char *msg, struct Inst *i) {
	fprintf(stderr, "%s:%ld:%ld %s\n", i->file, i->line, i->col, msg);
	exit(1);
}

// Byte List Add Times
void blat(struct BList *l, uc *s, long t) {
	for (long i = 0; i < t; i++)
		blist_add(l, s[i]);
}

struct Gner *new_gner(struct PList *is, enum Target t) {
	struct Gner *g = malloc(sizeof(struct Gner));
	g->t = t;
	g->is = is;
	g->pos = 0;
	g->prol = new_blist(100);
	g->text = new_blist(100);
	return g;
}

void gen_Linux_ELF_86_64_prolog(struct Gner *, struct ELFH *);
void gen_Linux_ELF_86_64_text(struct Gner *, long *);
struct ELFH *new_elfh(struct Gner *, long, long, short, long, short);

void gen(struct Gner *g) {
	struct ELFH *h;
	long entry;

	switch (g->t) {
	case Linux_ELF_86_64:
		gen_Linux_ELF_86_64_text(g, &entry);
		h = new_elfh(g, entry, 0x40, 0x01, 0x00, 0x00);
		gen_Linux_ELF_86_64_prolog(g, h);

		free(h);
		break;
	case Linux_OBJ_86_64:
		break;
	case Windows_EXE_86_64:
		break;
	}
}

const uc ELFH_MAG[] = {0x7f, 0x45, 0x4c, 0x46};
const uc ELFH_INDENT[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
const uc ELFH_TYPE_EXEC[] = {0x02, 0x00};
const uc ELFH_TYPE_DYN[] = {0x03, 0x00}; // not sure about this
const uc ELFH_MACHINE_AMDx86_64[] = {0x3e, 0x00};
const uc ELFH_FORMAT_VERSION[] = {0x01, 0x00, 0x00, 0x00}; // only possible

struct ELFH *new_elfh(struct Gner *g, long entrytoff, long phoff, short phn,
					  long shoff, short shn) {
	struct ELFH *h = malloc(sizeof(struct ELFH));

	memcpy(h->mag, ELFH_MAG, 4);
	h->elf_class = 2;	// 2 for 64 bit
	h->data = 1;		// little endian
	h->elf_version = 1; // only possible
	h->osabi = 0;		// UNIX System V ABI
	memcpy(h->indent, ELFH_INDENT, 8);

	memcpy(h->machine, ELFH_MACHINE_AMDx86_64, 2);
	memcpy(h->format_version, ELFH_FORMAT_VERSION, 4);
	memcpy(h->entry, &entrytoff, sizeof(long));
	memcpy(h->phoff, &phoff, sizeof(long));
	memcpy(h->shoff, &shoff, sizeof(long));

	switch (g->t) {
	case Linux_ELF_86_64:
		memcpy(h->type, ELFH_TYPE_EXEC, 2);
		break;
	case Linux_OBJ_86_64:
		memcpy(h->type, ELFH_TYPE_DYN, 2);
		break;
	default:
		eeg("НЕ ДОСТУПНАЯ ЦЕЛЬ КОМПИЛЯЦИИ НА ДАННЫЙ МОМЕНТ", g->is->st[0]);
	}
	return h;
}

// machine codes
const uc MLESYSCALL[] = {0x0f, 0x05};

void gen_Linux_ELF_86_64_prolog(struct Gner *g, struct ELFH *h) {
	struct BList *l = g->prol;
	blat(l, (uc *)h, sizeof(struct ELFH));
}

void *alloc_len(long len, long *buf_len) {
	*buf_len = len;
	return malloc(len);
}

long get_label_offset(char *label) { return 0xb0; }

void gen_Linux_ELF_86_64_text(struct Gner *g, long *entry_off) {
	long i = 0, buf_len;
	long *blp = &buf_len; // buf len ptr
	struct Inst *in;
	struct Token *tok;
	uc *ibuff;
	for (; i < g->is->size; i++) {
		in = plist_get(g->is, i);

		switch (in->code) {
		case ISYSCALL:
			ibuff = alloc_len(2, blp);
			memcpy(ibuff, MLESYSCALL, 2);
			break;
		case IENTRY:
			tok = in->os->st[0];
			*entry_off = get_label_offset(tok->view);
			break;
		default:
			eeg("НЕИЗВЕСТНАЯ ИНСТРУКЦИЯ", in);
		}
		blat(g->text, ibuff, buf_len);
	}
}
