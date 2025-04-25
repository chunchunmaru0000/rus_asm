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
	int base = 10;

	if (c == '0' && (n == 'x' || n == 'X' || n == 'b' || n == 'B' ||
					 (n == 0b11010001 && nn == 0b10000101) ||  // х cyrillic
					 (n == 0b11010000 && nn == 0b10100101) ||  // Х
					 (n == 0b11010000 && nn == 0b10110001) ||  // б
					 (n == 0b11010000 && nn == 0b10010001))) { // Б
		next(t);
		c = cur(t);
		n = get(t, 1);
		base = c == 'x' || c == 'X' || (c == 0b11010001 && n == 0b10000101) ||
					   (c == 0b11010000 && n == 0b10100101)
				   ? 16
				   : 2;
		if (c == 'x' || c == 'X' || c == 'b' || c == 'B')
			next(t);
		else {
			next(t);
			next(t);
		}
		ee(t, "НЕ РАБОТАЕТ ПОКА 0х и 0б, НАДО ПРИДУМАТЬ КАКФОРМАТ МЕНЯТЬ ОТ "
			  "РУССКИХ БУКВ ИЛИ САМОМУ ОБРАБАТЫВАТЬ");
		if (base == 16) {
			while (1) {
				next(t);
				c = cur(t);
				n = get(t, 1);
				if ((c < '0' || c > '9') && (c < 'A' || c > 'F') &&
					(c < 'a' || c > 'f') &&
					(c == 0b11010000 && ((n < 0b10110000 && n > 0b10110110) &&
										 (n < 0b10010000 && n > 0b10010110))))
					break;
				num_len++;
			}
		} else {
			while (1) {
				next(t);
				c = cur(t);
				if (c < '0' || c > '1')
					break;
				num_len++;
			}
		}
	} else {
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
	}

	num_view = malloc(num_len + 1);
	num_view[num_len] = 0;
	strncpy(num_view, &t->code[start_pos], num_len);

	if (base == 10)
		if (code == INT)
			token->number = atol(num_view);
		else
			token->fpn = atof(num_view);
	else
		token->number = strtol(num_view, NULL, base);

	token->view = num_view;
	return code;
}

enum TCode str_token(struct Tzer *t, struct Token *token) {
	long start_pos = t->pos, str_len = 2;
	char *str_view;
	next(t);
	// TODO: maybe do like \т\н\0\t\n\0
	while (cur(t) != '"') {
		next(t);
		str_len++;
	}
	next(t);

	str_view = malloc(str_len + 1);
	str_view[str_len] = 0;
	strncpy(str_view, &t->code[start_pos], str_len);

	token->view = str_view;
	return STR;
}

char *alloc_str_and_set_code(char str[], long len, enum TCode *codeptr,
							 enum TCode code) {
	char *s = malloc(len + 1);
	memcpy(s, str, len + 1);
	*codeptr = code;
	return s;
}
char *next_and_alloc(struct Tzer *t, char str[], long len, enum TCode *codeptr,
					 enum TCode code) {
	next(t);
	return alloc_str_and_set_code(str, len, codeptr, code);
}
char *usable_chars = ";:\\/+-*=,";
unsigned char usable_char(char c) {
	for (int i = 0; usable_chars[i]; i++)
		if (c == usable_chars[i])
			return 1;
	return 0;
}
enum TCode usable_token(struct Tzer *t, struct Token *token) {
	char *view;
	enum TCode code;
	enum TCode *cp = &code;
	unsigned char c = cur(t), n = get(t, 1), nn = get(t, 2);
	next(t);

	switch (c) {
	case ';':
		view = alloc_str_and_set_code(";", 1, cp, code);
		break;
	case ':':
		view = alloc_str_and_set_code(":", 1, cp, code);
		break;
	case '\\':
		view = alloc_str_and_set_code("\\", 1, cp, code);
		break;
	case '+':
		view = n == '+' ? next_and_alloc(t, "++", 2, cp, code)
						: alloc_str_and_set_code("+", 1, cp, code);
		break;
	case '-':
		view = alloc_str_and_set_code("-", 1, cp, code);
		break;
	case '*':
		view = alloc_str_and_set_code("*", 1, cp, code);
		break;
	case '/':
		view = alloc_str_and_set_code("/", 1, cp, code);
		break;
	case '=':
		view = alloc_str_and_set_code("=", 1, cp, code);
		break;
	default:
		ee(t, "НЕ ДОЛЖНО БЫТЬ ДОСТИЖИМО");
	}

	token->view = view;
	return code;
}
enum TCode id_token(struct Tzer *t, struct Token *token) { return ID; }
enum TCode com_token(struct Tzer *t, struct Token *token) { return ID; }

struct Token *new_token(struct Tzer *t) {
	while (cur(t) == ' ' || cur(t) == 13 ||
		   cur(t) == '\t') // for sure for windows
		next(t);

	unsigned char c = cur(t);
	enum TCode code;
	struct Token *token = malloc(sizeof(struct Token));
	token->line = t->line;
	token->col = t->col;

	// every of funcs that takes token shall assign view to token
	if (c == '\0')
		code = EOF;
	else if (c == ';')
		code = com_token(t, token);
	else if (c == '\n')
		code = next_line(t, token);
	else if (c >= '0' && c <= '9')
		code = num_token(t, token);
	else if (c == '"')
		code = str_token(t, token);
	else if (usable_char(c))
		code = usable_token(t, token);
	else
		code = id_token(t, token);

	token->code = code;
	return token;
}
