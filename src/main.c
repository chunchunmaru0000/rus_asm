#include "pser.h"
#include <stdio.h>

int main() {
	char filename[] = "тест.асм";
	enum Target tr = Linux_ELF_86_64;

	struct Pser *p = new_pser(filename);
	struct Inst *in;
	struct PList *is = pse(p);
	for (int i = 0; i < is->size; i++) {
		in = plist_get(is, i);
		printf("instruction %s:%ld:%ld:token: %d\n", filename, in->line, in->col, in->code);
	}
/*
*/

	return 0;
}
