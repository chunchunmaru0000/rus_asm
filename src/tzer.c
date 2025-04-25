#include "ttypes.h"
#include <stdio.h>
#include <string.h>

struct Tzer {
	char *filename;
	long line;
	long col;

	long pos;
	char *code;
};

struct Tzer *new_tzer(char *filename) {
	FILE *file = fopen(filename, "r");
	if (!file) {
		fprintf(stderr, "ОШИБКА В ОТКРЫТИИ ФАЙЛА [%s]", filename);
		exit(1);
	}

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);

	char *text = (char *)malloc(size + 1);
	fread(text, 1, size, file);
	text[size] = '\0';

	struct Tzer *t = (struct Tzer *)malloc(sizeof(struct Tzer));
	t->line = 0;
	t->col = 0;
	t->pos = 0;
	t->code = text;
	return t;
}

#define next(t) (((t)->pos++, (t)->col++))
#define cur(t) ((t)->code[(t)->pos])
void next_line(struct Tzer *t) {
	next(t);
	t->col = 0;
	t->line++;
}

struct Token *new_token(struct Tzer *t) {
	while (cur(t) == 10 || cur(t) == 13) // for sure for windows
		next(t);

	char c = cur(t);
	struct Token *token = malloc(sizeof(struct Token));
	token->line = t->line;
	token->col = t->col;

	if (c == 0) {
		token->code = EOF;
	} else if (c == 10) {
		next_line(t);
		token->code = SLASHN;
	} else {
		fprintf(stderr, "НЕИЗВЕСТНЫЙ СИМВОЛ [%d]", c);
		exit(1);
	}

	return token;
}
