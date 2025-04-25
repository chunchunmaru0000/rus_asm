#include "tzer.h"
#include <stdio.h>

int main() {
	char filename[16] = "тест.асм";
	struct Tzer *t = new_tzer(filename);

	printf("%s:0:0\n%s\n", filename, t->code);
	return 0;
}
