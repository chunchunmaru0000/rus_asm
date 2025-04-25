#include "tzer.h"
#include <stdio.h>

int main() {
	char filename[16] = "тест.асм";
	struct Tzer *t = new_tzer(filename);
	printf("%s:0:0\n%s\n", filename, t->code);

	struct Token *token;
	struct PList *l = get_tokens(t, 10);
	for (int i = 0; i < l->size; i++) {
		token = list_get(l, i);
		printf("%s:%ld:%ld:token: %s : %d\n", filename, token->line, token->col,
			   token->view, token->code);
	}

	return 0;
}
