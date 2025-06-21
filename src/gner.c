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
	g->compiled = 0;
	g->prol = new_blist(100);
	g->text = new_blist(100);

	g->pie = 0x400000;
	g->lps = new_plist(5);
	g->jmps = new_plist(8);
	g->shs = new_plist(1);
	// g->phs = new_plist(3);

	struct Ephs *es = malloc(sizeof(struct Ephs));
	es->phs_c = 0;
	es->phs_cur_sz = 0;
	es->all_h_sz = 0;
	es->phs = new_plist(3);
	g->eps = es;

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

		blat(g->prol, (uc *)g->elfh, sizeof(struct ELFH));
		for (int i = 0; i < g->eps->phs->size; i++)
			blat(g->prol, plist_get(g->eps->phs, i), sizeof(struct ELFPH));
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

	struct Inst *i = plist_get(g->is, 0);
	switch (g->t) {
	case Linux_ELF_86_64:
		memcpy(h->type, ELFH_TYPE_EXEC, 2);
		break;
	case Linux_OBJ_86_64:
		memcpy(h->type, ELFH_TYPE_DYN, 2);
		break;
	default:
		ee(i->f, i->p, "НЕ ДОСТУПНАЯ ЦЕЛЬ КОМПИЛЯЦИИ НА ДАННЫЙ МОМЕНТ");
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

	int segment_place = g->eps->phs->size - 1;
	p->label = ((struct Token *)plist_get(in->os, 0))->view;
	p->si = segment_place;
	p->us = new_plist(4);
	// p->a = g->pie; // + first ph memsz;
	p->ipos = g->pos;
	p->declared = 0;

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
		g->pos = i;

		if (in->code == ISEGMENT) {
			flags = plist_get(in->os, 0);
			// type =   1,  flags, offset,     adress,     size
			ph = new_ph(1, *flags, 0x7366666f, 0x72646461, 0x657a6973);
			plist_add(g->eps->phs, ph);
		} else if (in->code == ILABEL || in->code == ILET)
			plist_add(g->lps, new_label(g, in));
	}
	// entry = 0 but will be editet
	g->elfh = new_elfh(g, 0, 0x40, g->eps->phs->size, 0x00, g->shs->size);
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
	struct Inst *in = plist_get(g->is, g->pos);
	ee(in->f, in->p, err);
	return 0;
}

void recompile_jmp_rel32_as_jmp_rel8(struct Ipcd *i) {
	blist_clear(i->cmd);
	blist_clear(i->data);
	((struct Oper *)plist_get(i->in->os, 0))->sz = BYTE;
	get_ops_code(i);
}

const char *const TOO_BIG_TO_BE_REL_8 =
	"Значение было слишком большим чтобы уместиться в 1 байт.";
const char *const ERR_ZERO_SEGMENTS =
	"На Линуксе для правильного исполнимого файла в данном ассембелере "
	"требуется хотя бы один участок.";

void assert_phs_not_zero(struct Gner *g, struct Inst *in) {
	if (g->eps->phs->size == 0)
		ee(in->f, in->p, ERR_ZERO_SEGMENTS);
}

int try_to_short_to_rel_8(struct Gner *g, struct Ipcd *i, struct Plov **l) {
	if ((0b1 & OPT_FLAG) == 0)
		return UNSHORTABLE;

	struct Oper *oper = plist_get(i->in->os, 0);
	int rel_addr = -129, is_shorted = UNSHORTABLE;

	if (is_imm(oper)) {
		if (oper->code == OREL) {
			assert_phs_not_zero(g, i->in);

			// 1 plov for rel 32 value
			struct Defn *not_plov = plist_get(i->not_plovs, 0);
			struct Usage *usage = not_plov->value;
			*l = find_label(g, not_plov->view);

			if (usage->type == REL_ADDR && (*l)->si == g->eps->phs_c - 1) {
				if ((*l)->declared) {
					rel_addr =
						(*l)->rel_addr - (g->text->size + SHORT_JMP_CMND_SZ);
					if (is_in_byte(rel_addr))
						plist_clear_items_free(i->not_plovs); // frees usage too
				} else
					// not yet declared but possibly can be shorter
					is_shorted = SHORTABLE;
			}
		} else
			rel_addr = oper->code == OINT ? oper->t->number : oper->t->fpn;

		if (is_in_byte(rel_addr)) {
			recompile_jmp_rel32_as_jmp_rel8(i);
			is_shorted = SHORTENED;
		}
	}

	return is_shorted;
}

struct Jump *try_find_in_jmps(struct Gner *g, struct Plov *l) {
	struct Jump *jmp, *found_jmp = 0;

