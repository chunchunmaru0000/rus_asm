#include "ttypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct Tzer {
	char *filename;
	long line;
	long col;

	long pos;
	char *code;
	long codeLen;
};

char *EMPTY_STR = "_";

void ee(struct Tzer *t, char *msg) { // error exit
	fprintf(stderr, "%s:%ld:%ld %s\n", t->filename, t->line, t->col, msg);
	exit(1);
}

struct Tzer *new_tzer(char *filename) {
	struct Tzer *t = (struct Tzer *)malloc(sizeof(struct Tzer));
	t->filename = filename;
	t->line = 0;
	t->col = 0;
	t->pos = 0;

	FILE *file = fopen(filename, "r");
	if (!file)
		ee(t, "ОШИБКА В ОТКРЫТИИ ФАЙЛА");

	fseek(file, 0, SEEK_END);
	long size = ftell(file);
	rewind(file);

	char *text = (char *)malloc(size + 1);
	fread(text, 1, size, file);
	text[size] = '\0';

	t->code = text;
	t->codeLen = size - 1;
	return t;
}

#define next(t) (((t)->pos++, (t)->col++))
#define cur(t) ((t)->pos < (t)->codeLen ? (t)->code[(t)->pos] : '\0')
#define get(t, offset)                                                         \
	((t)->pos + (offset) < (t)->codeLen ? (t)->code[(t)->pos + (offset)] : '\0')

enum TCode next_line(struct Tzer *t, struct Token *token) {
	next(t);
	t->col = 0;
	t->line++;
	token->view = EMPTY_STR;
	return SLASHN;
}

enum TCode num_token(struct Tzer *t, struct Token *token) {
	unsigned char c = cur(t), n = get(t, 1), nn = get(t, 2), fpn = 0;
	long start_pos = t->pos, num_len = 1;
	enum TCode code = INT;
	char *num_view;

	if (c == '0' && (n == 'x' || n == 'X' || n == 'b' || n == 'B' ||
					 (n == 0b11010001 && nn == 0b10000101) || // х cyrillic
					 (n == 0b11010000 && nn == 0b10100101) || // Х
					 (n == 0b11010000 && nn == 0b10110001) || // б
					 (n == 0b11010000 && nn == 0b10010001)))  // Б
		ee(t, "ЦИФРЫ С ПРИСТАВКАМИ ПОКА ЕЩЕ НЕ СДЕЛАНЫ");

	while (1) {
		next(t);
		c = cur(t);
		if (c == '.') {
			fpn++;
			if (fpn > 1)
				ee(t, "СЛИШКОМ МНОГО ТОЧЕК НА ОДНО ЧИСЛО");
			code = REAL;
			next(t);
			c = cur(t);
			num_len++;
		}
		if (c < '0' || c > '9')
			break;
		num_len++;
	}

	num_view = malloc(num_len + 1);
	num_view[num_len] = 0;
	strncpy(num_view, &t->code[start_pos], num_len);

	if (code == INT)
		token->number = atol(num_view);
	else
		token->fpn = atof(num_view);

	token->view = num_view;
	return code;
}

enum TCode str_token(struct Tzer *t, struct Token *token) { return EOF; }
enum TCode usable_token(struct Tzer *t, struct Token *token) { return EOF; }
enum TCode usable_char(char c) { return EOF; }

struct Token *new_token(struct Tzer *t) {
	while (cur(t) == ' ' || cur(t) == 13 ||
		   cur(t) == '\t') // for sure for windows
		next(t);

	unsigned char c = cur(t);
	enum TCode code;
	struct Token *token = malloc(sizeof(struct Token));
	token->line = t->line;
	token->col = t->col;

	// everu of funcs that takes t and token shall assign view to token
	if (c == '\0')
		code = EOF;
	else if (c == '\n')
		code = next_line(t, token);
	else if (c >= '0' && c <= '9')
		code = num_token(t, token);
	else if (c == '"')
		code = str_token(t, token);
	else if (usable_char(c))
		code = usable_token(t, token);
	else
		ee(t, "НЕИЗВЕСТНЫЙ СИМВОЛ");

	token->code = code;
	return token;
}
