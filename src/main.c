#include "flgs.h"
#include <stdio.h>

int main(int argc, char **args) {
	struct Flags *fs = get_flags(argc, args);

	struct Pser *p = new_pser(fs->filename, fs->debug);
	struct PList *is = pse(p);
	struct Gner *g = new_gner(is, fs->tr, fs->debug);
	gen(g);

	long bytes = g->prol->size + g->text->size;
	FILE *f = fopen(fs->outname, "wb");
	fwrite(g->prol->st, 1, g->prol->size, f);
	fwrite(g->text->st, 1, g->text->size, f);
	fclose(f);

	printf("В файл [%s] записано %ld байт", fs->outname, bytes);
	int end = bytes % 10;
	if (end >= 2 && end <= 4)
		printf("а");
	putchar('\n');

	return 0;
}
