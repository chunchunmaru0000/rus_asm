#include "gner.h"
#include <stdio.h>

int main() {
	char filename[] = "тест.асм";
	enum Target tr = Linux_ELF_86_64;

	struct Pser *p = new_pser(filename);
	struct PList *is = pse(p);
	struct Inst *in;
	for (int i = 0; i < is->size; i++) {
		in = plist_get(is, i);
		printf("i %s:%ld:%ld:t: %d\n", filename, in->line, in->col, in->code);
	}

	struct Gner *g = new_gner(is);
	gen(g, tr);

	return 0;
}
