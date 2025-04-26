#include "gner.h"
#include <stdio.h>
#include <stdlib.h>

struct Gner *new_gner(struct PList *is) {
	struct Gner *g = malloc(sizeof(struct Gner));
	g->is = is;
	g->pos = 0;
	g->prol = new_blist(100);
	g->text = new_blist(100);
	return g;
}

void gen_Linux_ELF_86_64_prolog(struct Gner *);
void gen_Linux_ELF_86_64_text(struct Gner *);

void gen(struct Gner *g, enum Target tr) {
	switch (tr) {
	case Linux_ELF_86_64:
		gen_Linux_ELF_86_64_prolog(g);
		gen_Linux_ELF_86_64_text(g);
		break;
	case Linux_OBJ_86_64:
		break;
	case Windows_EXE_86_64:
		break;
	}
}

void eeg(const char* msg, struct Inst *i) {
	fprintf(stderr, "%s:%ld:%ld %s\n", i->file, i->line, i->col, msg);
}

void gen_Linux_ELF_86_64_prolog(struct Gner *) {
	
}
void gen_Linux_ELF_86_64_text(struct Gner *) {
	
}
