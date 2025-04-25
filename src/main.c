#include "tzer.h"
#include <stdio.h>

int main() {
	char filename[16] = "тест.асм";
	struct Tzer *t = new_tzer(filename);

	printf("%s:0:0\n%s\n", filename, t->code);

	struct Token *token = new_token(t);
	while (token->code != EOF) {
		printf("token: %s\n", token->view);
		token = new_token(t);
	}
	return 0;
}
