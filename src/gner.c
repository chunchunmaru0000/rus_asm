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

	h->osabi = 0; // UNIX System V ABI
	if (g->t == Linux_ELF_86_64)
		h->osabi = 3; // ELFOSABI_GNU

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
	p->label = ((struct Token *)plist_get(in->os, 0))->view;
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

struct Plov *find_label(struct Gner *g, char *s) {
	struct Plov *l;
	for (long i = 0; i < g->lps->size; i++) {
		l = g->lps->st[i];
		if (sc(l->label, s))
			return l;
	}
	char *err; // no need to free, cuz will be exit(1)
	asprintf(&err, UNKNOWN_LABEL, s);
	eeg(err, plist_get(g->is, g->pos));
	return 0;
}

void gen_Linux_ELF_86_64_text(struct Gner *g) {
	long i, j, last_text_sz;
	struct BList *cmd = new_blist(16), *data = new_blist(16);
	int all_h_sz = sizeof(struct ELFH) + g->phs->size * sizeof(struct ELFPH);
	enum ICode code;
	// take
	struct Inst *in;
	struct Token *tok;
	struct BList *data_bl;
	struct ELFPH *ph, *phl;
	// make
	struct Plov *l;
	struct Usage *usage;
	struct Defn *not_plov;

	struct Ipcd *ipcd = malloc(sizeof(struct Ipcd));
	ipcd->data = data;
	ipcd->cmd = cmd;
	ipcd->not_plovs = new_plist(2);
	ipcd->debug = g->debug;

	uint64_t phs_counter = 0, phs_cur_sz;

	for (i = 0; i < g->is->size; i++) {
		in = plist_get(g->is, i);
		g->pos = i;
		code = in->code;
		blist_clear(cmd);
		blist_clear(data);

		ipcd->in = in;
		switch (code) {
		case INOP:
		case IRET:
		case IRETF:
		case ISYSCALL:
		case ILOCK:
		case IADD:
		case IOR:
		case IADC:
		case ISBB:
		case IAND:
		case ISUB:
		case IXOR:
		case ICMP:
		case ITEST:
		case IMOV:
		case IJMPF:
		case ICALL:
		case IINT:
		case ICALLF:
		case IPUSH:
		case IPOP:
		case IINC:
		case IDEC:
		case IDIV:
		case IIDIV:
		case IMUL:
		case IIMUL:
		case IINSB:
		case IINSW:
		case IINSD:
		case IOUTSB:
		case IOUTSW:
		case IOUTSD:
		// TODO: near jmp
		case IJMP:
		case IJO:
		case IJNO:
		case IJB:
		case IJNB:
		case IJE:
		case IJNE:
		case IJBE:
		case IJA:
		case IJS:
		case IJNS:
		case IJP:
		case IJNP:
		case IJL:
		case IJNL:
		case IJLE:
		case IJG:
			get_ops_code(ipcd);
			// so here is has a list of denfs with usages relative to data size
			if (ipcd->not_plovs->size == 0)
				break;
			for (j = 0; j < ipcd->not_plovs->size; j++) {
				not_plov = plist_get(ipcd->not_plovs, j);
				usage = not_plov->value;

				usage->hc = phs_counter;
				usage->place += (uint64_t)(g->text->size) + cmd->size;

				ph = plist_get(g->phs, phs_counter - 1);
				uint64_t ph_start = ph->offset - all_h_sz * (phs_counter > 1);
				// phs_counter == 1 ? 0 : ph->offset - all_h_sz;
				usage->cmd_end =
					(g->text->size - ph_start) + cmd->size + data->size;

				l = find_label(g, not_plov->view);
				plist_add(l->us, usage);
			}
			plist_clear_items_free(ipcd->not_plovs);
			break;
		case ILABEL:
			tok = plist_get(in->os, 0);
			l = find_label(g, tok->view);

			ph = plist_get(g->phs, phs_counter - 1);
			l->addr = phs_cur_sz + ph->vaddr;
			l->rel_addr = g->text->size;
			if (g->debug & 1)
				printf("метка[%s]\t[0x%08lx]\n", l->label, l->addr);
			break;
		case IDATA:
			data_bl = plist_get(in->os, 0);
			blat(data, data_bl->st, data_bl->size);
			break;
		case ILET:
			tok = plist_get(in->os, 0);
			l = find_label(g, tok->view);

			ph = plist_get(g->phs, phs_counter - 1);
			l->addr = phs_cur_sz + ph->vaddr;
			l->rel_addr = g->text->size;
			if (g->debug & 1)
				printf("перем[%s]\t[0x%08lx]\n", l->label, l->addr);

			data_bl = plist_get(in->os, 1);
			blat(data, data_bl->st, data_bl->size);
			break;
		case ISEGMENT:
			phs_cur_sz = phs_counter ? 0 : all_h_sz;
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
				ph->offset = 0;		  // 0 offset = 0
				ph->vaddr = g->pie;	  // 0 addr = 0 + pie
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
			phl->memsz = g->phs->size == 1 ? (int64_t)phl->memsz + g->text->size
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
		}
	}

	phs_counter = 0;
	for (i = 0; i < g->is->size; i++) {
		in = plist_get(g->is, i);

		if (in->code == ILABEL || in->code == ILET) {
			// in both cases name is first opperand
			tok = plist_get(in->os, 0);
			l = find_label(g, tok->view);

			for (j = 0; j < l->us->size; j++) {
				usage = plist_get(l->us, j);
				uc *usage_place = g->text->st + usage->place;

				if (usage->type == ADDR) {
					memcpy(usage_place, &l->addr, REL_SIZE);
				} else if (usage->type == REL_ADDR) {
					ph = plist_get(g->phs, usage->hc - 1);
					uint64_t rel_addr = l->addr - (ph->vaddr + usage->cmd_end);
					memcpy(usage_place, &rel_addr, REL_SIZE); // TODO: rel8
				}
			}
		} else if (in->code == IENTRY) {
			tok = in->os->st[0];
			l = find_label(g, tok->view);
			g->elfh->entry = l->addr;
		} else if (in->code == ISEGMENT)
			phs_counter++;
	}
}
