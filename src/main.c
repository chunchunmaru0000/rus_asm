#include "gner.h"
#include <stdio.h>

int main() {
	char filename[] = "тест.асм";
	char outname[] = "тест";
	enum Target tr = Linux_ELF_86_64;

	struct Pser *p = new_pser(filename);
	struct PList *is = pse(p);
	struct Inst *in;
	for (int i = 0; i < is->size; i++) {
		in = plist_get(is, i);
		//printf("i %s:%ld:%ld:t: %d\n", filename, in->line, in->col, in->code);
	}

	struct Gner *g = new_gner(is, tr);
	gen(g);

	FILE *f = fopen(outname, "wb");
	fwrite(g->prol->st, 1, g->prol->size, f);
	//fwrite("from here generated", 1, 20, f);
	fwrite(g->text->st, 1, g->text->size, f);

	return 0;
}