	// TODO: i beleive here need a reverse loop so that it shortens
	// the nearest ones before the farest
	for (uint32_t i = 0; i < g->jmps->size; i++) {
		jmp = plist_get(g->jmps, i);

		if (l->ipos == jmp->lipos) {
			// this should be right
			int rel_addr =
				g->text->size - jmp->addr -
				(jmp->code == IJMP ? LONG_JMP_CMND_SZ : LONG_NOT_JMP_CMND_SZ);

			if (is_in_byte(rel_addr)) {
				found_jmp = jmp;
				break;
			} else {
				// TODO: i beleive dont need to anything here
				// cuz if its just too far for NOW
				// its still can be possibly SHORT after other shortings
				// BUT still some of them can already be not reachable and
				// doesnt matter what si shortened
				// in such a case maybe just need to free(jmp) and jmps->st++
				// TODO: find out how to determine if UNSHORTABLE
				//  to count jmps between laaabel and jmp and have max shortable
				//  size
			}
		}
	}

	return found_jmp;
}

struct Jump *new_jmp(struct Gner *g, struct Ipcd *ipcd, struct Plov *l,
					 enum ICode code) {
	struct Jump *jmp = malloc(sizeof(struct Jump));
	jmp->lipos = l->ipos;
	jmp->addr = g->text->size;
	jmp->ipos = g->pos;
	jmp->code = code;
	jmp->size = inst_size(ipcd);
	return jmp;
}

void shift_left_on_shorter(struct Gner *g, struct Jump *jmp, int shorter) {
	int i, ui, uj;
	struct Plov *l;
	struct Jump *tjmp;
	struct Usage *usage;

	int jmp_rel_end = jmp->addr + jmp->size;
	void *jmp_end = g->text->st + jmp_rel_end;
	memmove(jmp_end, jmp_end + shorter, g->text->size - jmp_rel_end);

	g->text->size -= shorter;
	g->eps->phs_cur_sz -= shorter;

	for (i = 0; i < g->lps->size; i++) {
		l = plist_get(g->lps, i);

		if (l->ipos > jmp->ipos) {
			l->addr -= shorter;
			l->rel_addr -= shorter;
			l->declared = 0;
		}

		for (ui = 0; ui < l->us->size; ui++) {
			usage = plist_get(l->us, ui);

			if (usage->ic > jmp->ipos) {
				for (uj = ui; uj < l->us->size; uj++) {
					usage = plist_get(l->us, uj);

					usage->place -= shorter;
					usage->cmd_end -= shorter;
				}
				goto exit_shift_labels_loop;
			} else if (usage->ic == jmp->ipos) {
				usage->place = jmp->addr + 1;
				usage->type = REL_ADDR_8;
				printf("\t%s usage->place: %ld\n", l->label, usage->place);

				struct ELFPH *ph = plist_get(g->eps->phs, g->eps->phs_c - 1);
				uint64_t ph_start =
					ph->offset - g->eps->all_h_sz * (g->eps->phs_c > 1);
				// 2 is size of cmd and data
				usage->cmd_end = (g->text->size - ph_start) + 2;
			}
		}
	}
exit_shift_labels_loop:

	for (ui = 0; ui < g->jmps->size; ui++) {
		tjmp = plist_get(g->jmps, ui);

		if (tjmp->ipos > jmp->ipos) {
			for (uj = ui; uj < g->jmps->size; uj++) {
				tjmp = plist_get(g->jmps, uj);

				tjmp->addr -= shorter;
			}
			return; // exit loop
		}
	}
}

void recompile_loop(struct Gner *g, struct Ipcd *ipcd, struct Jump *jmp) {
	struct Token *tok;
	struct Plov *l;
	struct Inst *in;

	// ipcd hves recompiled jmp unstruction and jmp holds old jmp size
	int shorter = jmp->size - inst_size(ipcd);
	jmp->size = inst_size(ipcd);
	printf("\tg->text->size: %ld\tshorter: %d\tjmp->addr: %d\n", g->text->size,
		   shorter, jmp->addr);
	if (!shorter)
		return;
	shift_left_on_shorter(g, jmp, shorter);
	// all rel8 jmps are 1 byte cmd
	g->text->st[jmp->addr] = ipcd->cmd->st[0];

	for (long ri = jmp->ipos; ri < g->compiled; ri++) {
		g->pos = ri;
		in = plist_get(g->is, ri);

		ipcd->in = in;
		if (in->code == ILET || in->code == ILABEL) {
			tok = plist_get(in->os, 0);
			l = find_label(g, tok->view);

			jmp = try_find_in_jmps(g, l);
			if (jmp) {
				in = plist_get(g->is, jmp->ipos); // i = jmp->ipos
				ipcd->in = in;					  // jmp instruction

				recompile_jmp_rel32_as_jmp_rel8(ipcd);

				recompile_loop(g, ipcd, jmp);
				break; //  ???
			}
		}
	}
}

