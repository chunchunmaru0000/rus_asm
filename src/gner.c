#include "gner.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Gner *new_gner(struct PList *is, enum Target t, uc debug) {
	struct Gner *g = malloc(sizeof(struct Gner));
	g->t = t;
	g->debug = debug;
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
	p->l = ((struct Token *)plist_get(in->os, 0))->view;
	p->si = segment_place;
	p->us = new_plist(4);
	// p->a = g->pie; // + first ph memsz;

	return p;
}

// machine codes
const uc MLESYSCALL[] = {0x0f, 0x05};

void gen_Linux_ELF_86_64_prolog(struct Gner *g) {
	int *flags;
	struct Inst *in;
	struct ELFPH *ph;
	// struct ELFSH *sh;

	for (long i = 0; i < g->is->size; i++) {
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

const char *UNKNOWN_LABEL = "Неизвестная метка [%s]";
struct Plov *find_label(struct Gner *g, char *s) {
	struct Plov *l;
	for (long i = 0; i < g->lps->size; i++) {
		l = g->lps->st[i];
		if (sc(l->l, s))
			return l;
	}
	char *err;
	asprintf(&err, UNKNOWN_LABEL, s);
	eeg(err, plist_get(g->is, g->pos));
	return 0;
}

#define REL_SIZE 4

struct Usage *new_usage(uint64_t place, enum UT type) {
	struct Usage *u = malloc(sizeof(struct Usage));
	u->place = place;
	u->type = type;
	return u;
}

uint64_t reg2reg(enum RegCode l, enum RegCode r, int size) {
	uint64_t v = 0;
	enum RegCode lmin, rmin;

	switch (size) {
	case QWORD:
		lmin = l >= R_RAX && l <= R_RDI ? R_RAX : R_R8;
		rmin = r >= R_RAX && r <= R_RDI ? R_RAX : R_R8;
		v = (l - lmin) + (r - rmin) * 8;
		break;
	case DWORD:

		break;
	case WORD:
		break;
	case BYTE:
		break;
	}
	return v;
}

void gen_Linux_ELF_86_64_text(struct Gner *g) {
	long i, j, last_text_sz;
	struct BList *cmd = new_blist(16), *data = new_blist(8);
	uint64_t some_value = 0, vlen = 0;
	int all_h_sz = sizeof(struct ELFH) + g->phs->size * sizeof(struct ELFPH);
	enum ICode code;
	// take
	struct Inst *in;
	struct Token *tok;
	struct Oper * or, *ol;
	struct BList *data_bl;
	struct ELFPH *ph, *phl;
	// make
	struct Plov *l;
	struct Usage *usage;
	long phs_counter = 0, phs_cur_sz;
	// struct ELFSH *sh;

	for (i = 0; i < g->is->size; i++) {
		in = plist_get(g->is, i);
		g->pos = i;
		code = in->code;
		blist_clear(cmd);
		blist_clear(data);

		if (code == IADD || code == IMOV) {
			get_ops_code(in, cmd, data);

			if (cmd->size + data->size) {
				blat(g->text, cmd->st, cmd->size);
				blat(g->text, data->st, data->size);

				phs_cur_sz += cmd->size + data->size;

				blist_clear(cmd);
				blist_clear(data);
			}
			continue;
		} else if (code == IMOV) {
			ol = plist_get(in->os, 0);
			or = plist_get(in->os, 1);
			switch (ol->code) {
			case OREG:
				switch (or->code) {
				case OREG:
					if (ol->sz != or->sz)
						eeg(REGS_SIZES_NOT_MATCH, in);
					some_value = reg2reg(ol->rm, or->rm, ol->sz);
					switch (ol->sz) {
					case QWORD:
						vlen = 3;
						some_value = (some_value << 16) + 0xc08948;
						break;
					case DWORD:
						break;
					case WORD:
						break;
					case BYTE:
						break;
					}
					blat(cmd, (uc *)&some_value, vlen);
					break;
				case OINT:
				case OFPN:
				case OREL:
					if (or->sz == DWORD) {
						if (ol->rm >= R_EAX && ol->rm <= R_EDI) {
							vlen = 1;
							some_value = 0xb8 + ol->rm - R_EAX;
						} else if (ol->rm >= R_R8D && ol->rm <= R_R15D) {
							vlen = 2;
							some_value = ((0xb8 + ol->rm - R_R8D) << 8) + 0x41;
						} else if (ol->rm >= R_RAX && ol->rm <= R_RDI) {
							vlen = 3;
							some_value =
								((0xc0 + ol->rm - R_RAX) << 16) + 0xc748;
						} else if (ol->rm >= R_R8 && ol->rm <= R_R15) {
							vlen = 3;
							some_value =
								((0xc0 + ol->rm - R_R8) << 16) + 0xc749;
						} else
							eeg(WRONG_FST_OPER_REG_DWORD, in);
					} else if (or->sz == QWORD) {
						if (ol->rm >= R_RAX && ol->rm <= R_RDI) {
							vlen = 2;
							some_value = ((0xb8 + ol->rm - R_RAX) << 8) + 0x48;
						} else if (ol->rm >= R_R8 && ol->rm <= R_R15) {
							vlen = 2;
							some_value = ((0xb8 + ol->rm - R_R8) << 8) + 0x49;
						} else
							eeg(WRONG_FST_OPER_REG_QWORD, in);
					} else
						eeg(WRONG_SND_OPER_SIZE, in);

					blat(cmd, (uc *)&some_value, vlen);

					tok = or->t;
					if (or->code == OINT)
						blat(data, (uc *)&tok->number, or->sz);
					else if (or->code == OFPN) {
						if (or->sz == DWORD) {
							float tmp_real = (float)tok->fpn;
							blat(data, (uc *)&tmp_real, DWORD);
						} else if (or->sz == QWORD)
							blat(data, (uc *)&tok->fpn, QWORD);
						else
							eeg(WRONG_FPN_OP_SIZE, in);
					} else {
						some_value = 0x766f6d6c;
						blat(data, (uc *)&some_value, or->sz);

						l = find_label(g, tok->view);
						usage = new_usage((uint64_t)(g->text->size) + cmd->size,
										  ADDR);
						plist_add(l->us, usage);
					}
					break;
				default:
					if (g->debug)
						printf("прав код = %d\t", or->code);
					eeg(WRONG_SND_OPER, in);
				}
				break;
			default:
				if (g->debug)
					printf("лев  код = %d\t", ol->code);
				eeg(WRONG_FST_OPER, in);
			}
			if (cmd->size + data->size) {
				blat(g->text, cmd->st, cmd->size);
				blat(g->text, data->st, data->size);

				phs_cur_sz += cmd->size + data->size;

				blist_clear(cmd);
				blist_clear(data);
			}
			continue;
		}

		switch (code) {
		//  TODO: near jmp
		case IJMP:
			ol = plist_get(in->os, 0);
			//  TODO: jmp for qword
			if (ol->sz == DWORD) {
				blist_add(cmd, 0xe9);

				if (ol->code == OREL) {
					l = find_label(g, ol->t->view);
					// text size + 1 is place in text to where need to put rel
					usage = new_usage((uint64_t)(g->text->size) + cmd->size,
									  REL_ADDR);
					plist_add(l->us, usage);

					some_value = 0x706d6a;
					blat(cmd, (uc *)&some_value, DWORD);
				} else if (ol->code == OINT)
					blat(data, (uc *)&ol->t->number, DWORD);
				//  TODO: jmp for reg
				else {
					if (g->debug)
						printf("ol->code = %d\t", ol->code);
					eeg(WRONG_FST_OPER, in);
				}
			}
			break;
		case ILABEL:
			tok = plist_get(in->os, 0);
			l = find_label(g, tok->view);

			ph = plist_get(g->phs, phs_counter - 1);
			l->a = phs_cur_sz + ph->vaddr;
			l->ra = g->text->size;
			if (g->debug)
				printf("label[%s]\t[0x%08lx]\n", l->l, l->a);
			break;
		case IDATA:
			data_bl = plist_get(in->os, 0);
			blat(data, data_bl->st, data_bl->size);
			break;
		case ILET:
			tok = plist_get(in->os, 0);
			// printf("try find [%s], i = %ld, code = %d\n", tok->view, i,
			// code);
			l = find_label(g, tok->view);

			ph = plist_get(g->phs, phs_counter - 1);
			l->a = phs_cur_sz + ph->vaddr;
			l->ra = g->text->size;
			if (g->debug)
				printf(" var [%s]\t[0x%08lx]\n", l->l, l->a);

			data_bl = plist_get(in->os, 1);
			blat(data, data_bl->st, data_bl->size);
			break;
		case ISYSCALL:
			blat(cmd, (uc *)MLESYSCALL, 2);
			break;
		case INOP:
			blist_add(cmd, 0x90);
			break;
		case ISEGMENT:
			if (phs_counter == 0)
				phs_cur_sz = all_h_sz;
			else
				phs_cur_sz = 0;
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
		case IENTRY:
			break;
		default:
			printf("%d ", in->code);
			eeg("НЕИЗВЕСТНАЯ ИНСТРУКЦИЯ", in);
		}
		if (cmd->size + data->size) {
			blat(g->text, cmd->st, cmd->size);
			blat(g->text, data->st, data->size);

			phs_cur_sz += cmd->size + data->size;

			blist_clear(cmd);
			blist_clear(data);
		}
	}

	phs_counter = 0;
	for (i = 0; i < g->is->size; i++) {
		in = plist_get(g->is, i);

		if (in->code == ILABEL || in->code == ILET) {
			tok = plist_get(in->os,
							0); // in both cases name is first opperand
			l = find_label(g, tok->view);
			for (j = 0; j < l->us->size; j++) {
				usage = plist_get(l->us, j);

				void *textptr = ((uc *)g->text->st) + usage->place;
				if (usage->type == ADDR) {
					memcpy(textptr, &l->a, REL_SIZE);
				} else if (usage->type == REL_ADDR) {
					long text_pos = l->ra - usage->place - REL_SIZE;
					memcpy(textptr, &text_pos, REL_SIZE);
				}
			}
		} else if (in->code == IENTRY) {
			tok = in->os->st[0];
			l = find_label(g, tok->view);
			g->elfh->entry = l->a;
		} else if (in->code == ISEGMENT)
			phs_counter++;
	}
}
