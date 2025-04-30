#include "gner.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void eeg(const char *msg, struct Inst *i) {
	fprintf(stderr, "%s:%ld:%ld %s\n", i->file, i->line, i->col, msg);
	exit(1);
}

struct Gner *new_gner(struct PList *is, enum Target t) {
	struct Gner *g = malloc(sizeof(struct Gner));
	g->t = t;
	g->is = is;
	g->pos = 0;
	g->prol = new_blist(100);
	g->text = new_blist(100);

	g->pie = 0x400000;
	g->lps = new_plist(5);
	g->phs = new_plist(3);
	g->shs = new_plist(1);
	return g;
}

void gen_Linux_ELF_86_64_prolog(struct Gner *);
void gen_Linux_ELF_86_64_text(struct Gner *);
struct ELFPH *new_ph(int, int, uint64_t, uint64_t, uint64_t);
struct ELFH *new_elfh(struct Gner *, long, long, short, long, short);
struct Plov *new_label(struct Gner *, struct Inst *);

void gen(struct Gner *g) {
	switch (g->t) {
	case Linux_ELF_86_64:
		gen_Linux_ELF_86_64_prolog(g);
		gen_Linux_ELF_86_64_text(g);

		// is it should be here or do separate function like bze_prolog
		blat(g->prol, (uc *)g->elfh, sizeof(struct ELFH));
		for (int i = 0; i < g->phs->size; i++)
			blat(g->prol, plist_get(g->phs, i), sizeof(struct ELFPH));
		for (int i = 0; i < g->shs->size; i++)
			blat(g->prol, plist_get(g->shs, i), sizeof(struct ELFSH));
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
	h->entry = entrytoff;
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

struct ELFPH *new_ph(int t, int flags, uint64_t off, uint64_t addr,
					 uint64_t sz) {
	struct ELFPH *ph = malloc(sizeof(struct ELFPH));
	ph->type = t;
	ph->flags = flags;
	ph->offset = off;
	ph->vaddr = addr;
	ph->paddr = addr;
	ph->filesz = sz;
	ph->memsz = sz;
	ph->align = 0x1000;

	return ph;
}

struct Plov *new_label(struct Gner *g, struct Inst *in) {
	struct Plov *p = malloc(sizeof(struct Plov));

	int segment_place = g->phs->size - 1;
	p->l = ((struct Token *)in->os->st[0])->view;
	p->si = segment_place;
	p->us = new_plist(4);
	p->a = g->pie; // + first ph memsz;

	return p;
}

// machine codes
const uc MLESYSCALL[] = {0x0f, 0x05};

void gen_Linux_ELF_86_64_prolog(struct Gner *g) {
	long i;
	int *flags;
	struct Inst *in;
	struct ELFPH *ph;
	// struct ELFSH *sh;

	for (i = 0; i < g->is->size; i++) {
		in = plist_get(g->is, i);

		if (in->code == ISEGMENT) {
			flags = plist_get(in->os, 0);
			// type =   1,  flags, offset,     adress,     size
			ph = new_ph(1, *flags, 0x7366666f, 0x72646461, 0x657a6973);
			plist_add(g->phs, ph);
		} else if (in->code == ILABEL || in->code == ILET)
			plist_add(g->lps, new_label(g, in));
	}
	// entry = 0 but will be editet
	g->elfh = new_elfh(g, 0, 0x40, g->phs->size, 0x00, g->shs->size);
}

void *alloc_len(long len, long *buf_len) {
	*buf_len = len;
	return malloc(len);
}

void cpy_len(uc *buf, long *tbuf, long value, int len) {
	*tbuf = value;
	memcpy(buf, tbuf, len);
}

struct Plov *find_label(struct Gner *g, char *s) {
	struct Plov *l;
	for (long i = 0; i < g->lps->size; i++) {
		l = g->lps->st[i];
		if (sc(l->l, s))
			return l;
	}
	return 0;
}

#define REL_SIZE 4

struct Usage *new_usage(uint64_t place, enum UT type) {
	struct Usage *u = malloc(sizeof(struct Usage));
	u->place = place;
	u->type = type;
	return u;
}

void gen_Linux_ELF_86_64_text(struct Gner *g) {
	long i, j, command, last_text_sz;
	long tmpb, *tmpp = &tmpb;	   // temp buf and ptr to buf
	long buf_len, *blp = &buf_len; // buf len ptr
	uc *ibuff;
	int all_h_sz = sizeof(struct ELFH) + g->phs->size * sizeof(struct ELFPH);
	enum ICode code;
	struct Inst *in;
	struct Token *tok;
	struct Plov *l;
	struct ELFPH *ph, *phl;
	struct Usage *usage;
	long phs_counter = 0;
	// struct ELFSH *sh;

	for (i = 0; i < g->is->size; i++) {
		in = plist_get(g->is, i);
		buf_len = 0;
		code = in->code;

		if (code >= IMOV_EAX_INT && code <= IMOV_ESI_LABEL) {
			tok = plist_get(in->os, 1);
			if (code >= IMOV_EAX_INT && code <= IMOV_ESI_INT) {
				if (code == IMOV_EAX_INT)
					command = 0xb8;
				else if (code == IMOV_EDX_INT)
					command = 0xba;
				else if (code == IMOV_EDI_INT)
					command = 0xbf;
				else if (code == IMOV_ESI_INT)
					command = 0xbe;

				ibuff = alloc_len(5, blp);
				cpy_len(ibuff, tmpp, command, 1);
				cpy_len(ibuff + 1, tmpp, tok->number, 4);
			} else
				switch (code) {
				case IMOV_ESI_LABEL:
					ibuff = alloc_len(1 + REL_SIZE, blp);
					cpy_len(ibuff, tmpp, 0xbe, 1);

					l = find_label(g, tok->view);
					usage = new_usage((uint64_t)(g->text->size) + 1, ADDR);
					plist_add(l->us, usage);
					cpy_len(ibuff + 1, tmpp, 0x766f6d6c, REL_SIZE);
					break;
				default:
					eeg("НЕИЗВЕСТНАЯ [быть] КОМАНДА", in);
				}

			if (buf_len) {
				blat(g->text, ibuff, buf_len);
				free(ibuff);
			}
			continue;
		}

		switch (code) {
		case IJMP:
			tok = plist_get(in->os, 0);
			l = find_label(g, tok->view);
			// TODO: near jmp
			ibuff = alloc_len(1 + REL_SIZE, blp);
			cpy_len(ibuff, tmpp, 0xe9, 1);
			// text size + 1 is palce in text to where need to put rel addr
			usage = new_usage((uint64_t)(g->text->size) + 1, REL_ADDR);
			plist_add(l->us, usage);
			cpy_len(ibuff + 1, tmpp, 0x706d6a, REL_SIZE);
			break;
		case ILABEL:
			tok = plist_get(in->os, 0);
			l = find_label(g, tok->view);
			l->a += g->text->size + all_h_sz;
			printf("label [%s]\n\t[0x%08lx]\n", l->l, l->a);
			break;
		case ILET:
			tok = plist_get(in->os, 0);
			l = find_label(g, tok->view);
			l->a += g->text->size + all_h_sz;
			printf("var   [%s]\n\t[0x%08lx]\n", l->l, l->a);

			struct BList *data = plist_get(in->os, 1);
			blat(g->text, data->st, data->size);
			break;
		case ISYSCALL:
			ibuff = alloc_len(2, blp);
			memcpy(ibuff, MLESYSCALL, 2);
			break;
		case ISEGMENT:
			/*
			 * 0 offset = 0
			 * 0 addr = 0 + pie
			 * 0 size = all p h + elf h + segment size
			 * i offset = last segment size + last segment offset
			 * i addr = offset + align * (i - 1) + pie
			 * i size = segment size
			 */
			//  do segment adress and offset and size for prev segment
			ph = g->phs->st[phs_counter];
			if (phs_counter == 0) {
				ph->offset = 0;		  // 0 offset
				ph->vaddr = g->pie;	  // 0 addr
				ph->memsz = all_h_sz; // + segment size // 0 size
			} else {
				phl = g->phs->st[phs_counter - 1];
				// because for first its size of all data that is only first
				// data for the moment
				if (phs_counter == 1)
					phl->memsz += g->text->size; // 0 size
				else
					phl->memsz = g->text->size - last_text_sz; // i size
				phl->filesz = phl->memsz;

				ph->offset = phl->memsz + phl->offset; // i offset
				ph->vaddr =
					ph->offset + ph->align * (phs_counter) + g->pie; // i addr
			}
			ph->filesz = ph->memsz;
			ph->paddr = ph->vaddr;
			phs_counter++;
			last_text_sz = g->text->size;
			break;
		case IEOI:
			phl = g->phs->st[phs_counter - 1];
			phl->memsz = g->phs->size == 1 ? phl->memsz + g->text->size
										   : g->text->size - last_text_sz;
			phl->filesz = phl->memsz;
			break;
			//	default:
			//		eeg("НЕИЗВЕСТНАЯ ИНСТРУКЦИЯ", in);
		}
		if (buf_len) {
			blat(g->text, ibuff, buf_len);
			free(ibuff);
		}
	}

	for (i = 0; i < g->is->size; i++) {
		in = plist_get(g->is, i);

		if (in->code == ILABEL || in->code == ILET) {
			tok = plist_get(in->os, 0); // in both cases name is first opperand
			l = find_label(g, tok->view);
			for (j = 0; j < l->us->size; j++) {
				usage = plist_get(l->us, j);

				long text_pos;
				void *textptr = ((uc *)g->text->st) + usage->place;
				if (usage->type == ADDR) {
					memcpy(textptr, &l->a, REL_SIZE);
				} else if (usage->type == REL_ADDR) {
					//  only far jump to the same segment
					//  but it works for other segemnts too?
					//  isnt fasm add also ph offset to rel addr
					//  ARE OFFSETS IRRELEVANT WITH JMP ON x64?
					//  im not sure but chat gpt says so and my tests also
					text_pos =
						l->a - g->pie - (usage->place + all_h_sz) - REL_SIZE;
					memcpy(textptr, &text_pos, REL_SIZE);
				}
			}
		} else if (in->code == IENTRY) {
			tok = in->os->st[0];
			l = find_label(g, tok->view);
			g->elfh->entry = l->a;
		}
	}
}