void gen_Linux_ELF_86_64_text(struct Gner *g) {
	long i, j, last_text_sz, li, ui, uj;
	struct BList *cmd = new_blist(16), *data = new_blist(16);
	g->eps->all_h_sz =
		sizeof(struct ELFH) + g->eps->phs->size * sizeof(struct ELFPH);
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
	struct Jump *jmp, *tjmp;

	struct Ipcd *ipcd = malloc(sizeof(struct Ipcd));
	ipcd->data = data;
	ipcd->cmd = cmd;
	ipcd->not_plovs = new_plist(2);
	ipcd->debug = g->debug;

	int *phs_c = &g->eps->phs_c;

	for (i = 0; i < g->is->size; i++) {
		in = plist_get(g->is, i);
		g->pos = i;
		code = in->code;
		blist_clear(cmd);
		blist_clear(data);

		ipcd->in = in;
		if (code > IDATA) {
			get_ops_code(ipcd);

			if (is_rel8_shortable(code)) {
				if (try_to_short_to_rel_8(g, ipcd, &l) == SHORTABLE)
					plist_add(g->jmps, new_jmp(g, ipcd, l, code));
			}

		gen_Linux_ELF_86_64_text_add_usages:
			if (ipcd->not_plovs->size == 0)
				goto gen_Linux_ELF_86_64_text_first_loop_end;

			// so here is has a list of denfs with usages relative to data size
			for (j = 0; j < ipcd->not_plovs->size; j++) {
				assert_phs_not_zero(g, in);

				not_plov = plist_get(ipcd->not_plovs, j);
				usage = not_plov->value;
				l = find_label(g, not_plov->view);

				usage->hc = *phs_c;
				usage->ic = g->pos;
				usage->place += (uint64_t)(g->text->size) + cmd->size;

				ph = plist_get(g->eps->phs, *phs_c - 1);
				uint64_t ph_start =
					ph->offset - g->eps->all_h_sz * (*phs_c > 1);
				// phs_counter == 1 ? 0 : ph->offset - all_h_sz;
				usage->cmd_end =
					(g->text->size - ph_start) + cmd->size + data->size;

				plist_add(l->us, usage);
			}
			plist_clear_items_free(ipcd->not_plovs);
			goto gen_Linux_ELF_86_64_text_first_loop_end;
		}
		switch (code) {
		case ILABEL:
		case ILET:
			tok = plist_get(in->os, 0);
			l = find_label(g, tok->view);

			assert_phs_not_zero(g, in);
			ph = plist_get(g->eps->phs, *phs_c - 1);
			l->addr = g->eps->phs_cur_sz + ph->vaddr;
			l->rel_addr = g->text->size;
			l->declared = 1;

			// if jmp means its SHORTABLE
			jmp = try_find_in_jmps(g, l);
			if (jmp && (0b10 & OPT_FLAG)) {
				i = jmp->ipos; // it will be incremented after the loop
				g->pos = i;
				in = plist_get(g->is, i); // i = jmp->ipos
				ipcd->in = in;			  // jmp instruction

				recompile_jmp_rel32_as_jmp_rel8(ipcd);
				jmp->size = inst_size(ipcd);

				g->eps->phs_cur_sz -= g->text->size - jmp->addr;
				g->text->size = jmp->addr;

				for (li = 0; li < g->lps->size; li++) {
					l = plist_get(g->lps, li);
					if (l->ipos >= jmp->ipos)
						l->declared = 0;

					for (ui = 0; ui < l->us->size; ui++) {
						usage = plist_get(l->us, ui);

						if (usage->ic >= jmp->ipos) {
							for (uj = ui; uj < l->us->size; uj++)
								free(plist_get(l->us, uj));
							l->us->size = ui;
							break;
						}
					}
				}

				for (ui = 0; ui < g->jmps->size; ui++) {
					tjmp = plist_get(g->jmps, ui);

					if (tjmp->ipos == jmp->ipos) {
						for (uj = ui; uj < g->jmps->size; uj++)
							free(plist_get(g->jmps, uj));
						g->jmps->size = ui;
						break;
					}
				}
				// l->declared = 0; // because returns back in pos
				goto gen_Linux_ELF_86_64_text_add_usages;
			}

			if (g->debug & 1)
				printf("метка[%s]\t[0x%08lx]\n", l->label, l->addr);

			if (code == ILET) {
				data_bl = plist_get(in->os, 1);
				blat(data, data_bl->st, data_bl->size);
			}
			break;
		case IDATA:
			data_bl = plist_get(in->os, 0);
			blat(data, data_bl->st, data_bl->size);
			break;
		case IALIGN:
			j = ((struct Oper *)plist_get(in->os, 0))->t->number;
			j = (j - ((g->text->size + g->eps->all_h_sz) % j)) % j;
			printf("\talign %ld\n", j);
			get_align(ipcd, j);
			break;
		case ISEGMENT:
			g->eps->phs_cur_sz = *phs_c ? 0 : g->eps->all_h_sz;
			/*
			 * 0 offset = 0
			 * 0 addr = 0 + pie
			 * 0 size = all p h + elf h + segment size
			 * i offset = last segment size + last segment offset
			 * i addr = offset + align * (i - 1) + pie
			 * i size = segment size
			 */
			//  do segment adress and offset and size for prev segment
			ph = g->eps->phs->st[*phs_c];
			if (*phs_c == 0) {
				ph->offset = 0;				  // 0 offset = 0
				ph->vaddr = g->pie;			  // 0 addr = 0 + pie
				ph->memsz = g->eps->all_h_sz; // + segment size // 0 size
			} else {
				phl = g->eps->phs->st[*phs_c - 1];
				// because for first its size of all data that is only first
				// data for the moment
				if (*phs_c == 1)
					// 0 size = all p h + elf h + segment size
					phl->memsz += g->text->size;
				else
					// i size = segment size
					phl->memsz = g->text->size - last_text_sz;
				phl->filesz = phl->memsz;

				// i offset = last segment size + last segment offset
				ph->offset = phl->memsz + phl->offset;
				// i addr = offset + align * (i - 1) + pie
				ph->vaddr = ph->offset + ph->align * (*phs_c) + g->pie;
			}
			ph->filesz = ph->memsz;
			ph->paddr = ph->vaddr;
			(*phs_c)++;
			last_text_sz = g->text->size;
			// if its another segment then its already not near jmp
			plist_clear_items_free(g->jmps);
			break;
		case IEOI:
			assert_phs_not_zero(g, in);
			phl = g->eps->phs->st[*phs_c - 1];
			phl->memsz = g->eps->phs->size == 1
							 ? (int64_t)phl->memsz + g->text->size
							 : g->text->size - last_text_sz;
			phl->filesz = phl->memsz;
			break;
		case IENTRY:
			break;
		default:
			printf("%d ", in->code);
			ee(in->f, in->p, "НЕИЗВЕСТНАЯ ИНСТРУКЦИЯ");
		}

	gen_Linux_ELF_86_64_text_first_loop_end:
		// if (1 || g->pos >= g->compiled) {
		// 	g->compiled++;
		if (inst_size(ipcd)) {
			blat(g->text, cmd->st, cmd->size);
			blat(g->text, data->st, data->size);

			g->eps->phs_cur_sz += inst_size(ipcd);
		}
		//}
	}

	phs_c = 0;
	for (i = 0; i < g->is->size; i++) {
		g->pos = i;
		in = plist_get(g->is, i);
		if (in->code == ILABEL || in->code == ILET) {
			// in both cases name is first opperand
			tok = plist_get(in->os, 0);
			l = find_label(g, tok->view);

			for (j = 0; j < l->us->size; j++) {
				usage = plist_get(l->us, j);
				uc *usage_place = g->text->st + usage->place;

				if (usage->type == ADDR) {
					memcpy(usage_place, &l->addr, DWORD);
				} else {
					ph = plist_get(g->eps->phs, usage->hc - 1);
					int rel_addr =
						l->addr - (ph->vaddr + usage->cmd_end +
								   g->eps->all_h_sz * ((usage->hc - 1) == 0));

					if (usage->type == REL_ADDR)
						memcpy(usage_place, &rel_addr, DWORD);
					else { // REL_ADDR_8
						if (!is_in_byte(rel_addr)) {
							if (g->debug)
								printf("было то %d;\n", rel_addr);
							in = plist_get(g->is, usage->ic);
							ee(in->f, in->p, TOO_BIG_TO_BE_REL_8);
						}
						memcpy(usage_place, &rel_addr, BYTE);
					}
				}
			}
		} else if (in->code == IENTRY) {
			tok = in->os->st[0];
			l = find_label(g, tok->view);
			g->elfh->entry = l->addr;
		} else if (in->code == ISEGMENT)
			phs_c++;
	}
}
