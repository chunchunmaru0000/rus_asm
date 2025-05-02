#include "gner.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **args) {
	if (argc < 3) {
		printf("Использвание: [Исходный код] [исполнимый файл]\n");
		exit(1);
	}
	if (access(args[1], F_OK) != 0) {
		printf(
			"ОШИБКА: Файл [%s] с предпологаемым исходным кодом не существует\n",
			args[1]);
		exit(1);
	}
	char *filename = args[1];
	char *outname = args[2];
	enum Target tr = Linux_ELF_86_64; // also need to be in args later
	uc debug = 1;

	struct Pser *p = new_pser(filename, debug);
	struct PList *is = pse(p);
	struct Gner *g = new_gner(is, tr, debug);
	gen(g);

	long bytes = g->prol->size + g->text->size;
	FILE *f = fopen(outname, "wb");
	fwrite(g->prol->st, 1, g->prol->size, f);
	fwrite(g->text->st, 1, g->text->size, f);
	fclose(f);

	printf("В файл [%s] записано %ld байт/а/ов\n", outname, bytes);
	return 0;
}
