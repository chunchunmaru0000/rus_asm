#include "gner.h"
#include <stdint.h>
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

	g->entry = 0;
	g->phs = new_plist(3);
	g->shs = new_plist(1);
	return g;
}

void gen_Linux_ELF_86_64_prolog(struct Gner *, struct ELFH *);
void gen_Linux_ELF_86_64_text(struct Gner *);
struct ELFPH *new_ph(int, uc r, uc w, uc x, uint64_t, uint64_t, uint64_t);
struct ELFH *new_elfh(struct Gner *, long, long, short, long, short);

void gen(struct Gner *g) {
	switch (g->t) {
	case Linux_ELF_86_64:
		gen_Linux_ELF_86_64_text(g);

		plist_add(g->phs, new_ph(1, 1, 0, 1, 0, 10, 10));
		struct ELFH *h;
		h = new_elfh(g, g->entry, 0x40, g->phs->size, 0x00, g->shs->size);

		gen_Linux_ELF_86_64_prolog(g, h);
		for (int i = 0; i < g->phs->size; i++)
			blat(g->prol, plist_get(g->phs, i), sizeof(struct ELFPH));
		for (int i = 0; i < g->shs->size; i++)
			blat(g->prol, plist_get(g->shs, i), sizeof(struct ELFSH));

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
const uc ELFH_MACHINE_AMD_x86_64[] = {0x3e, 0x00};
const uc ELFH_FORMAT_VERSION[] = {0x01, 0x00, 0x00, 0x00}; // only possible
const uc ELFH_SHSTRNDX[] = {0x00, 0x00};
const uc ELFH_FLAGS[] = {0x00, 0x00, 0x00, 0x00};
const uc ELFH_EHSIZE[] = {0x40, 0x00}; // 64 for 64 bit
const uc ELFH_PHSIZE[] = {0x38, 0x00}; // 56 for 64 bit
const uc ELFH_SHSIZE[] = {0x40, 0x00}; // 64 for 64 bit

struct ELFH *new_elfh(struct Gner *g, long entrytoff, long phoff, short phn,
					  long shoff, short shn) {
	struct ELFH *h = malloc(sizeof(struct ELFH));

	memcpy(h->mag, ELFH_MAG, 4);
	h->elf_class = 2;	// 2 for 64 bit
	h->data = 1;		// little endian
	h->elf_version = 1; // only possible
	h->osabi = 0;		// UNIX System V ABI
	memcpy(h->indent, ELFH_INDENT, 8);

	memcpy(h->machine, ELFH_MACHINE_AMD_x86_64, 2);
	memcpy(h->format_version, ELFH_FORMAT_VERSION, 4);
	memcpy(h->entry, &entrytoff, sizeof(long));
	memcpy(h->phoff, &phoff, sizeof(long));
	memcpy(h->shoff, &shoff, sizeof(long));
	memcpy(h->flags, ELFH_FLAGS, 4);
	memcpy(h->ehsize, ELFH_EHSIZE, 2);
	memcpy(h->phsize, ELFH_PHSIZE, 2);
	memcpy(h->phnum, &phn, 2);
	memcpy(h->shsize, ELFH_SHSIZE, 2);
	memcpy(h->shnum, &shn, 2);
	memcpy(h->shstrndx, ELFH_SHSTRNDX, 2);

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

struct ELFPH *new_ph(int t, uc r, uc w, uc x, uint64_t off, uint64_t addr,
					 uint64_t sz) {
	struct ELFPH *ph = malloc(sizeof(struct ELFPH));
	ph->type = t;
	ph->flags = (r << 2) | (w << 1) | x;
	ph->offset = off;
	ph->vaddr = addr;
	ph->paddr = addr;
	ph->filesz = sz;
	ph->memsz = sz;
	ph->align = 0x0100;

	return ph;
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

void gen_Linux_ELF_86_64_text(struct Gner *g) {
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
			g->entry = get_label_offset(tok->view);
			break;
			//		default:
			//			eeg("НЕИЗВЕСТНАЯ ИНСТРУКЦИЯ", in);
		}
		blat(g->text, ibuff, buf_len);
	}
}
